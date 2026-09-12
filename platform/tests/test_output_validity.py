from __future__ import annotations

from contextlib import redirect_stdout, redirect_stderr
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from ace_echo.output_validity import compare_output_bits, layout_masks, read_layouts
from ace_echo.dag_output_compare import compare_dag_outputs, compare_dma_returns
from ace_echo.cli import main


def layout(n=2, ranges=None):
    return {'schema':'ace-echo-output-layout/v1','transport_bytes':n,
            'valid_bit_ranges':[[0,8]] if ranges is None else ranges}


class OutputValidityTests(unittest.TestCase):
    def test_strict_default(self):
        for actual in ([1,None],[1,'xx'],[1,'zz'],[1,4]):
            self.assertEqual(compare_output_bits([1,0],actual)['status'],'FAIL')
        self.assertEqual(compare_output_bits([1,0],[1,0])['status'],'PASS')

    def test_padding_unknown_warning_not_failure(self):
        r=compare_output_bits([1,0],[1,'xx'],layout())
        self.assertEqual(r['status'],'PASS')
        self.assertEqual(r['transport_bit_exact_status'],'FAIL')
        self.assertEqual(r['warnings'],[{'code':'PADDING_UNKNOWN','bit_count':8}])

    def test_padding_nonzero_is_not_uninitialized(self):
        r=compare_output_bits([1,0],[1,255],layout())
        self.assertEqual(r['status'],'PASS')
        self.assertEqual(r['padding_unknown_bits'],0)
        self.assertEqual({w['code'] for w in r['warnings']},{'PADDING_NONZERO','PADDING_DIFFERENCE'})
        same=compare_output_bits([1,255],[1,255],layout())
        self.assertEqual(same['transport_bit_exact_status'],'PASS')
        self.assertEqual(same['warnings'],[{'code':'PADDING_NONZERO','bit_count':8}])

    def test_valid_error_and_unknown_fail(self):
        for actual in ([2,0],['xx',0],['x1',0]):
            self.assertEqual(compare_output_bits([1,0],actual,layout())['status'],'FAIL')

    def test_partial_byte_unknown_padding_preserves_known_valid_nibble(self):
        for value in ('x5','z5','a5'):
            self.assertEqual(compare_output_bits([5],[value],layout(1,[[0,4]]))['status'],'PASS')
        for value in ('5x','x4'):
            self.assertEqual(compare_output_bits([5],[value],layout(1,[[0,4]]))['status'],'FAIL')

    def test_cross_byte_bit_ranges(self):
        masks=layout_masks(layout(3,[[3,7],[17,1]]))
        self.assertEqual(masks,[248,3,2])
        for bit in range(24):
            data=bytearray(3);data[bit//8]=1<<(bit%8)
            result=compare_output_bits(bytes(3),data,layout(3,[[3,7],[17,1]]))
            self.assertEqual(result['status']=='FAIL',bool(masks[bit//8] & data[bit//8]))

    def test_missing_extra_or_layout_length_mismatch_fail(self):
        for observed in ([],[1],[1,0,0]):
            r=compare_output_bits([1,0],observed,layout())
            self.assertEqual(r['status'],'FAIL')
            self.assertEqual(r['padding_unknown_bits'],0)
        self.assertEqual(compare_output_bits([1,0],[1,0],layout(3))['status'],'FAIL')

    def test_invalid_layout_is_not_an_ignore_all_escape(self):
        for ranges in ([],[[0,0]],[[0,17]],[[-1,1]],[[0,8],[7,1]],[[True,1]],[[8,1],[0,1]]):
            with self.subTest(ranges=ranges),self.assertRaises(ValueError):
                layout_masks(layout(2,ranges))
        for bad in ({},dict(layout(),task_name='domain'),dict(layout(),transport_bytes=True)):
            with self.assertRaises(ValueError): layout_masks(bad)

    @staticmethod
    def trace(task=2, value='xx01', native=False):
        identity=f'ret_source_tile: 0 | task_id: {task}' if native else f'tid: {task} | tname: generic'
        return f'src: 0 | dst: 0 | len: 00000002 | {identity} | retid: 0\ndata:\n{value}\n\n'

    def fixture(self, root):
        ref=root/'ref.log'; ref.write_text(self.trace())
        actual=root/'actual';actual.mkdir()
        (actual/'task_2_port_0.bin').write_bytes(b'\x01\0')
        layouts=root/'layouts.json'
        layouts.write_text(json.dumps({'schema':'ace-echo-output-layouts/v1',
                                      'outputs':[{'task_id':2,'port':0,'layout':layout()}]}))
        return ref,actual,layouts

    def test_dag_opt_in_and_native_headers(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);ref,actual,layouts=self.fixture(root)
            for native in (False,True):
                ref.write_text(self.trace(native=native))
                self.assertEqual(compare_dag_outputs(ref,actual)['status'],'FAIL')
                r=compare_dag_outputs(ref,actual,output_layouts=layouts)
                self.assertEqual(r['status'],'PASS')
                self.assertEqual(r['warnings'][0]['code'],'PADDING_UNKNOWN')
                self.assertEqual(r['transport_bit_exact_status'],'FAIL')
                self.assertFalse(r['bus_assertions_evaluated'])
                self.assertEqual(len(r['layout_sha256']),64)

    def test_dag_unknown_valid_nibble_and_duplicate_guard(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);ref,actual,layouts=self.fixture(root)
            ref.write_text(self.trace(value='00x1'))
            self.assertEqual(compare_dag_outputs(ref,actual,output_layouts=layouts)['status'],'FAIL')
            ref.write_text(self.trace()*2)
            with self.assertRaises(ValueError): compare_dag_outputs(ref,actual,output_layouts=layouts)

    def test_dma_missing_extra_duplicate_and_padding(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);ref,_,layouts=self.fixture(root)
            actual=root/'actual.log';actual.write_text(self.trace(value='0001'))
            self.assertEqual(compare_dma_returns(ref,actual)['status'],'FAIL')
            self.assertEqual(compare_dma_returns(ref,actual,layouts)['status'],'PASS')
            actual.write_text(self.trace(task=3))
            r=compare_dma_returns(ref,actual,layouts)
            self.assertEqual(r['status'],'FAIL')
            self.assertEqual(r['results'][0]['reason'],'missing_return')
            actual.write_text(self.trace()*2)
            with self.assertRaises(ValueError): compare_dma_returns(ref,actual,layouts)

    def test_invalid_output_identity_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);_,_,layouts=self.fixture(root)
            with self.assertRaises(ValueError): read_layouts(layouts,{(3,0)})
            data=json.loads(layouts.read_text()); data['outputs']*=2
            layouts.write_text(json.dumps(data))
            with self.assertRaises(ValueError): read_layouts(layouts,{(2,0)})

    def test_cli_opt_in_raw_and_dma(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);ref,actual,layouts=self.fixture(root)
            p=root/'layout.json';p.write_text(json.dumps(layout()))
            a=root/'a.bin';b=root/'b.bin';a.write_bytes(b'\x01\0');b.write_bytes(b'\x01\xff')
            with patch('ace_echo.cli.selected_config',return_value=(None,None)),redirect_stdout(io.StringIO()),redirect_stderr(io.StringIO()):
                self.assertEqual(main(['compare',str(a),str(b)]),1)
                self.assertEqual(main(['compare',str(a),str(b),'--layout',str(p)]),0)
                self.assertEqual(main(['compare',str(a),str(b),'--layout',str(p),'--dtype','i8']),2)
                self.assertEqual(main(['compare-dag-outputs','--expected-dma',str(ref),'--actual-dir',str(actual),'--output-layouts',str(layouts)]),0)


if __name__ == '__main__':
    unittest.main()
