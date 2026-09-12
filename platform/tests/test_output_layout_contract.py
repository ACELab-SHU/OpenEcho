from copy import deepcopy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from ace_echo.output_layout_contract import parse_contract, resolve_contract, export_contract, discover_layouts
from ace_echo.dag_output_compare import compare_dag_outputs


class OutputContractTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name); self.dag = self.root/'dag1.json'
        self.data = [{'current_taskId':7, 'debug_task_name':'Task_generic',
                      'all_output':[{'name':'count','length':64,'allocation_bytes':64}]}]
        self.doc = {'schema':'ace-echo-output-contract/v1', 'basis':'One int16 field in an aligned 64-byte object.',
                    'outputs':[{'task':'Task_generic','output':'count','layout':{
                        'schema':'ace-echo-output-layout/v1','transport_bytes':64,'valid_bit_ranges':[[0,16]]}}]}
        self.dag.write_text(json.dumps(self.data)); self.dag.with_suffix('.bin').write_bytes(b'compiled-input')

    def export(self): return export_contract(json.dumps(self.doc).encode(),self.dag)

    def test_legacy_strict_no_contract(self):
        self.assertIsNone(discover_layouts(self.dag)); self.assertIsNone(discover_layouts(None))

    def test_name_mapping_and_binding(self):
        path=self.export(); self.assertEqual(discover_layouts(self.dag),path)
        item=json.loads(path.read_text())['outputs'][0]
        self.assertEqual((item['task_id'],item['port']),(7,0))
        with self.assertRaises(ValueError): self.export()

    def test_explicit_case_contract_wins(self):
        explicit=self.root/'case-layout.json'
        self.assertEqual(discover_layouts(self.dag,explicit),explicit)

    def test_tampered_components_rejected(self):
        self.export()
        for path in [self.dag,self.dag.with_suffix('.bin'),self.root/'output-validity.json',self.root/'output-layouts.json']:
            with self.subTest(path=path.name):
                raw=path.read_bytes(); path.write_bytes(raw+b' ')
                with self.assertRaises(ValueError): discover_layouts(self.dag)
                path.write_bytes(raw)
        path=self.root/'output-layouts.binding.json'
        data=json.loads(path.read_text()); data['layout_sha256']='0'*64; path.write_text(json.dumps(data))
        with self.assertRaises(ValueError): discover_layouts(self.dag)

    def test_missing_binding_is_not_silently_ignored(self):
        self.export(); (self.root/'output-layouts.binding.json').unlink()
        with self.assertRaises(ValueError): discover_layouts(self.dag)

    def test_unknown_and_ambiguous_identity_fail(self):
        for data in [[],[dict(self.data[0],debug_task_name='Other')],self.data*2]:
            with self.assertRaises(ValueError): resolve_contract(self.doc,data)

    def test_capacity_cannot_be_bypassed(self):
        doc=deepcopy(self.doc); doc['outputs'][0]['layout']['transport_bytes']=128
        with self.assertRaises(ValueError): resolve_contract(doc,self.data)

    def test_invalid_and_duplicate_contracts(self):
        for key,value in [('schema','bad'),('basis',''),('outputs',[])]:
            doc=deepcopy(self.doc); doc[key]=value
            with self.assertRaises(ValueError): parse_contract(json.dumps(doc).encode())
        doc=deepcopy(self.doc); doc['outputs']*=2
        with self.assertRaises(ValueError): parse_contract(json.dumps(doc).encode())
        doc=deepcopy(self.doc); doc['outputs'][0]['layout']['valid_bit_ranges']=[]
        with self.assertRaises(ValueError): parse_contract(json.dumps(doc).encode())

    def compare(self,values,actual=b'\x01\0'+bytes(62),extra=False):
        dma=self.root/'dma.log'; dma.write_text('retained raw evidence')
        folder=self.root/'outputs'; folder.mkdir(exist_ok=True)
        (folder/'task_7_port_0.bin').write_bytes(actual)
        records=[{'task_id':7,'task_name':'generic','port':0,'values':values,'length':64}]
        if extra:
            records.append(dict(records[0],task_id=8)); (folder/'task_8_port_0.bin').write_bytes(actual)
        with patch('ace_echo.rtl_return_identity.resolve_return_identity',return_value=(records,{})):
            return compare_dag_outputs(dma,folder,dag_json=self.dag,rtl_log=self.root/'sim.log',identity_contract={})

    def test_automatic_padding_warning_and_valid_data_failure(self):
        self.export(); result=self.compare([1,0]+[None]*62)
        self.assertEqual(result['status'],'PASS'); self.assertEqual(result['transport_bit_exact_status'],'FAIL')
        self.assertEqual(result['warnings'][0]['code'],'PADDING_UNKNOWN')
        self.assertEqual(result['warnings'][0]['bit_count'],496)
        for values in ([2,0]+[None]*62,[1,None]+[None]*62):
            self.assertEqual(self.compare(values)['status'],'FAIL')

    def test_missing_extra_bytes_and_undeclared_outputs_stay_errors(self):
        self.export()
        for actual in (b'\x01\0',b'\x01\0'+bytes(63)):
            self.assertEqual(self.compare([1,0]+[None]*62,actual)['status'],'FAIL')
        self.assertEqual(self.compare([1,0]+[None]*62,extra=True)['status'],'FAIL')

    def test_strict_legacy_comparison_unchanged(self):
        self.assertEqual(self.compare([1,0]+[None]*62)['status'],'FAIL')


if __name__ == '__main__': unittest.main()
