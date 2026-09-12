import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'venus_test'))
import scalar_o
import type_verify
from venus_ext_vector_normalize import normalize_venus_ext_vector_source


class ReturnLengthTests(unittest.TestCase):
    def extract(self, body, bas='', params='short_struct count', typedef=''):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'config.mk').write_text('TARGET_DAG=stale\n')
            (root/'probe.bas').write_text(bas)
            folder = root/'venus_test'
            folder.mkdir()
            source = ('typedef short Vec __attribute__((ext_vector_type(8400)));\n'
                      'typedef struct { short data; } __attribute__((aligned(64))) short_struct;\n'
                      + typedef + '\nint Task_probe(' + params + ') { Vec out; ' + body + ' }')
            (folder/'Task_probe.c').write_text(source)
            cwd = os.getcwd()
            try:
                os.chdir(folder)
                with patch.dict(os.environ, {'TARGET_DAG': 'probe'}):
                    return scalar_o.extract_info('Task_probe.c')[1]
            finally:
                os.chdir(cwd)

    def test_build_target_overrides_stale_config(self):
        self.assertEqual(self.extract('int n=count.data; vreturn(out,n);',
            'parameter char input = {240,0}\n[x]=Task_probe(input)'), [{'out':240}])

    def test_multi_byte_little_endian(self):
        self.assertEqual(self.extract('vreturn(out,count.data);',
            'parameter char input = {0,2}\n[x]=Task_probe(input)'), [{'out':512}])

    def test_all_calls_and_branch_maximum(self):
        self.assertEqual(self.extract('int n; if(count.data==1){n=40;}else{n=48;} vreturn(out,n*2);',
            'parameter short a={1}\nparameter short b={2}\n[x]=Task_probe(a)\n[y]=Task_probe(b)'), [{'out':96}])

    def test_unknown_call_does_not_reuse_first_length(self):
        self.assertEqual(self.extract('int n=count.data; vreturn(out,n);',
            'parameter short a={12}\n[x]=Task_probe(a)\n[y]=Task_probe(runtime_input)'), [{'out':16800}])

    def test_unknown_reassignment_invalidates_initializer(self):
        self.assertEqual(self.extract('int n=12; n=unknown(); vreturn(out,n);'), [{'out':16800}])

    def test_unknown_branch_joins_all_paths(self):
        self.assertEqual(self.extract('int n; if(count.data){n=96;}else{n=80;} vreturn(out,n);'), [{'out':16800}])
        # Different constants do not pretend to be one exact runtime value.

    def test_unknown_branch_with_return_sites_uses_one_port(self):
        self.assertEqual(self.extract('if(count.data){vreturn(out,96);}else{vreturn(out,80);}'), [{'out':96}])

    def test_unknown_loop_and_shadowing(self):
        self.assertEqual(self.extract('int n=12; while(count.data){n=unknown();} vreturn(out,n);'), [{'out':16800}])
        self.assertEqual(self.extract('int n=100; {int n=10;} vreturn(out,n);'), [{'out':16800}])

    def test_alias_and_real_vector_capacity(self):
        text='typedef short v2048i16 __attribute__((ext_vector_type(8400))); v2048i16 out;'
        self.assertEqual(scalar_o.resolve_sizeof(text,'out'),16800)
        self.assertEqual(scalar_o.resolve_sizeof(text,'v2048i16'),16800)

    def test_alias_in_header(self):
        self.assertEqual(scalar_o.resolve_sizeof('Old out;', 'out',
            'typedef short Old __attribute__((ext_vector_type(73)));'),146)

    def test_comment_markers_in_line_comments_and_strings(self):
        self.assertEqual(self.extract('//** */\n int n=20; printf("/*text*/"); vreturn(out,n);'), [{'out':20}])

    def test_struct_fields_and_multiline_parameter(self):
        self.assertEqual(self.extract('int n=config.blocks*24; vreturn(out,n);',
            'parameter short cfg={\n0,4\n}\n[x]=Task_probe(cfg)', params='Config config',
            typedef='typedef struct Config {short start; short blocks;} __attribute__((aligned(64))) Config;'), [{'out':96}])

    def test_unknown_parameter_token_not_skipped(self):
        self.assertEqual(self.extract('vreturn(out,count.data);',
            'parameter char input={unknown,2}\n[x]=Task_probe(input)'), [{'out':16800}])

    def test_conditional_preprocessor_not_guessed(self):
        self.assertEqual(self.extract('\n#if FEATURE\nint n=10;\n#else\nint n=20;\n#endif\nvreturn(out,n);'), [{'out':16800}])

    def test_inconsistent_ports_rejected(self):
        with self.assertRaisesRegex(ValueError,'inconsistent vreturn'):
            self.extract('if(count.data){vreturn(out,4);}else{vreturn(out,4,out,4);}')

    def test_sizeof_expression_not_truncated(self):
        self.assertEqual(self.extract('vreturn(out,sizeof(short)*40);'), [{'out':80}])

    def test_side_effects_in_initializers_not_ignored(self):
        self.assertEqual(self.extract('int n=12; int x=mutate(&n); vreturn(out,n);'), [{'out':16800}])
        self.assertEqual(self.extract('int n=12; int x=(n=100); vreturn(out,n);'), [{'out':16800}])
        self.assertEqual(self.extract('int n=12; printf("%n",&n); vreturn(out,n);'), [{'out':16800}])

    def test_runtime_input_is_not_a_parameter(self):
        self.assertEqual(self.extract('vreturn(out,count.data+64);',
            'dag_input short input[1]\n[x]=Task_probe(input)'), [{'out':16800}])

    def test_nonstandard_alias_is_vector_for_compiler_and_input_abi(self):
        text=('typedef char v4096i8 __attribute__((ext_vector_type(4096)));\n'
              'int Task_probe(v4096i8 data, short_struct length){vreturn(data,length.data);}')
        rewritten=normalize_venus_ext_vector_source(text)
        self.assertIn('Task_probe(__v4096i8 data, short_struct length)',rewritten)
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'Task_probe.c'
            path.write_text(text)
            self.assertEqual(type_verify.extract_function_arguments(path,'Task_probe')['args'],
                [{'name':'data','type':'__v4096i8'}, {'name':'length','type':'short_struct'}])

    def test_misleading_alias_uses_actual_count_for_compiler(self):
        text='typedef short __v2048i16 __attribute__((ext_vector_type(8400))); __v2048i16 out;'
        self.assertIn('__v8400i16 out;',normalize_venus_ext_vector_source(text))

    def test_unsigned_arithmetic_is_not_signed_folded(self):
        self.assertEqual(self.extract('unsigned int n=0; vreturn(out,(n-1)/2+4);'), [{'out':16800}])

    def test_commented_typedef_does_not_override_live_alias(self):
        text=('typedef short Vec __attribute__((ext_vector_type(8400)));\n'
              '// typedef short Vec __attribute__((ext_vector_type(2048)));\nVec out;')
        self.assertIn('__v8400i16 out;',normalize_venus_ext_vector_source(text))

    def test_readonly_printf_newline_keeps_scalar_value(self):
        self.assertEqual(self.extract('short n=48; printf("num = %hd\\n",&n); vreturn(out,n*2);'), [{'out':96}])


if __name__ == '__main__':
    unittest.main()
