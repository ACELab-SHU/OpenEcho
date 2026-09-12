import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from ace_echo.output_capacity import check_observed_returns


class ConsumerCapacityTests(unittest.TestCase):
    def test_real_return_must_fit_even_when_producer_allocation_is_safe(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dag = [{'current_taskId': 0, 'Output_Num': 1,
                    'all_output': [{'length': 15600, 'allocation_bytes': 15616}]},
                   {'current_taskId': 1, 'Output_Num': 0, 'all_output': [], 'all_input': [
                    {'name': 'soft', 'type': '0b000', 'parentTasksPort': '0b0',
                     'length': 6148, 'consumer_capacity_bytes': 6148}]}]
            path = root / 'dag.json'
            path.write_text(json.dumps(dag))
            payload = root / 'task_0_port_0.bin'
            payload.write_bytes(bytes(172))
            self.assertEqual(check_observed_returns(path, root, root/'pass.json')['status'], 'PASS')
            payload.write_bytes(bytes(6149))
            with self.assertRaisesRegex(ValueError, 'INPUT_PAYLOAD_EXCEEDS_TRANSFER'):
                check_observed_returns(path, root, root/'fail.json')
            report = json.loads((root/'fail.json').read_text())
            self.assertEqual(report['status'], 'FAIL')

    def test_old_small_producer_bound_cannot_silently_truncate(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dag = [{'current_taskId': 0, 'Output_Num': 1,
                    'all_output': [{'length': 176, 'allocation_bytes': 512}]},
                   {'current_taskId': 1, 'Output_Num': 0, 'all_output': [], 'all_input': [
                    {'type': '0b000', 'parentTasksPort': '0b0', 'length': 176,
                     'consumer_capacity_bytes': 12296}]}]
            (root/'dag.json').write_text(json.dumps(dag))
            (root/'task_0_port_0.bin').write_bytes(bytes(336))
            with self.assertRaisesRegex(ValueError, 'INPUT_PAYLOAD_EXCEEDS_TRANSFER'):
                check_observed_returns(root/'dag.json', root, root/'report.json')

    def test_runtime_dmt_ignores_static_transfer_hint_and_warns(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dag = [{'current_taskId': 0, 'Output_Num': 1,
                    'all_output': [{'length': 128, 'allocation_bytes': 128}]},
                   {'current_taskId': 1, 'Output_Num': 0, 'all_output': [], 'all_input': [
                    {'name': 'soft', 'type': '0b000', 'parentTasksPort': '0b0',
                     'length': 64, 'consumer_capacity_bytes': 128}]}]
            path = root / 'dag.json'
            path.write_text(json.dumps(dag))
            payload = root / 'task_0_port_0.bin'
            payload.write_bytes(bytes(172))
            report = check_observed_returns(path, root, root/'report.json', runtime_dmt=True)
            self.assertEqual(report['status'], 'PASS')
            self.assertEqual(report['dependency_length_source'], 'runtime_dmt_16bit')
            self.assertEqual({x['code'] for x in report['diagnostics']},
                {'OUTPUT_EXCEEDS_STATIC_ESTIMATE', 'INPUT_EXCEEDS_STATIC_CAPACITY'})
            self.assertTrue(all(x['severity']=='WARNING' for x in report['diagnostics']))
            self.assertEqual(payload.stat().st_size, 172)

    def test_missing_real_output_still_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'dag.json').write_text(json.dumps([{'current_taskId':0,
                'Output_Num':1, 'all_output':[{'length':128}]}]))
            with self.assertRaisesRegex(ValueError, 'OUTPUT_MISSING'):
                check_observed_returns(root/'dag.json', root, root/'report.json', runtime_dmt=True)

    def test_advisory_length_does_not_block_materialization(self):
        path = Path(__file__).parents[1] / 'components/gem5/tools/venus_dag.py'
        spec = importlib.util.spec_from_file_location('consumer_dag_test', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for length in (None, 0, -1, 6149, '6148'):
            with self.assertWarns(UserWarning):
                self.assertEqual(module.dependency_input_length([], {'name': 'soft',
                    'consumer_capacity_bytes': 6148, 'length': length}, (0, 0)),6148)
