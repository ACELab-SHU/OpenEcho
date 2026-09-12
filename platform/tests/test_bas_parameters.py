import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

from ace_echo.bas_parameters import (parse_parameters, materialize, runtime_include,
                                     read_binding, sha256, uncomment)
from ace_echo.adapters.toolchain import ToolchainAdapter
from ace_echo.adapters.scheduler import SchedulerAdapter


class BasParameterTests(unittest.TestCase):
    def test_windows_line_endings(self):
        self.assertEqual(parse_parameters('parameter short x={1}\r\n')['x']['values'],[1])
        self.assertEqual(materialize('dag_input short x[1]\r\n','parameter short x={1}\r\n')[1]['runtime'][0]['capacity'],1)

    def test_comments_are_not_values(self):
        text = "' parameter char fake = {99}\nparameter short x = {1, ' ignored 900\n -2}\n"
        p = parse_parameters(text)
        self.assertEqual(list(p), ['x'])
        self.assertEqual(p['x']['values'], [1, -2])
        self.assertEqual(parse_parameters("' parameter short x = {2}\n"), {})

    def test_bas_types_and_numeric_literals(self):
        p = parse_parameters('parameter char a = {-128,255}\nparameter short b = {-32768,65535}\nparameter int c = {-2147483648,4294967295}\nparameter float d = {0.5,-1E2}\nparameter double e = {1.25}\n')
        self.assertEqual(p['d']['values'], [0.5,-100.0])
        self.assertEqual(parse_parameters('parameter int n={008}')['n']['literals'], ['8'])
        self.assertEqual(parse_parameters('parameter float n={008}')['n']['literals'], ['8'])

    def test_invalid_syntax_rejected(self):
        for body in ['1,,2','1,','', '0x10','1+2','1 2','1foo','NaN','+1','1e2','1;2', '__import__("os")']:
            with self.subTest(body=body), self.assertRaises(ValueError):
                parse_parameters('parameter int x = {'+body+'}')
        for text in ['hello','parameter short x={1}\ngarbage','parameter long x={1}', 'parameter short x={1}\nparameter short x={2}']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                parse_parameters(text)

    def test_ranges_and_types_rejected(self):
        for typ, value in [('char','256'),('char','-129'),('short','65536'),('int','4294967296'),('int','1.5'),('float','1E99')]:
            with self.subTest(typ=typ,value=value), self.assertRaises(ValueError):
                parse_parameters('parameter %s x={%s}' % (typ,value))

    def test_static_override_and_external_definition(self):
        bas = "parameter short a={1}\nparameter short keep={7}\ndag dag1 = {[out] = Task(a, b, keep)}\n"
        text, receipt = materialize(bas, 'parameter short a={2}\nparameter short b={3}\n')
        self.assertIn('parameter short a = {2}', text)
        self.assertIn('parameter short b = {3}', text)
        self.assertIn('parameter short keep={7}', text)
        self.assertEqual([v['action'] for v in receipt['changes']], ['override_inline','external_definition'])

    def test_empty_external_file_preserves_bas_bytes(self):
        bas = "' parameter short old={1}\nparameter short x={2}\nEND\n"
        self.assertEqual(materialize(bas, "' comment only\n")[0], bas)

    def test_conflicts_rejected(self):
        cases = [('parameter short x={1}', 'parameter char x={1}'),
                 ('dag_input short x[1]', 'parameter char x={1}'),
                 ('dag_input short x[1]', 'parameter short x={1,2}'),
                 ('dag_input short x[1]\ndag_input short y[1]', 'parameter short x={1}'),
                 ('global short x', 'parameter short x={1}'),
                 ('return_value short x[1]', 'parameter short x={1}'),
                 ('dag dag1={}', 'parameter short typo={1}')]
        for bas, params in cases:
            with self.subTest(bas=bas), self.assertRaises(ValueError):
                materialize(bas, params)

    def test_runtime_storage_keeps_interface_capacity(self):
        bas = 'dfedata char iq[64]\ndag_input short mode[1]\n'
        merged, receipt = materialize(bas, 'parameter char iq={1,-2}\nparameter short mode={15}\n')
        self.assertEqual(merged, bas)
        header = runtime_include(receipt['runtime'])
        self.assertIn('char iq[64]', header)
        self.assertIn('section(".rfdata_data")', header)
        self.assertIn('short mode[1]', header)

    def test_comment_marker_inside_bas_string_is_preserved(self):
        self.assertEqual(uncomment('PRINT "a\'b"'), 'PRINT "a\'b"')

    def test_manifest_opt_in_and_escape(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            self.assertIsNone(ToolchainAdapter._parameter_source(root, None))
            params = root/'input.params'
            params.write_text('parameter short x={1}\n')
            m = root/'dag-source.json'
            m.write_text(json.dumps({'parameters':'input.params'}))
            self.assertEqual(ToolchainAdapter._parameter_source(root,None), params)
            m.write_text(json.dumps({'parameters':'../outside.params'}))
            with self.assertRaises(ValueError):
                ToolchainAdapter._parameter_source(root,None)

    def test_handoff_digest_and_main_opt_in(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            dag, binary = root/'dag1.json', root/'dag1.bin'
            dag.write_text('[]')
            binary.write_bytes(b'bin')
            self.assertIsNone(read_binding(dag,binary))
            header = b'/* input */\n'
            (root/'ace_echo_inputs.inc').write_bytes(header)
            binding = dict(schema='ace-echo-bas-parameters/v1',dag_json_sha256=sha256(dag.read_bytes()),dag_bin_sha256=sha256(binary.read_bytes()), runtime_include_sha256=sha256(header))
            (root/'parameters.json').write_text(json.dumps(binding))
            self.assertEqual(read_binding(dag,binary), binding)
            src = root/'source'
            src.mkdir()
            with self.assertRaisesRegex(ValueError,'isolated'):
                SchedulerAdapter._stage_parameter_inputs(src,'../outside.c',dag,binding)
            (src/'main.c').write_text('void main(void){}')
            with self.assertRaisesRegex(ValueError,'include'):
                SchedulerAdapter._stage_parameter_inputs(src,'main.c',dag,binding)
            (src/'main.c').write_text('#include "ace_echo_inputs.inc"\n')
            SchedulerAdapter._stage_parameter_inputs(src,'main.c',dag,binding)
            self.assertTrue((src/'main.c').read_text().startswith('#define ACE_ECHO_EXTERNAL_INPUTS 1'))
            (root/'ace_echo_inputs.inc').write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'digest'):
                SchedulerAdapter._stage_parameter_inputs(src,'main.c',dag,binding)
            binary.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'match'):
                read_binding(dag,binary)

    def test_missing_sidecar_is_not_silent_fallback(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            dag=root/'dag1.json'
            binary=root/'dag1.bin'
            dag.write_text(json.dumps([{'current_taskId':0,'_ace_echo_parameters':{'runtime_inputs':True}}, {'return_output':[]}]))
            binary.write_bytes(b'bin')
            with self.assertRaisesRegex(ValueError,'missing parameters.json'):
                read_binding(dag,binary)

    def test_migrated_tv9_equals_legacy_values_and_storage(self):
        repo = Path(__file__).resolve().parents[1]
        script = repo/'components/scheduler/python/materialize_pdcch_tv9.py'
        spec = importlib.util.spec_from_file_location('tv9_materializer',script)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        path = repo/'components/scheduler/dags/input/nrPDCCH_tv9.params'
        values = module.parse_parameters(path.read_text())
        # Independently frozen from the legacy 7c0c877 TV9 parser/data before migration.
        self.assertEqual(sha256(json.dumps(values,sort_keys=True).encode()),
                         '1673aeabd65075ac9a824eb849c94d61e91b675034e06648a1972557a7dd58d8')
        self.assertEqual(len(values),19)
        for name,typ,capacity,section in module.SPECS:
            self.assertIn('[%d]' % capacity,module.emit_declaration(name,typ,capacity,section,values[name]))


if __name__ == '__main__':
    unittest.main()
