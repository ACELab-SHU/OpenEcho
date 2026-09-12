"""No ISA emulation: validate declared input backing and live firmware startup."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1] / 'components/gem5/tools'
sys.path.insert(0, str(TOOLS))
import venus_l1_dag as decoder
import venus_firmware_registry as registry


class DynamicInputTests(unittest.TestCase):
    def runtime(self, kind=6, start=16, length=4, payload=b'ABCD'):
        return {'blob':b'CODE'+bytes(28),
                'tasks':[{'id':0,'code_address':0,'code_length':4,
                          'data_address':4,'data_length':4,
                          'inputs':[{'type':kind,'parent_task':0,'output_port':1,
                                     'destination':0x22000,'index':0}]}],
                'globals':[{'source':start,'length':length}],
                'l2_backing_inputs':[{'address':start,'payload':payload}]}

    def test_dynamic_value_and_pointer_backing_inside_or_outside_blob(self):
        for kind in (2,6):
            for start in (16,64):
                with self.subTest(kind=kind,start=start):
                    r=self.runtime(kind,start)
                    image=decoder.materialize_shared_l2(r)
                    self.assertEqual(image[start:start+4],b'ABCD')
                    self.assertEqual(image[:4],b'CODE')

    def test_pointer_dma_record_is_not_backing_payload(self):
        r=self.runtime(6,64,64,b'A'*64)
        r['dma_records']=[{'task':0,'destination':0x22000,'payload':b'P'*64}]
        self.assertEqual(decoder.materialize_shared_l2(r)[64:], b'A'*64)
        r['l2_backing_inputs']=[]
        with self.assertRaisesRegex(ValueError,'outside the canonical'):
            decoder.materialize_shared_l2(r)

    def test_static_pointer_can_use_missing_backing_but_not_change_canonical(self):
        self.assertEqual(decoder.materialize_shared_l2(self.runtime(5,64))[64:],b'ABCD')
        with self.assertRaisesRegex(ValueError,'static pointer backing'):
            decoder.materialize_shared_l2(self.runtime(5,16))

    def test_missing_short_wrong_address_and_conflicting_payloads_rejected(self):
        cases=[[],[{'address':64,'payload':b'ABC'}],
               [{'address':65,'payload':b'ABCD'}],
               [{'address':64,'payload':b'ABCD'}, {'address':64,'payload':b'WXYZ'}]]
        for entries in cases:
            with self.subTest(entries=entries),self.assertRaises(ValueError):
                r=self.runtime(6,64)
                r['l2_backing_inputs']=entries
                decoder.materialize_shared_l2(r)

    def test_declared_input_cannot_overwrite_code_or_task_data(self):
        for kind in (2,6):
            for start in (0,4):
                with self.subTest(kind=kind,start=start), self.assertRaisesRegex(ValueError,'overwrites task'):
                    decoder.materialize_shared_l2(self.runtime(kind,start))

    def test_undeclared_canonical_overwrite_rejected(self):
        r=self.runtime()
        r['tasks'][0]['inputs']=[]
        with self.assertRaisesRegex(ValueError,'without an identical declared'):
            decoder.materialize_shared_l2(r)

    def test_oversized_fixture_cannot_expand_declared_overwrite(self):
        r=self.runtime(payload=b'ABCDEFGH')
        with self.assertRaisesRegex(ValueError,'without an identical declared'):
            decoder.materialize_shared_l2(r)

    def test_invalid_global_range(self):
        for start,length in ((-1,4),(16,0),(16,65536)):
            with self.subTest(start=start,length=length), self.assertRaisesRegex(ValueError,'invalid range'):
                decoder.materialize_shared_l2(self.runtime(start=start,length=length))

    def test_pointer_word_still_uses_declared_length_and_64_byte_transport(self):
        r=self.runtime(6,64,1024,b'A'*1024)
        task=r['tasks'][0]
        task.update(hardware_requirement=0,crc=1)
        image, _, inputs=decoder.build_task_image(task,decoder.materialize_shared_l2(r),r['globals'])
        self.assertEqual(len(inputs[0]['payload']),64)
        self.assertEqual(int.from_bytes(inputs[0]['payload'],'little'),(64<<16)|1024)

    def test_live_task_scaffold_does_not_seed_or_require_dynamic_payload(self):
        r=self.runtime(6,65536)
        task=r['tasks'][0]
        task.update(hardware_requirement=0,crc=1)
        image, _, inputs=decoder.build_task_image(task,r['blob'],r['globals'],live_firmware_inputs=True)
        self.assertEqual(inputs,[])
        self.assertEqual(image[:4],b'CODE')
        with self.assertRaisesRegex(ValueError,'outside its replay image'):
            decoder.build_task_image(task,r['blob'],r['globals'])

    def test_live_manifest_cannot_be_replayed_as_contract(self):
        manifest={'input_materialization':'live-firmware-dma'}
        registry.validate_input_execution_mode(manifest,True)
        with self.assertRaisesRegex(ValueError,'require the firmware'):
            registry.validate_input_execution_mode(manifest,False)
        for key in ('initial_inputs','l2_backing_inputs'):
            with self.assertRaisesRegex(ValueError,'must not preload'):
                registry.validate_input_execution_mode(dict(manifest,**{key:[{}]}),True)
        registry.validate_input_execution_mode({},False)

    def test_live_materializer_refuses_captures(self):
        for key in ('dma_records','dma_transactions','l2_backing_inputs','rtl_lsu_sparse_bytes'):
            with self.subTest(key=key),self.assertRaisesRegex(ValueError,'cannot use'):
                decoder.materialize({key:[1]},Path('unused'),live_firmware_inputs=True)

    def test_registry_selects_live_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            manifest=root/'venus_dag_manifest.json'
            def fake_materialize(runtime,destination,**kwargs):
                self.assertEqual(kwargs,{'live_firmware_inputs':True})
                manifest.write_text('{}')
                return manifest
            class Elf:
                symbols={'dag_task_container':None}
            with patch.object(decoder,'Elf32',return_value=Elf()), \
                 patch.object(decoder,'decode_runtime_dag',return_value={}), \
                 patch.object(decoder,'materialize',side_effect=fake_materialize), \
                 patch.object(registry,'attach_identity'):
                registry.materialize_registry(root/'l1.elf',None,root,3,True)


if __name__=='__main__':
    unittest.main()
