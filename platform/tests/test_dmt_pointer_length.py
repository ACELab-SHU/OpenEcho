import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "venus_dag_pointer_length_test", ROOT / "components/gem5/tools/venus_dag.py")
dag = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dag)


class DmtPointerLengthTests(unittest.TestCase):
    def test_large_backing_span_uses_explicit_length_not_capacity(self):
        descriptor = {"name": "workspace", "offset": 59968, "length": 64}
        result = dag.static_pointer_payload(descriptor, {59968: 233216})
        # Independently frozen Scheduler DMT word from Venus_3.
        self.assertEqual(result, bytes.fromhex("400040ea0000") + bytes(58))

    def test_length_is_not_forced_to_transport_size(self):
        for length in (1, 31, 64, 1024, 65535):
            with self.subTest(length=length):
                payload = dag.static_pointer_payload(
                    {"offset": 128, "length": str(length)}, {128: 300000})
                self.assertEqual(len(payload), 64)
                self.assertEqual(int.from_bytes(payload[:2], "little"), length)
                self.assertEqual(int.from_bytes(payload[2:6], "little"), 128)
                self.assertEqual(payload[6:], bytes(58))

    def test_bad_declared_lengths_are_not_replaced_or_truncated(self):
        for value in (None, 0, -1, 65536, 233216, True, 64.5, "64.5", "bad"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                dag.static_pointer_payload(
                    {"offset": 0, "length": value}, {0: 300000})
        with self.assertRaisesRegex(ValueError, "explicit integer length"):
            dag.static_pointer_payload({"offset": 0}, {0: 300000})

    def test_bad_offsets_and_unknown_backing_are_rejected(self):
        for value in (None, -1, 2**32, True, 1.5, "bad", 129):
            with self.subTest(value=value), self.assertRaises(ValueError):
                dag.static_pointer_payload(
                    {"offset": value, "length": 64}, {128: 256})

    def test_span_bounds_remain_independent(self):
        for capacity in (0, -1, 63):
            with self.subTest(capacity=capacity), self.assertRaisesRegex(
                    ValueError, "exceeds backing span"):
                dag.static_pointer_payload(
                    {"offset": 128, "length": 64}, {128: capacity})
        self.assertEqual(len(dag.static_pointer_payload(
            {"offset": 128, "length": 64}, {128: 64})), 64)

    def test_static_spans_validate_image_bounds(self):
        def tasks(offsets):
            return [{"all_input": [
                {"type": "0b101", "offset": off} for off in offsets]}]
        self.assertEqual(dag.static_allocation_capacities(
            tasks([128, 128, 192]), 256), {128: 64, 192: 64})
        for offsets in ([-1, 128], [128, 256], [128, 300], [256]):
            with self.subTest(offsets=offsets), self.assertRaisesRegex(
                    ValueError, "outside combined image"):
                dag.static_allocation_capacities(tasks(offsets), 256)

    def test_manifest_types_5_and_6_pack_declared_lengths_and_keep_image(self):
        for kind in (5, 6):
            with self.subTest(kind=kind), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                binary = root / "dag.bin"
                contents = bytes(233216 + 128)
                binary.write_bytes(contents)
                inputs = [
                    {"name": "workspace", "type": bin(kind), "offset": 128,
                     "length": 64, "dest_address": "0x22000"},
                    # Value and pointer aliases do not shrink the backing span.
                    {"name": "header", "type": "0b001", "offset": 128,
                     "length": 2, "data": "0x2211", "dest_address": "0x22040"},
                ]
                tasks = [{"current_taskId": 0, "debug_task_name": "fixture",
                          "all_input": inputs, "all_output": []}]
                with patch.object(dag, "materialize_task", return_value=root / "task.elf"), \
                     patch.object(dag, "materialize_scheduler_images",
                                  return_value=(root / "code.bin", root / "data.bin")):
                    manifest_path = dag.build_inprocess_manifest(
                        tasks, binary, root, root / "run")
                manifest = json.loads(manifest_path.read_text())
                payload = Path(manifest["initial_inputs"][0]["file"]).read_bytes()
                self.assertEqual(payload, bytes.fromhex("400080000000") + bytes(58))
                self.assertEqual(Path(manifest["initial_inputs"][1]["file"]).read_bytes(),
                                 bytes.fromhex("1122"))
                self.assertEqual(Path(manifest["shared_l2_image"]).read_bytes(), contents)
                record = manifest["static_pointer_records"][0]
                self.assertEqual(record["record_length_bytes"], 64)
                self.assertEqual(record["backing_span_bytes"], 233216)
                self.assertEqual(record["transport_bytes"], 64)
                self.assertEqual(record["type"], kind)

    def test_dependency_pointer_path_unchanged(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            binary = root / "dag.bin"
            binary.write_bytes(bytes(64))
            tasks = [
                {"current_taskId": 0, "debug_task_name": "producer",
                 "all_input": [], "all_output": [{"length": 1024}]},
                {"current_taskId": 1, "debug_task_name": "consumer",
                 "all_input": [{"name": "temporary", "type": "0b100",
                                "parentTasksPort": "0", "length": 1024,
                                "dest_address": "0x22000"}], "all_output": []},
            ]
            with patch.object(dag, "materialize_task", return_value=root / "task.elf"), \
                 patch.object(dag, "materialize_scheduler_images",
                              return_value=(root / "code.bin", root / "data.bin")):
                path = dag.build_inprocess_manifest(tasks, binary, root, root / "run")
            manifest = json.loads(path.read_text())
            dependency = manifest["dependency_inputs"][0]
            self.assertEqual(dependency["length"], 64)
            self.assertEqual(dependency["slot_consumer_bytes"], 1024)
            self.assertEqual(Path(dependency["pointer_file"]).read_bytes(),
                             bytes.fromhex("000400000001") + bytes(58))
            self.assertEqual(manifest["static_pointer_records"], [])


if __name__ == "__main__":
    unittest.main()
