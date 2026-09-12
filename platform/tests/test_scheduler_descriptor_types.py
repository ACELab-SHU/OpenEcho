from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from ace_echo.adapters.scheduler import SchedulerAdapter


class SchedulerDescriptorTypeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._tmp = tempfile.TemporaryDirectory()
        cls.scheduler_root = Path(cls._tmp.name) / "scheduler-source"
        shutil.copytree(
            Path(__file__).resolve().parents[1] / "components/scheduler",
            cls.scheduler_root,
        )
        SchedulerAdapter._configure_backend_copy(cls.scheduler_root)
        parser_path = cls.scheduler_root / "dags/read_dag_json.py"
        spec = importlib.util.spec_from_file_location(
            "ace_echo_scheduler_read_dag_json", parser_path
        )
        cls.module = importlib.util.module_from_spec(spec)
        assert spec.loader is not None
        spec.loader.exec_module(cls.module)

    @classmethod
    def tearDownClass(cls) -> None:
        cls._tmp.cleanup()

    def test_static_slice_uses_physical_transfer_length(self) -> None:
        self.assertEqual(
            self.module.descriptor_transfer_length({
                "length": "25344", "slice_length": "64"
            }),
            64,
        )
        self.assertEqual(
            self.module.descriptor_transfer_length({"length": "25344"}),
            25344,
        )

    @classmethod
    def output_descriptors(cls, lag: int) -> list[tuple[int, int]]:
        parser = cls.module.DAG_parser(
            "unused.json", output_descriptor_lag_ports=lag
        )
        parser.task_num = 1
        parser.json_data = [{
            "all_output": [
                {"temp_offset": 0, "length": 64},
                {"temp_offset": 64, "length": 64},
                {"temp_offset": 128, "length": 64},
            ]
        }]
        parser._DAG_parser__output_addr_parser()
        payload = b"".join(
            int(value, 16).to_bytes(4, "little")
            for value in parser.output_descriptor_mem
        )
        result = []
        for offset in range(0, 18, 6):
            descriptor = int.from_bytes(payload[offset:offset + 6], "little")
            result.append((descriptor >> 16, descriptor & 0xffff))
        return result

    def test_two_bit_default_preserves_logical_dynamic_ownership(self) -> None:
        self.assertIn(
            self.module.TYPE_PTR_DAG_DFE, self.module.DAG_INPUT_TYPES
        )
        self.assertEqual(self.module.TYPE_DAG_DFE, 2)

    def test_legacy_dynamic_type_maps_to_global_physical_class(self) -> None:
        parser = self.module.DAG_parser(
            "unused.json", task_input_type_bits=2
        )
        value, encoded = parser._DAG_parser__input_type_bin("0b010", 0, 0)
        self.assertEqual(value, 2)
        self.assertEqual(encoded, "01")

    def test_output_descriptor_lag_is_backend_configurable(self) -> None:
        self.assertEqual(
            self.output_descriptors(0),
            [(0, 64), (64, 64), (128, 64)],
        )
        self.assertEqual(
            self.output_descriptors(1),
            [(64, 64), (128, 64), (128, 64)],
        )

    def test_negative_output_descriptor_lag_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "non-negative"):
            self.module.DAG_parser(
                "unused.json", output_descriptor_lag_ports=-1
            )

    def test_output_descriptor_lag_restarts_for_each_task(self) -> None:
        parser = self.module.DAG_parser(
            "unused.json", output_descriptor_lag_ports=1
        )
        parser.task_num = 2
        parser.json_data = [
            {"all_output": [
                {"temp_offset": 0, "length": 64},
                {"temp_offset": 64, "length": 64},
            ]},
            {"all_output": [
                {"temp_offset": 128, "length": 64},
                {"temp_offset": 192, "length": 64},
            ]},
        ]
        parser._DAG_parser__output_addr_parser()
        payload = b"".join(
            int(value, 16).to_bytes(4, "little")
            for value in parser.output_descriptor_mem
        )
        packed = int.from_bytes(payload, "little")

        def descriptor(task: int, port: int) -> tuple[int, int]:
            global_port = task * 16 + port
            bit_offset = (
                (global_port // 10) * 512 + (global_port % 10) * 48
            )
            value = (packed >> bit_offset) & ((1 << 48) - 1)
            return value >> 16, value & 0xffff

        self.assertEqual(descriptor(0, 0), (64, 64))
        self.assertEqual(descriptor(0, 1), (64, 64))
        self.assertEqual(descriptor(1, 0), (192, 64))
        self.assertEqual(descriptor(1, 1), (192, 64))

    def test_rtl_gate_rejects_semantically_unsupported_pointer_input(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dag_json = Path(tmp) / "dag.json"
            dag_json.write_text(json.dumps([{
                "debug_task_name": "Task_consumer",
                "all_input": [{"name": "parent_ptr", "type": "0b100"}],
            }]), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Task_consumer.parent_ptr"):
                SchedulerAdapter._reject_unsupported_logical_types(
                    dag_json, {4, 5, 6}
                )

    def test_rtl_gate_accepts_backend_native_value_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dag_json = Path(tmp) / "dag.json"
            dag_json.write_text(json.dumps([{
                "debug_task_name": "Task_consumer",
                "all_input": [
                    {"name": "parent_value", "type": "0b000"},
                    {"name": "static_value", "type": "0b001"},
                ],
            }]), encoding="utf-8")
            SchedulerAdapter._reject_unsupported_logical_types(
                dag_json, {4, 5, 6}
            )

    def test_rtl_gate_rejects_excess_top_level_dag_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dag_json = Path(tmp) / "dag.json"
            dag_json.write_text(json.dumps([
                {"debug_task_name": "Task_producer", "all_input": []},
                {"return_output": [
                    {"parentTasksPort": f"{index:010b}"}
                    for index in range(9)
                ]},
            ]), encoding="utf-8")
            with self.assertRaisesRegex(
                ValueError, "at most 8.*declares 9"
            ):
                SchedulerAdapter._reject_excess_dag_outputs(dag_json, 8)

    def test_rtl_gate_accepts_eight_top_level_dag_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dag_json = Path(tmp) / "dag.json"
            dag_json.write_text(json.dumps([{"return_output": [
                {"parentTasksPort": f"{index:010b}"}
                for index in range(8)
            ]}]), encoding="utf-8")
            SchedulerAdapter._reject_excess_dag_outputs(dag_json, 8)

    def test_completion_descriptor_precedes_interrupting_dma(self) -> None:
        cluster_source = (
            self.scheduler_root / "src/cluster.c"
        ).read_text(encoding="utf-8")
        collect_start = cluster_source.index("int cluster_collect_outputs")
        collect_end = cluster_source.index(
            "void cluster_interrupt_handler", collect_start
        )
        collect_source = cluster_source[collect_start:collect_end]
        self.assertLess(
            collect_source.index("dmapush_fifo"),
            collect_source.index("dma_transfer"),
            "the last C2D DMA can interrupt immediately, so its owning "
            "descriptor must already be visible",
        )

    def test_static_only_adapter_changes_only_isolated_copy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parser = root / "dags/read_dag_json.py"
            parser.parent.mkdir(parents=True)
            parser.write_text(
                'f"unsigned int {self.dag_name}_input_offset[{self.l1_input_num}] = {{" + ", ".join(map(str, self.input_offset)) + "};\\n"\n'
                'f"unsigned int {self.dag_name}_input_length[{self.l1_input_num}] = {{" + ", ".join(map(str, self.input_length)) + "};\\n"\n'
                "      if self.l1_input_num == 0:\n"
                "        raise ValueError(f'The DAG \"{self.dag_name}\" has no input data!')\n",
                encoding="utf-8",
            )
            SchedulerAdapter._enable_static_only_dag_codegen(root)
            adapted = parser.read_text(encoding="utf-8")
            self.assertIn("max(1, self.l1_input_num)", adapted)
            self.assertNotIn("has no input data", adapted)

    def test_auto_static_main_launches_and_exposes_all_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dag_json = root / "dag.json"
            dag_json.write_text(json.dumps([
                {
                    "debug_task_name": "Task_static",
                    "all_input": [{"name": "table", "type": "0b001"}],
                },
                {"return_output": [
                    {"name": "first"},
                    {"name": "second"},
                ]},
            ]), encoding="utf-8")
            destination = root / "main.c"

            SchedulerAdapter._write_auto_static_main(
                dag_name="static_probe",
                dag_json=dag_json,
                destination=destination,
            )

            source = destination.read_text(encoding="utf-8")
            self.assertIn("fire_dag(static_probe, 0, 2", source)
            self.assertEqual(source.count("static stdata_t ace_echo_output_"), 2)
            self.assertIn("fire_dag_fence();", source)
            self.assertIn("REG_WRITE(0x1fff4000, 0x8);", source)

    def test_auto_static_main_rejects_runtime_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dag_json = root / "dag.json"
            dag_json.write_text(json.dumps([
                {
                    "debug_task_name": "Task_dynamic",
                    "all_input": [{"name": "samples", "type": "0b010"}],
                },
                {"return_output": []},
            ]), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "runtime DAG inputs"):
                SchedulerAdapter._write_auto_static_main(
                    dag_name="dynamic_probe",
                    dag_json=dag_json,
                    destination=root / "main.c",
                )

    def test_dcache_startup_capability_changes_only_isolated_copy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            kernel = root / "src/kernel.c"
            kernel.parent.mkdir(parents=True)
            kernel.write_text(
                "void start_kernel(void) {\n"
                "  change_dcache_end_addr(&_cacheram_end_addr);\n"
                "}\n",
                encoding="utf-8",
            )
            SchedulerAdapter._disable_dcache_end_configuration(root)
            adapted = kernel.read_text(encoding="utf-8")
            self.assertNotIn("change_dcache_end_addr(&", adapted)
            self.assertIn("no qualified dcache-end CSR", adapted)

    def test_pre_spmd_task_container_omits_only_generation_fields(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parser = root / "dags/read_dag_json.py"
            parser.parent.mkdir(parents=True)
            parser.write_text(
                "        packed = (\n"
                "          hardware_info +\n"
                "          is_spmd +\n"
                "          min_core_num +\n"
                "          inputnum +\n"
                "          flattened_task_descriptor_array\n"
                "        )\n",
                encoding="utf-8",
            )

            SchedulerAdapter._omit_task_container_spmd_fields(root)

            adapted = parser.read_text(encoding="utf-8")
            self.assertNotIn("is_spmd +", adapted)
            self.assertNotIn("min_core_num +", adapted)
            self.assertIn("hardware_info +\n          inputnum +", adapted)
            self.assertIn("flattened_task_descriptor_array", adapted)

    def test_devctrl_body_changes_only_isolated_copy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src/peripheral_init.c"
            source.parent.mkdir(parents=True)
            source.write_text(
                "void helper(void) {}\n"
                '__attribute__((optimize("O0"))) void devctrl_init(void) {\n'
                "  old_sequence();\n"
                "}\n",
                encoding="utf-8",
            )
            body = root / "devctrl.inc"
            body.write_text("  new_sequence();\n", encoding="utf-8")

            SchedulerAdapter._install_devctrl_init_body(root, body)

            adapted = source.read_text(encoding="utf-8")
            self.assertIn("void helper(void) {}", adapted)
            self.assertNotIn("old_sequence", adapted)
            self.assertIn("new_sequence", adapted)

    def test_pll_helper_body_changes_only_helper(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src/peripheral_init.c"
            source.parent.mkdir(parents=True)
            source.write_text(
                '__attribute__((optimize("O0"))) void '
                'GC0802_change_pll_settings(uint32_t base_addr, '
                'uint32_t offset_addr, uint32_t target_settings) {\n'
                "  old_protocol();\n"
                "}\n\n"
                '__attribute__((optimize("O0"))) void devctrl_init(void) {\n'
                "  retained_sequence();\n"
                "}\n",
                encoding="utf-8",
            )
            body = root / "pll.inc"
            body.write_text("  v1_protocol();\n", encoding="utf-8")

            SchedulerAdapter._install_pll_helper_body(root, body)

            adapted = source.read_text(encoding="utf-8")
            self.assertNotIn("old_protocol", adapted)
            self.assertIn("v1_protocol", adapted)
            self.assertIn("retained_sequence", adapted)

    def test_v1_ctrl_iopads_are_enabled_only_in_private_copy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "src/peripheral_init.c"
            source.parent.mkdir(parents=True)
            lines = []
            for index in range(4):
                lines.append(
                    "  // CONFIG_GC0802_IOPAD_REG("
                    f"GC0802_IOPAD_CTRL_OUT{index} * 0x4, 0x030);\n"
                )
            for index in range(8):
                lines.append(
                    "  // CONFIG_GC0802_IOPAD_REG("
                    f"GC0802_IOPAD_CTRL_IN{index} * 0x4, 0x039);\n"
                )
            source.write_text("".join(lines), encoding="utf-8")

            SchedulerAdapter._enable_ctrl_iopads(root)

            adapted = source.read_text(encoding="utf-8")
            self.assertNotIn("// CONFIG_GC0802_IOPAD_REG", adapted)
            self.assertEqual(adapted.count("CONFIG_GC0802_IOPAD_REG"), 12)
            self.assertIn("CONFIG_GC0802_IOPAD_REG(0x00, 0x030)", adapted)
            self.assertIn("CONFIG_GC0802_IOPAD_REG(0x2c, 0x039)", adapted)


if __name__ == "__main__":
    unittest.main()
