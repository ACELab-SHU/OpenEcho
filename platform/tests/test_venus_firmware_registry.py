import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


class FirmwareRegistryTests(unittest.TestCase):
    def setUp(self):
        source = Path(__file__).parents[1] / 'components/gem5/tools/venus_firmware_registry.py'
        spec = importlib.util.spec_from_file_location('firmware_registry_test', source)
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        document = Path(self.tmp.name) / 'dag.json'
        document.write_text(json.dumps([{'return_output': [
            {'parentTasksPort': '0b0', 'parentTasks': '0', 'index': 0, 'length': 64}]}]))
        self.runtime = {
            'prefix': 'test', 'blob': b'CODE' + bytes(64),
            'tasks': [{'code_address': 0, 'code_length': 4}],
            'dmt_layout': {'json': str(document), 'slots': [
                {'task': 0, 'port': 0, 'capacity': 64, 'relative_offset': 128}]},
        }
        self.tables = {
            'test_task_container': bytes(384), 'test_global_para': bytes(64),
            'test_output_num': bytes([1]) + bytes(63),
            'test_task_num': bytes([1]) + bytes(63),
            'test_output_addr': ((128 << 16) | 64).to_bytes(6, 'little') + bytes(122),
            'test_return_value': (((1 << 502) - 2) << 10).to_bytes(64, 'little'),
        }
        class Elf:
            def __init__(self, tables):
                self.symbols = tables
            def symbol_data(self, key):
                return self.symbols[key]
        self.elf = Elf(self.tables)

    def test_exact_identity_excludes_runtime_values(self):
        segments, returns = self.module.identity_segments(self.elf, self.runtime)
        self.assertEqual(segments[-1], (0, b'CODE'))
        self.assertEqual(returns, [{'task': 0, 'port': 0, 'source': 0, 'length': 64}])
        changed = copy.deepcopy(self.runtime)
        changed['blob'] = b'CODE' + b'X' * 64
        self.assertEqual(self.module.identity_segments(self.elf, changed), (segments, returns))

    def test_code_change_changes_identity(self):
        before = self.module.identity_segments(self.elf, self.runtime)
        self.runtime['blob'] = b'EDIT' + bytes(64)
        self.assertNotEqual(before, self.module.identity_segments(self.elf, self.runtime))

    def test_stale_json_dmt_rejected(self):
        self.runtime['dmt_layout']['slots'][0]['relative_offset'] += 64
        with self.assertRaisesRegex(ValueError, 'DMT layout'):
            self.module.identity_segments(self.elf, self.runtime)

    def test_return_route_and_padding_rejected(self):
        for bit in [0, 30]:
            with self.subTest(bit=bit):
                original = self.tables['test_return_value']
                self.tables['test_return_value'] = (int.from_bytes(original, 'little') ^
                                                    (1 << bit)).to_bytes(64, 'little')
                with self.assertRaisesRegex(ValueError, 'return table'):
                    self.module.identity_segments(self.elf, self.runtime)
                self.tables['test_return_value'] = original

    def test_missing_or_oversized_descriptor_rejected(self):
        original = self.tables.pop('test_global_para')
        with self.assertRaisesRegex(ValueError, 'requires test_global_para'):
            self.module.identity_segments(self.elf, self.runtime)
        self.tables['test_global_para'] = original
        self.tables['test_task_num'] = bytes(65)
        with self.assertRaisesRegex(ValueError, 'identity size'):
            self.module.identity_segments(self.elf, self.runtime)

    def test_invalid_code_range_rejected(self):
        self.runtime['tasks'][0]['code_length'] = 1000
        with self.assertRaisesRegex(ValueError, 'code outside'):
            self.module.identity_segments(self.elf, self.runtime)


if __name__ == '__main__':
    unittest.main()
