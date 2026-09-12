from pathlib import Path
import json
import tempfile
import unittest

from ace_echo.rtl_return_identity import resolve_return_identity
from ace_echo.dag_output_compare import compare_dag_outputs


class ReturnIdentityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.dma, self.log, self.dag = (self.root/n for n in ('dma.txt','sim.log','dag.json'))
        self.contract = dict(protocol='venus-serial-ack-v1',tile_address_base='0x1000',tile_address_stride='0x100',tile_count=2)
        self.dag.write_text(json.dumps([
            dict(current_taskId=5, Output_Num=2, all_output=[dict(length=1,temp_offset=0),dict(length=1,temp_offset=64)]),
            dict(current_taskId=9, Output_Num=1, all_output=[dict(length=1,temp_offset=128)])]))
        self.log.write_text('[10 ns] arbiter recycle task: dag 2 task 5 processing result received from tile 0. data_count: 1\n'
            '[30 ns] arbiter recycle task: dag 2 task 5 processing result received from tile 0. data_count: 2\n'
            '[70 ns] arbiter recycle task: dag 2 task 9 processing result received from tile 1. data_count: 1\nTest complete!\n')
        self.dma.write_text('src: 1010 | dst: 8000 | len: 1 | ret_source_tile: 0 | task_id: 5 | retid: 0 | time: 20 ns\ndata:\n01\n\n'
            'src: 1020 | dst: 8040 | len: 1 | ret_source_tile: 1 | task_id: 9 | retid: 0 | time: 65 ns\ndata:\n02\n\n'
            'src: 1110 | dst: 8080 | len: 1 | ret_source_tile: 1 | task_id: 9 | retid: 0 | time: 75 ns\ndata:\n03\n')

    def resolve(self):
        return resolve_return_identity(self.dma,self.log,self.dag,self.contract)

    def test_dynamic_arbitration_label_is_reconciled_without_fixed_delay(self):
        records, report = self.resolve()
        self.assertEqual([(r['task_id'],r['port']) for r in records],[(5,0),(5,1),(9,0)])
        self.assertEqual(report['corrected_labels'],1)
        self.assertFalse(report['payload_used_for_identity'])
        self.assertEqual(records[1]['values'],[2])

    def test_payload_differences_cannot_select_an_identity_or_pass(self):
        for task,port,data in ((5,0,b'\x01'),(5,1,b'\xff'),(9,0,b'\x03')):
            (self.root/f'task_{task}_port_{port}.bin').write_bytes(data)
        report=compare_dag_outputs(self.dma,self.root,rtl_log=self.log,dag_json=self.dag,identity_contract=self.contract)
        self.assertEqual(report['status'],'FAIL')
        self.assertEqual(report['identity']['corrected_labels'],1)

    def test_omitted_reconciliation_remains_strict(self):
        with self.assertRaisesRegex(ValueError,'duplicate'):
            compare_dag_outputs(self.dma,self.root)

    def test_missing_request_is_rejected(self):
        self.log.write_text(self.log.read_text().replace('[30 ns]','[21 ns]').replace('task 5 processing result received from tile 0. data_count: 2','task 5 processing result received from tile 0. data_count: 1'))
        with self.assertRaises(ValueError): self.resolve()

    def test_missing_payload_is_rejected(self):
        self.dma.write_text(self.dma.read_text().split('src: 1110')[0])
        with self.assertRaisesRegex(ValueError,'coverage'): self.resolve()

    def test_wrong_tile_address_is_rejected(self):
        self.dma.write_text(self.dma.read_text().replace('src: 1020','src: 1120'))
        with self.assertRaisesRegex(ValueError,'source address'): self.resolve()

    def test_wrong_destination_is_rejected(self):
        self.dma.write_text(self.dma.read_text().replace('dst: 8040','dst: 8041'))
        with self.assertRaisesRegex(ValueError,'destinations'): self.resolve()

    def test_wrong_length_is_rejected(self):
        self.dag.write_text(self.dag.read_text().replace('"length": 1','"length": 0'))
        with self.assertRaisesRegex(ValueError,'length'): self.resolve()

    def test_runtime_length_may_be_less_than_slot_capacity(self):
        self.dag.write_text(self.dag.read_text().replace('"length": 1','"length": 2'))
        self.assertEqual(self.resolve()[1]['status'],'PASS')

    def test_explicit_allocation_capacity_not_semantic_length(self):
        dag = json.loads(self.dag.read_text())
        dag[0]['all_output'][0]['allocation_bytes'] = 64
        self.dag.write_text(json.dumps(dag))
        self.dma.write_text(self.dma.read_text().replace('len: 1 | ret_source_tile: 0', 'len: 2 | ret_source_tile: 0').replace('data:\n01\n', 'data:\n0102\n'))
        self.assertEqual(self.resolve()[1]['status'], 'PASS')

    def test_allocation_capacity_does_not_waive_real_overrun(self):
        dag = json.loads(self.dag.read_text())
        dag[0]['all_output'][0]['allocation_bytes'] = 1
        self.dag.write_text(json.dumps(dag))
        self.dma.write_text(self.dma.read_text().replace('len: 1 | ret_source_tile: 0', 'len: 2 | ret_source_tile: 0').replace('data:\n01\n', 'data:\n0102\n'))
        with self.assertRaisesRegex(ValueError, 'exceeds compiled output capacity'):
            self.resolve()

    def test_unfinished_simulation_is_rejected(self):
        self.log.write_text(self.log.read_text().replace('Test complete!',''))
        with self.assertRaisesRegex(ValueError,'completed'): self.resolve()

    def test_timestamp_units_are_exact(self):
        self.dma.write_text(self.dma.read_text().replace('20 ns','20000 ps'))
        self.assertEqual(self.resolve()[1]['status'],'PASS')

    def test_ambiguous_same_timestamp_is_rejected(self):
        self.log.write_text(self.log.read_text().replace('[30 ns]','[10 ns]'))
        with self.assertRaisesRegex(ValueError,'ambiguous'): self.resolve()

    def test_multi_dag_scope_is_not_silently_collapsed(self):
        self.log.write_text(self.log.read_text().replace('dag 2 task 9','dag 3 task 9'))
        with self.assertRaisesRegex(ValueError,'coverage'): self.resolve()

    def test_duplicate_start_is_rejected(self):
        self.dma.write_text(self.dma.read_text().replace('time: 65 ns','time: 20 ns'))
        with self.assertRaisesRegex(ValueError,'unordered'): self.resolve()

    def test_uart_binary_bytes_do_not_change_ascii_events(self):
        self.log.write_bytes(b'UART: \xff\xfe\x80\n'+self.log.read_bytes())
        self.assertEqual(self.resolve()[1]['outputs'],3)
