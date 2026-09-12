import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'script'))
from final_json_input import bind_consumer_lengths


class ConsumerLengthTests(unittest.TestCase):
    def fixture(self):
        producer = {'current_taskId': 0, 'debug_task_name': 'producer',
                    'all_output': [{'length': 15600, 'allocation_bytes': 15616}]}
        edge = {'name': 'soft', 'type': '0b000', 'dest_address': '0x101a00',
                'parentTasksPort': '0b0000000000'}
        consumer = {'current_taskId': 1, 'debug_task_name': 'decode*2', 'all_input': [edge]}
        bindings = {'producer': {'input': {}}, 'decode*2': {'input': {
            'different_bas_name': {'index': 1, 'dest_address': '0x101a00'},
            'first': {'index': 0, 'dest_address': '0x100000'}}}}
        signatures = {'producer': {'args': []}, 'decode': {'args': [
            {'type': '__v16i16'}, {'type': '__v6148i8'}]}}
        return [producer, consumer], bindings, signatures

    def test_producer_capacity_and_consumer_address_are_preserved(self):
        dag, bindings, signatures = self.fixture()
        original = copy.deepcopy(dag[0])
        bind_consumer_lengths(dag, bindings, signatures)
        edge = dag[1]['all_input'][0]
        self.assertEqual(edge['length'], 6148)
        self.assertEqual(edge['consumer_arg_index'], 1)
        self.assertEqual(edge['dest_address'], '0x101a00')
        self.assertEqual(dag[0], original)

    def test_i16_uses_bytes_and_does_not_expand_small_producer(self):
        dag, bindings, signatures = self.fixture()
        signatures['decode']['args'][1]['type'] = '__v6148i16'
        dag[0]['all_output'][0]['length'] = 336
        bind_consumer_lengths(dag, bindings, signatures)
        self.assertEqual(dag[1]['all_input'][0]['consumer_capacity_bytes'], 12296)
        self.assertEqual(dag[1]['all_input'][0]['length'], 336)

    def test_pointer_slice_and_concat_contracts_are_unchanged(self):
        for extra in ({'type': '0b100', 'length': 64}, {'slice_length': '12'}, {'concat_value': 1}):
            dag, bindings, signatures = self.fixture()
            dag[1]['all_input'][0].update(extra)
            before = copy.deepcopy(dag)
            bind_consumer_lengths(dag, bindings, signatures)
            self.assertEqual(dag, before)

    def test_ambiguous_binding_warns_without_changing_transport(self):
        dag, bindings, signatures = self.fixture()
        bindings['decode*2']['input']['first']['dest_address'] = '0x101a00'
        with self.assertWarnsRegex(UserWarning, 'ambiguous'):
            bind_consumer_lengths(dag, bindings, signatures)
