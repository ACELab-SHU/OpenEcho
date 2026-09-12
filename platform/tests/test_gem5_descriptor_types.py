import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

from ace_echo.adapters.gem5 import Gem5Adapter
from ace_echo.config import load_config
from ace_echo.process import Runner


class Gem5DescriptorTypeTests(unittest.TestCase):
    def test_application_firmware_is_wired_to_gem5_with_private_trace(self):
        repository = Path(__file__).resolve().parents[1]
        config = load_config(repository / "configs/local.toml")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            firmware = root / "l1.elf"
            firmware.write_bytes(b"immutable scheduler fixture")
            runner = Runner(root / "commands", dry_run=True)
            adapter = Gem5Adapter(config, runner)
            output = root / "artifacts" / "gem5"

            adapter.run_application_contract(
                l1_elf=firmware,
                output=output,
                mode="fast",
                timeout=10,
                execute_firmware=True,
                firmware_completion_gpio_mask=0x8,
            )

            command_records = sorted((root / "commands").glob("*.json"))
            decode_record = json.loads(command_records[0].read_text())
            self.assertIn('--all-dags', decode_record['argv'])
            gem5_record = json.loads(command_records[-1].read_text())
            self.assertIn(
                f"--scheduler-firmware-elf={firmware.resolve()}",
                gem5_record["argv"],
            )
            self.assertIn(
                f"--scheduler-firmware-trace={output / 'firmware-trace.jsonl'}",
                gem5_record["argv"],
            )
            self.assertIn(
                "--scheduler-firmware-completion-gpio-mask=0x8",
                gem5_record["argv"],
            )

    @staticmethod
    def load_l1_decoder():
        tool_dir = Path(__file__).parents[1] / "components" / "gem5" / "tools"
        spec = importlib.util.spec_from_file_location(
            "venus_l1_dag_under_test", tool_dir / "venus_l1_dag.py"
        )
        module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        sys.path.insert(0, str(tool_dir))
        try:
            spec.loader.exec_module(module)
        finally:
            sys.path.pop(0)
        return module

    def test_l1_decoder_respects_physical_descriptor_type_width(self):
        module = self.load_l1_decoder()

        for type_bits in (2, 3):
            descriptor_width = 42 + type_bits
            value = 0
            first_descriptor = (
                0x12345678 | (3 << 32) | (17 << 36) |
                (((1 << type_bits) - 1) << 42)
            )
            value |= first_descriptor
            offset = 64 * descriptor_width

            fields = (
                (1, 7), (5, 6), (1, 1), (0x2aa, 11),
                (0x12345, 25), (0x23456, 25),
                (0x34567, 22), (0x25678, 22), (0xabcd, 16),
            )
            for field_value, width in fields:
                value |= field_value << offset
                offset += width

            task = module.decode_task(
                value.to_bytes(384, "little"), 0, type_bits
            )
            self.assertEqual(task["inputs"][0]["destination"], 0x12345678)
            self.assertEqual(task["inputs"][0]["output_port"], 3)
            self.assertEqual(task["inputs"][0]["parent_task"], 17)
            self.assertEqual(task["inputs"][0]["type"], (1 << type_bits) - 1)
            self.assertEqual(task["hardware_requirement"], 0x2aa)
            self.assertEqual(task["data_length"], 0x12345)
            self.assertEqual(task["data_address"], 0x23456)
            self.assertEqual(task["code_length"], 0x34567)
            self.assertEqual(task["code_address"], 0x25678)
            self.assertEqual(task["crc"], 0xabcd)

    def test_l1_decoder_supports_pre_spmd_task_container(self):
        module = self.load_l1_decoder()
        descriptor_width = 44
        value = 0
        offset = 64 * descriptor_width
        fields = (
            (0, 7), (0x2aa, 11), (0x12345, 25), (0x23456, 25),
            (0x34567, 22), (0x25678, 22), (0xabcd, 16),
        )
        for field_value, width in fields:
            value |= field_value << offset
            offset += width

        task = module.decode_task(
            value.to_bytes(384, "little"), 0, 2,
            task_container_spmd_fields=False,
        )

        self.assertEqual(task["need_spmd"], 0)
        self.assertEqual(task["minimum_spmd_tasks"], 1)
        self.assertEqual(task["hardware_requirement"], 0x2aa)
        self.assertEqual(task["data_length"], 0x12345)
        self.assertEqual(task["data_address"], 0x23456)
        self.assertEqual(task["code_length"], 0x34567)
        self.assertEqual(task["code_address"], 0x25678)
        self.assertEqual(task["crc"], 0xabcd)

    def test_application_decoder_receives_pre_spmd_backend_flag(self):
        repository = Path(__file__).resolve().parents[1]
        config = load_config(repository / "configs/local.toml")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            firmware = root / "l1.elf"
            firmware.write_bytes(b"immutable scheduler fixture")
            runner = Runner(root / "commands", dry_run=True)
            adapter = Gem5Adapter(config, runner)
            adapter.run_application_contract(
                l1_elf=firmware,
                output=root / "gem5",
                mode="fast",
                timeout=10,
                task_container_spmd_fields=False,
            )
            decoder_record = json.loads(
                sorted((root / "commands").glob("*.json"))[0].read_text()
            )
            self.assertIn(
                "--task-container-without-spmd-fields",
                decoder_record["argv"],
            )

    def test_hydration_supports_static_and_dynamic_external_inputs(self):
        descriptor = [{
            "current_taskId": 1,
            "all_input": [
                {"name": "static_value", "type": "0b001", "offset": 0, "length": 2},
                {"name": "static_pointer", "type": "0b101", "offset": 2, "length": 2},
                {"name": "dynamic_value", "type": "0b010", "offset": 4, "length": 2},
                {"name": "dynamic_pointer", "type": "0b110", "offset": 6, "length": 2},
                {"name": "static_slice", "type": "0b001", "offset": 0,
                 "length": 8, "slice_length": "2"},
                {"name": "value_dependency", "type": "0b000"},
                {"name": "pointer_dependency", "type": "0b100"},
            ],
        }]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dag_json = root / "dag.json"
            dag_bin = root / "dag.bin"
            hydrated = root / "hydrated.json"
            dag_json.write_text(json.dumps(descriptor), encoding="utf-8")
            dag_bin.write_bytes(b"\x11\x22\x33\x44\x55\x66\x77\x88")

            adapter = object.__new__(Gem5Adapter)
            adapter._hydrate_descriptor(dag_json, dag_bin, hydrated)
            inputs = json.loads(hydrated.read_text(encoding="utf-8"))[0]["all_input"]

        self.assertEqual(inputs[0]["data"], "0x2211")
        self.assertEqual(inputs[1]["data"], "0x4433")
        self.assertEqual(inputs[2]["data"], "0x6655")
        self.assertEqual(inputs[3]["data"], "0x8877")
        self.assertEqual(inputs[4]["data"], "0x2211")
        self.assertNotIn("data", inputs[5])
        self.assertNotIn("data", inputs[6])

    def test_runtime_parser_accepts_current_and_legacy_dependency_widths(self):
        tool_path = (Path(__file__).parents[1] / "components" / "gem5" /
                     "tools" / "venus_dag.py")
        spec = importlib.util.spec_from_file_location("venus_dag_under_test", tool_path)
        module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(module)

        for type_value in ("0b00", "0b000", "0b100"):
            self.assertEqual(
                module.parse_parent({"type": type_value, "parentTasksPort": "0b0010000011"}),
                (0b001000, 0b0011),
            )
        for type_value in ("0b001", "0b010", "0b101", "0b110"):
            self.assertIsNone(module.parse_parent({"type": type_value}))

    def test_dependency_length_falls_back_to_parent_output(self):
        tool_path = (Path(__file__).parents[1] / "components" / "gem5" /
                     "tools" / "venus_dag.py")
        spec = importlib.util.spec_from_file_location("venus_dag_length_test", tool_path)
        module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(module)

        tasks = [{"all_output": [{"length": "1024"}]}]
        self.assertEqual(
            module.dependency_input_length(tasks, {"name": "tmp"}, (0, 0)),
            1024,
        )
        self.assertEqual(
            module.dependency_input_length(
                tasks, {"name": "slice", "length": "128"}, (0, 0)),
            128,
        )

        pointer = module.packed_dmt_pointer(1024, 0x01002000)
        self.assertEqual(len(pointer), 64)
        self.assertEqual(int.from_bytes(pointer[0:2], "little"), 1024)
        self.assertEqual(int.from_bytes(pointer[2:6], "little"), 0x01002000)

    def test_runtime_decoder_uses_slice_length(self):
        tool_path = (Path(__file__).parents[1] / "components" / "gem5" /
                     "tools" / "venus_dag.py")
        spec = importlib.util.spec_from_file_location("venus_dag_slice_test", tool_path)
        module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(module)

        self.assertEqual(
            module.decode_input_data({
                "name": "static_slice", "data": "0x2211", "length": 8,
                "slice_length": "2",
            }),
            b"\x11\x22",
        )

    def test_legacy_dmt_slots_are_aligned_and_share_parent_port(self):
        tool_path = (Path(__file__).parents[1] / "components" / "gem5" /
                     "tools" / "venus_dag.py")
        spec = importlib.util.spec_from_file_location("venus_dag_slot_test", tool_path)
        module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(module)

        tasks = [
            {"all_output": [{"length": "100"}, {"length": "65"}]},
            {"all_output": [], "all_input": [
                {"type": "0b000", "parentTasksPort": "0", "length": "80"},
                {"type": "0b100", "parentTasksPort": "1", "length": "65"},
            ]},
        ]
        slots = module.build_legacy_dmt_slots(tasks, base=0x1000)
        self.assertEqual(slots[(0, 0)], {"address": 0x1000, "capacity": 100})
        self.assertEqual(slots[(0, 1)], {"address": 0x1080, "capacity": 65})


if __name__ == "__main__":
    unittest.main()
