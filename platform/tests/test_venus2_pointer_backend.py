"""Backend pointer encoding must preserve payload and dynamic ownership."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from ace_echo.adapters.scheduler import SchedulerAdapter


class Venus2PointerBackendTests(unittest.TestCase):
    def test_native_pointer_encodings_and_reserved_types(self):
        root = Path(__file__).resolve().parents[1]
        backend = json.loads((root / 'configs/backends/venus2p0-16x128.json').read_text())
        descriptor = backend['abi']['task_input_descriptor']
        self.assertEqual(descriptor['physical_type_bits'], 3)
        self.assertEqual(descriptor['rtl_unsupported_logical_types'], [])
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'scheduler'
            shutil.copytree(root / 'components/scheduler', source)
            SchedulerAdapter._configure_backend_copy(source)
            spec = importlib.util.spec_from_file_location('pointer_regression_parser', source / 'dags/read_dag_json.py')
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            parser = module.DAG_parser('unused.json', task_input_type_bits=3,
                                       logical_type_map=','.join(f'{k}:{v}' for k, v in descriptor['logical_to_physical_type'].items()))
            for value in (0, 1, 2, 4, 5, 6):
                with self.subTest(value=value):
                    logical, encoded = parser._DAG_parser__input_type_bin(value, 0, 0)
                    self.assertEqual(logical, value)
                    self.assertEqual(encoded, format(5 if value == 6 else value, '03b'))
            for value in (3, 7, 8):
                with self.subTest(reserved=value):
                    with self.assertRaises(ValueError):
                        parser._DAG_parser__input_type_bin(value, 0, 0)

    def test_remapped_runtime_pointer_keeps_dynamic_input_and_table_entry(self):
        root = Path(__file__).resolve().parents[1]
        backend = json.loads((root / 'configs/backends/venus2p0-16x128.json').read_text())
        mapping = backend['abi']['task_input_descriptor']['logical_to_physical_type']
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'scheduler'
            shutil.copytree(root / 'components/scheduler', source)
            SchedulerAdapter._configure_backend_copy(source)
            spec = importlib.util.spec_from_file_location('dynamic_pointer_parser', source / 'dags/read_dag_json.py')
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            parser = module.DAG_parser('unused.json', task_input_type_bits=3,
                                       logical_type_map=','.join(f'{k}:{v}' for k,v in mapping.items()))
            # Adjacent static and runtime pointers must read different entries.
            # Physical 5 is a pointer transfer, not physical 1 (payload copy).
            parser.json_data = [{
                'hash':'0001','text_offset':0,'text_length':64,
                'data_offset':64,'data_length':0,'hardwareinfo':'0b1111',
                'Input_Num':2,'Output_Num':0,
                'all_input':[
                    {'name':'static_table','type':5,'index':1,'dest_address':'0x22c40','offset':0x300,'length':64},
                    {'name':'runtime_table','type':6,'index':2,'dest_address':'0x22c80','offset':0x400,'length':4096},
                ],
            }]
            parser._DAG_parser__task_container_parser()
            parser._DAG_parser__input_offset_parser()
            self.assertEqual(parser.l1_input_num, 1)
            self.assertEqual(parser.input_name, ['runtime_table'])
            self.assertEqual(parser.input_offset, [0x400])
            self.assertEqual(parser.input_length, [4096])
            packed = int(parser.task_container_mem[0], 2)
            for i, address in enumerate((0x22c40,0x22c80)):
                descriptor = (packed >> (45*i)) & ((1 << 45)-1)
                self.assertEqual(descriptor >> 42, 5)
                self.assertEqual(descriptor & 0xffffffff, address)
                self.assertEqual((descriptor >> 32) & 0x3ff, i+1)
            self.assertEqual(int(parser.global_para_descriptor_mem[0],2), (0x300 << 16) | 64)
            self.assertEqual(int(parser.global_para_descriptor_mem[1],2), (0x400 << 16) | 4096)
            # Explicit native-type-6 backends remain configurable; no global rewrite.
            native = module.DAG_parser('unused.json', task_input_type_bits=3,
                                       logical_type_map='0:0,1:1,2:2,4:4,5:5,6:6')
            self.assertEqual(native._DAG_parser__input_type_bin(6,0,0), (6,'110'))


if __name__ == '__main__':
    unittest.main()
