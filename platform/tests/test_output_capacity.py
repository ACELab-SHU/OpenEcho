import copy
import json
from pathlib import Path
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import Mock

from ace_echo.output_capacity import (export_allocations, output_capacity,
    check_compiled_returns, check_observed_returns)


class OutputCapacityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.dag = [dict(current_taskId=7, debug_task_name='Task_demo*1', Output_Num=1,
                         all_output=[dict(name='result', length=128, temp_offset=0)])]
        self.registers = dict(format='venus-mmio-return-ir-v1', count_address='0x102c',
                              length_base='0x1034', stride_bytes=8, port_count=16)

    def ir(self, lengths):
        text = '\n'.join('store volatile i32 1, ptr inttoptr (i32 4140 to ptr), align 4\n'
                          f'store volatile i32 {n}, ptr inttoptr (i32 4148 to ptr), align 4'
                          for n in lengths)
        (self.root/'Task_demo.0.ll').write_text(text)
        return check_compiled_returns(self.dag, self.root, self.registers, self.root/'report.json')

    def test_literal_return_fits(self):
        self.assertEqual(self.ir([128])['status'], 'PASS')

    def test_sizeof_folded_to_4096_is_hard_error(self):
        with self.assertRaisesRegex(ValueError, 'OUTPUT_CAPACITY_EXCEEDED'):
            self.ir([4096])
        report = json.loads((self.root/'report.json').read_text())
        self.assertEqual(report['status'], 'FAIL')
        self.assertEqual(report['diagnostics'][0]['allocation_bytes'], 128)

    def test_error_branch_checked_not_only_final_return(self):
        with self.assertRaises(ValueError):
            self.ir([4096, 128])

    def test_unknown_is_pending_not_pass(self):
        self.assertEqual(self.ir(['%length'])['status'], 'PENDING_RUNTIME')

    def test_unknown_does_not_hide_bad_constant_branch(self):
        with self.assertRaises(ValueError):
            self.ir(['%length', 4096])

    def test_negative_length_rejected(self):
        with self.assertRaises(ValueError):
            self.ir([-1])

    def test_empty_error_return_is_not_misreported_as_overflow(self):
        report = self.ir([0])
        self.assertEqual(report['diagnostics'][0]['code'], 'OUTPUT_EMPTY_RETURN')

    def test_legacy_length_not_rounded_by_guess(self):
        self.assertEqual(output_capacity(dict(length=2)), 2)

    def test_allocation_smaller_than_declared_rejected(self):
        with self.assertRaises(ValueError):
            output_capacity(dict(length=128, allocation_bytes=64))

    def allocation_map(self):
        return dict(row_bytes=64, total_bytes=1024, allocations=[
            dict(name='result', size_bytes=2, temp_offset=0)])

    def test_two_byte_struct_in_proven_64byte_slot_is_warning(self):
        self.dag[0]['all_output'][0]['length'] = 2
        export_allocations(self.dag, self.allocation_map())
        self.assertEqual(self.dag[0]['all_output'][0]['allocation_bytes'], 64)
        report = self.ir([64])
        self.assertEqual(report['status'], 'PASS')
        self.assertEqual(report['diagnostics'][0]['severity'], 'WARNING')
        with self.assertRaises(ValueError):
            self.ir([65])

    def test_allocator_join_rejects_wrong_offset_or_size(self):
        self.dag[0]['all_output'][0]['length'] = 2
        for key, val in [('temp_offset', 64), ('size_bytes', 3), ('name', 'other')]:
            table = self.allocation_map()
            table['allocations'][0][key] = val
            with self.subTest(key=key), self.assertRaises(ValueError):
                export_allocations(copy.deepcopy(self.dag), table)

    def test_alignment_is_from_allocator_not_hardware_default(self):
        self.dag[0]['all_output'][0]['length'] = 2
        table = self.allocation_map()
        table['row_bytes'] = 32
        export_allocations(self.dag, table)
        self.assertEqual(self.dag[0]['all_output'][0]['allocation_bytes'], 32)

    def test_runtime_catches_dynamic_overrun(self):
        path = self.root/'dag.json'
        path.write_text(json.dumps(self.dag))
        (self.root/'task_7_port_0.bin').write_bytes(bytes(129))
        with self.assertRaisesRegex(ValueError, 'OUTPUT_CAPACITY_EXCEEDED'):
            check_observed_returns(path, self.root, self.root/'runtime.json')
        (self.root/'task_7_port_0.bin').write_bytes(bytes(128))
        self.assertEqual(check_observed_returns(path, self.root, self.root/'runtime.json')['status'], 'PASS')

    def test_extra_return_port_is_not_ignored(self):
        path = self.root/'dag.json'
        path.write_text(json.dumps(self.dag))
        (self.root/'task_7_port_0.bin').write_bytes(bytes(128))
        (self.root/'task_7_port_1.bin').write_bytes(bytes(64))
        with self.assertRaisesRegex(ValueError, 'OUTPUT_UNEXPECTED'):
            check_observed_returns(path, self.root, self.root/'runtime.json')

    def test_missing_output_and_duplicate_task_fail(self):
        path = self.root/'dag.json'
        path.write_text(json.dumps(self.dag))
        with self.assertRaisesRegex(ValueError, 'OUTPUT_MISSING'):
            check_observed_returns(path, self.root, self.root/'runtime.json')
        self.dag *= 2
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.ir([128])

    def test_rtl_overrun_rejected_before_source_snapshot_or_build(self):
        from ace_echo.adapters.rtl import RtlAdapter
        dag = self.root/'dag.json'
        dag.write_text(json.dumps(self.dag))
        (self.root/'task_7_port_0.bin').write_bytes(bytes(4096))
        adapter = object.__new__(RtlAdapter)
        adapter.runner = SimpleNamespace(dry_run=False)
        adapter.backend = SimpleNamespace(gem5=SimpleNamespace(venus_config="venus-rtl-16x128"))
        adapter._validate_source_identity = Mock(side_effect=AssertionError('must not reach RTL'))
        with self.assertRaisesRegex(ValueError, 'OUTPUT_CAPACITY_EXCEEDED'):
            adapter.run(workspace=self.root/'rtl-artifacts', cases=[object()], timeout=10,
                        dag_json=dag, gem5_output_dir=self.root)
        adapter._validate_source_identity.assert_not_called()
        self.assertFalse((self.root/'rtl-artifacts/rtl-source').exists())

    def test_unconfigured_abi_or_missing_ir_pending(self):
        report = check_compiled_returns(self.dag, self.root, None, self.root/'report.json')
        self.assertEqual(report['status'], 'PENDING_RUNTIME')

    def test_count_mismatch_rejected(self):
        self.dag[0]['Output_Num'] = 2
        self.dag[0]['all_output'].append(dict(length=64, temp_offset=128, name='second'))
        with self.assertRaisesRegex(ValueError, 'OUTPUT_COUNT_MISMATCH'):
            self.ir([128])

    def test_old_typed_pointer_ir_supported(self):
        self.ir([128])
        source = self.root/'Task_demo.0.ll'
        source.write_text(source.read_text().replace(', ptr inttoptr', ', i32* inttoptr').replace('to ptr)', 'to i32*)'))
        self.assertEqual(check_compiled_returns(self.dag, self.root, self.registers, self.root/'report.json')['status'], 'PASS')
