from __future__ import annotations

from pathlib import Path
import shutil
import tempfile
import unittest
from unittest import mock

from ace_echo.adapters.rtl import RtlAdapter, RtlCase


class RtlAdapterTests(unittest.TestCase):
    def test_overlay_can_copy_only_declared_leaf_subdirectories(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "generated-ip"
            target = root / "snapshot-ip"
            (source / "core_a/src").mkdir(parents=True)
            (source / "core_a/src/core.v").write_text(
                "module core; endmodule\n", encoding="utf-8"
            )
            (source / "core_a/syn").mkdir()
            (source / "core_a/syn/large.ddc").write_bytes(b"unused")
            (source / "core_b/src").mkdir(parents=True)
            (source / "core_b/src/files.lst").write_text(
                "core_b.v\n", encoding="utf-8"
            )

            RtlAdapter._copy_overlay_leaf_subdirectories(
                source, target, ["src"]
            )

            self.assertTrue((target / "core_a/src/core.v").is_file())
            self.assertTrue((target / "core_b/src/files.lst").is_file())
            self.assertFalse((target / "core_a/syn").exists())

    def test_snapshot_excludes_match_relative_paths(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "rtl"
            destination = Path(temporary) / "snapshot"
            (root / ".git").mkdir(parents=True)
            (root / ".git/config").write_text("ignored", encoding="utf-8")
            (root / "sim/build_stale").mkdir(parents=True)
            (root / "sim/build_stale/simv").write_text(
                "stale", encoding="utf-8"
            )
            (root / "sim/testbenches").mkdir(parents=True)
            (root / "sim/testbenches/test.sv").write_text(
                "kept", encoding="utf-8"
            )
            shutil_ignore = RtlAdapter._snapshot_ignore(
                root, [".git", "sim/build_*"]
            )
            shutil.copytree(root, destination, ignore=shutil_ignore)
            self.assertFalse((destination / ".git").exists())
            self.assertFalse((destination / "sim/build_stale").exists())
            self.assertTrue(
                (destination / "sim/testbenches/test.sv").is_file()
            )

    def test_run_local_ucli_horizon_override(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            snapshot = Path(tmp)
            script = snapshot / "sim/simv_ucli.tcl"
            script.parent.mkdir(parents=True)
            script.write_text(
                "unsuppress_message WARNING\nrun 1000000us\nexit\n",
                encoding="utf-8",
            )
            RtlAdapter._set_sim_horizon(snapshot, 20000)
            self.assertIn("run 20000us", script.read_text(encoding="utf-8"))

    def test_run_local_task_boundary_trace_is_manifest_declared(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            script = snapshot / "sim/simv_ucli.tcl"
            script.parent.mkdir(parents=True)
            script.write_text("run 10us\nexit\n", encoding="utf-8")
            adapter.flow["task_boundary_trace"] = {
                "ucli_script": "sim/simv_ucli.tcl",
                "pre_build_targets": ["analyze-debug", "elaborate-debug"],
                "fresh_build_target": "debug-fresh",
                "replay_target": "debug-replay",
                "output": "task-boundary.vpd",
                "signals": ["testbench.dut.tile0.soft_reset_n"],
            }

            output = adapter._configure_task_boundary_trace(snapshot)

            self.assertEqual(output, "task-boundary.vpd")
            self.assertEqual(
                script.read_text(encoding="utf-8"),
                "# ACE-ECHO task-boundary observer\n"
                "dump -file task-boundary.vpd\n"
                "dump -add testbench.dut.tile0.soft_reset_n -depth 0 "
                "-aggregates\n"
                "run 10us\nexit\n",
            )

    def test_task_boundary_trace_rejects_unsafe_signal(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            script = snapshot / "sim/simv_ucli.tcl"
            script.parent.mkdir(parents=True)
            script.write_text("run 10us\nexit\n", encoding="utf-8")
            adapter.flow["task_boundary_trace"] = {
                "ucli_script": "sim/simv_ucli.tcl",
                "pre_build_targets": ["analyze-debug", "elaborate-debug"],
                "output": "task-boundary.vpd",
                "signals": ["testbench.dut; force reset 0"],
            }
            with self.assertRaisesRegex(ValueError, "unsafe.*signal"):
                adapter._configure_task_boundary_trace(snapshot)

    def test_run_local_testbench_edge_logger_is_backend_declared(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                    "sequencer_running_signal": (
                        "tb.dut.tile0.venus.sequencer.running"
                    ),
                    "sequencer_running_ids": 8,
                }],
            }

            output = adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertEqual(output, "task_execution_timing.log")
            self.assertIn(
                "always @(posedge tb.dut.tile0.soft_reset_n)", rendered
            )
            self.assertIn("task %0d start execute at tile 0", rendered)
            self.assertIn(
                "always @(posedge "
                "tb.dut.tile0.venus.sequencer.running[7])",
                rendered,
            )
            self.assertIn("vins 7 complete at task %0d tile 0", rendered)
            self.assertTrue(rendered.rstrip().endswith("endmodule"))

    def test_task_boundary_trace_can_bind_sequencer_port(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                    "sequencer_module": "venus_cluster0_tile0_sequencer",
                    "sequencer_running_port": "pe_vinsn_running_o",
                    "sequencer_running_ids": 8,
                }],
            }

            adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertIn(
                "module ace_echo_vins_observer_tile0", rendered
            )
            self.assertIn(
                "ACE_ECHO_VINS 7 complete tile 0 time %0t", rendered
            )
            self.assertIn(
                "bind venus_cluster0_tile0_sequencer", rendered
            )
            self.assertIn(
                ".running_i(pe_vinsn_running_o)", rendered
            )

    def test_task_boundary_trace_can_bind_shuffle_phase_observer(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                }],
                "shuffle_phase_observers": [{
                    "module": "venus_cluster0_tile0_shuffle_engine",
                    "clock_signal": "clk_i",
                    "reset_signal": "rst_ni",
                    "state_signal": "state_q",
                    "done_signal": "ShuffleUnit_vinsn_done",
                    "grant_signal": "shuffle_result_gnt_i",
                    "id_signal": (
                        "vinsn_queue_q.vinsn[vinsn_queue_q.issue_pnt].id"
                    ),
                    "vm_r_signal": (
                        "vinsn_queue_q.vinsn[vinsn_queue_q.issue_pnt].vm_r"
                    ),
                    "vew_signal": (
                        "vinsn_queue_q.vinsn[vinsn_queue_q.issue_pnt].vew"
                    ),
                    "vl_signal": (
                        "vinsn_queue_q.vinsn[vinsn_queue_q.issue_pnt].vl"
                    ),
                    "lane_count": 64,
                    "pe_count": 64,
                    "id_width": 3,
                    "vl_width": 16,
                }],
            }

            adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertIn(
                "module ace_echo_shuffle_phase_observer_0", rendered
            )
            self.assertIn("ACE_ECHO_SHUFFLE_PHASE", rendered)
            self.assertIn("for (lane=0; lane<64; lane=lane+1)", rendered)
            self.assertIn(
                "bind venus_cluster0_tile0_shuffle_engine", rendered
            )
            self.assertIn(
                ".grant_i(shuffle_result_gnt_i)", rendered
            )

    def test_task_boundary_trace_can_bind_lane_resource_observer(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                }],
                "lane_resource_observers": [{
                    "module": "venus_cluster0_tile0_lane",
                    "clock_signal": "clk_i",
                    "reset_signal": "rst_ni",
                    "lane_id_signal": "lane_id_i",
                    "lane_id_width": 6,
                    "observed_lane": 0,
                    "pe_valid_signal": "pe_req_valid_i",
                    "pe_ready_signal": "pe_req_ready_o",
                    "operand_valid_signal": "operand_req_valid_ls_or",
                    "operand_ready_signal": "operand_req_ready_or_ls",
                    "operand_width": 8,
                    "operand_queue_valid_signal":
                        "operand_queue_cmd_valid_or_oq",
                    "operand_queue_ready_signal": "operand_queue_ready_oq_or",
                    "mask_valid_signal": "operand_mask_req_valid_ls_vm",
                    "mask_ready_signal": "operand_mask_req_ready_vm_ls",
                    "vrf_req_signal": "vrf_req_or_dspm",
                    "vrf_wen_signal": "vrf_wen_or_dspm",
                    "bank_width": 4,
                    "vrf_arb_req_signal": "u_operand_requester.operand_req",
                    "vrf_arb_gnt_signal": "u_operand_requester.operand_gnt",
                    "vrf_arb_width": 48,
                    "hazard_vs1_signal": "pe_req_i.hazard_vs1",
                    "hazard_vs2_signal": "pe_req_i.hazard_vs2",
                    "hazard_vd1_signal": "pe_req_i.hazard_vd1",
                    "hazard_vd2_signal": "pe_req_i.hazard_vd2",
                    "hazard_width": 8,
                    "vfu_valid_signal": "vfu_operation_valid_ls_vfu",
                    "vfu_id_signal": "vfu_operation_ls_vfu.id",
                    "vfu_op_signal": "vfu_operation_ls_vfu.op",
                    "id_width": 3,
                    "op_width": 6,
                    "vm_r_signal": "vfu_operation_ls_vfu.vm_r",
                    "vm_w_signal": "vfu_operation_ls_vfu.vm_w",
                    "bitalu_ready_signal": "bitalu_ready_vfu_ls",
                    "cau_ready_signal": "cau_ready_vfu_ls",
                    "serdiv_ready_signal": "serdiv_ready_vfu_ls",
                    "bitalu_grant_signal": "bitalu_result_gnt_oq_vfu",
                    "cau_grant_signal": "cau_result_gnt_oq_vfu",
                    "serdiv_grant_signal": "serdiv_result_gnt_oq_vfu",
                }],
            }

            adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertIn(
                "module ace_echo_lane_resource_observer_0", rendered
            )
            self.assertIn("ACE_ECHO_LANE_RESOURCE", rendered)
            self.assertIn(
                "bind venus_cluster0_tile0_lane", rendered
            )
            self.assertIn(
                ".operand_valid_i(operand_req_valid_ls_or)", rendered
            )
            self.assertIn(
                ".vrf_arb_req_i(u_operand_requester.operand_req)", rendered
            )
            self.assertIn(
                ".operand_queue_ready_i(operand_queue_ready_oq_or)",
                rendered,
            )

    def test_task_boundary_trace_can_bind_sequencer_ready_observer(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                }],
                "sequencer_ready_observers": [{
                    "module": "venus_cluster0_tile0_sequencer",
                    "clock_signal": "clk_i",
                    "reset_signal": "rst_ni",
                    "ready_vector_signal": "pe_req_ready_i",
                    "ready_width": 65,
                    "pe_valid_signal": "pe_req_valid_o",
                    "state_signal": "state",
                    "state_width": 4,
                    "op_signal": "venus_req_i_transffered_q.op",
                    "op_width": 6,
                    "upstream_valid_signal": "venus_req_valid_i",
                    "upstream_ready_signal": "venus_req_ready_o",
                    "running_full_signal": "vinsn_running_full",
                    "lane_desync_signal": "stall_lanes_desynch",
                    "queue_ready_signal": "vinsn_queue_ready",
                    "queue_done_signal": "insn_queue_done",
                    "queue_counts_signal": "insn_queue_cnt_q",
                    "target_vfus_signal": "target_vfus_vec",
                    "accepted_signal": "accepted_insn",
                    "queue_width": 5,
                    "queue_counts_width": 15,
                }],
            }

            adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertIn(
                "module ace_echo_sequencer_ready_observer_0", rendered
            )
            self.assertIn("ACE_ECHO_SEQUENCER_READY", rendered)
            self.assertIn(
                "bind venus_cluster0_tile0_sequencer", rendered
            )
            self.assertIn(".ready_i(pe_req_ready_i)", rendered)
            self.assertIn("&ready_i[63:0]", rendered)
            self.assertIn(".upstream_ready_i(venus_req_ready_o)", rendered)
            self.assertIn(".queue_counts_i(insn_queue_cnt_q)", rendered)
            self.assertIn("qready %0h qdone %0h qcnt %0h", rendered)

    def test_task_boundary_trace_can_bind_scalar_retire_observer(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                }],
                "scalar_retire_observers": [{
                    "module": "spiritrv32_scalar_core_wrapper",
                    "clock_signal": "clk",
                    "reset_signal": "resetn",
                    "valid_signal": (
                        "u_scalar600_core.wb_stage_i.wb_debug_valid"
                    ),
                    "pc_signal": "u_scalar600_core.wb_stage_i.wb_pc",
                    "instruction_signal": (
                        "u_scalar600_core.wb_stage_i.wb_inst"
                    ),
                }],
            }

            adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertIn(
                "module ace_echo_scalar_retire_observer_0", rendered
            )
            self.assertIn("ACE_ECHO_SCALAR_RETIRE instance %m", rendered)
            self.assertIn(
                "bind spiritrv32_scalar_core_wrapper", rendered
            )
            self.assertIn(
                ".retire_pc_i(u_scalar600_core.wb_stage_i.wb_pc)",
                rendered,
            )

    def test_task_boundary_trace_can_bind_scalar_dispatch_observer(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            adapter, _ = self.adapter(root)
            snapshot = root / "snapshot"
            source = snapshot / "sim/testbenches/test.sv"
            source.parent.mkdir(parents=True)
            source.write_text(
                "module testbench;\nendmodule\n", encoding="utf-8"
            )
            adapter.flow["task_boundary_trace"] = {
                "mode": "testbench_edge_log",
                "testbench_source": "sim/testbenches/test.sv",
                "output": "task_execution_timing.log",
                "tiles": [{
                    "tile": 0,
                    "reset_signal": "tb.dut.tile0.soft_reset_n",
                    "task_signal": "tb.dut.tile0.manager.task_id",
                }],
                "scalar_dispatch_observers": [{
                    "module": "spiritrv32_scalar_core_wrapper",
                    "clock_signal": "clk",
                    "reset_signal": "resetn",
                    "valid_signal": "pico2venus_valid",
                    "ready_signal": "venus2pico_ready",
                    "msb_instruction_signal": "pico2venus_req.insn.msb_insn",
                    "lsb_instruction_signal": "pico2venus_req.insn.lsb_insn",
                    "id_instruction_signal": "u_scalar600_core.id_stage_i.id_inst",
                    "vec_msb_load_signal": "u_scalar600_core.id_stage_i.vec_msb_load",
                    "vec_lsb_load_wait_signal": "u_scalar600_core.id_stage_i.vec_lsb_load_wait",
                    "vec_lsb_load_signal": "u_scalar600_core.id_stage_i.vec_lsb_load",
                    "instr_vector_signal": "u_scalar600_core.id_stage_i.instr_vector",
                    "uncomplete_signal": "u_scalar600_core.id_stage_i.uncomplete",
                    "stall_signal": "u_scalar600_core.id_stage_i.stall_req",
                    "vector_wait_signal": "u_scalar600_core.id_stage_i.vector_wait_req",
                }],
            }

            adapter._configure_task_boundary_trace(snapshot)
            rendered = source.read_text(encoding="utf-8")

            self.assertIn(
                "module ace_echo_scalar_dispatch_observer_0", rendered
            )
            self.assertIn("ACE_ECHO_SCALAR_DISPATCH instance %m", rendered)
            self.assertIn(
                ".request_valid_i(pico2venus_valid)", rendered
            )
            self.assertIn(
                ".vector_wait_i(u_scalar600_core.id_stage_i.vector_wait_req)",
                rendered,
            )

    def adapter(self, root: Path):
        rtl = root / "rtl"
        rtl.mkdir()
        (rtl / "tracked").write_text("source", encoding="utf-8")
        manifest = {
            "engines": {"rtl": {
                "claim_scope": "test",
                "flow": {
                    "snapshot_excludes": [".git", "sim/build_*"],
                    "firmware_destinations": [
                        "software/scheduler/l1.bin",
                        "software/cases/{case_name}/l1.bin",
                    ],
                    "fresh_build_target": "fresh",
                    "replay_target": "replay",
                    "make_variables": {
                        "SIM_NAME": "chip_", "SIM_TYPE": "simonly"
                    },
                    "runtime_case_variable": "CASE",
                    "unique_build_variable": "COPY_ID",
                    "evidence_paths": ["sim.log"],
                },
            }},
            "repositories": {"rtl": {}},
        }
        backend = mock.MagicMock()
        backend.rtl_root = rtl
        backend.backend_id = "fixture"
        backend.manifest = manifest
        runner = mock.MagicMock()
        runner.dry_run = True
        return RtlAdapter(runner, backend), runner

    def test_rejects_unsafe_case_name(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            adapter, _ = self.adapter(Path(temporary))
            with self.assertRaisesRegex(ValueError, "unsafe RTL case"):
                adapter.run(
                    workspace=Path(temporary) / "work",
                    cases=[RtlCase("../escape", Path("firmware.bin"))],
                    timeout=1,
                )

    def test_dry_run_compiles_once_then_replays(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            adapter, runner = self.adapter(Path(temporary))
            report = adapter.run(
                workspace=Path(temporary) / "work",
                cases=[
                    RtlCase("PBCH", Path("pbch.bin")),
                    RtlCase("PDCCH", Path("pdcch.bin")),
                ],
                timeout=10,
            )
            self.assertEqual(report.name, "validation-report.json")
            commands = [call.args[1] for call in runner.run.call_args_list]
            self.assertIn("fresh", commands[0])
            self.assertIn("replay", commands[1])
            self.assertIn("CASE=PBCH", commands[0])
            self.assertIn("CASE=PDCCH", commands[1])

    def test_split_build_budget_and_per_case_runtime_budget(self):
        with tempfile.TemporaryDirectory() as temporary:
            adapter,runner=self.adapter(Path(temporary))
            adapter.flow.update(fresh_build_steps=['analyze','elaborate'],
                fresh_simulator_path='sim/build_{SIM_NAME}{SIM_TYPE}/simv')
            adapter.run(workspace=Path(temporary)/'work',
                cases=[RtlCase('first',Path('a.bin')),RtlCase('second',Path('b.bin'))],
                timeout=10,build_timeout=60,simulation_timeout=90)
            calls=runner.run.call_args_list
            self.assertEqual([c.args[1][-1] for c in calls],['analyze','elaborate','replay','replay'])
            self.assertTrue(all(0<c.kwargs['timeout']<=60 for c in calls[:2]))
            self.assertEqual([c.kwargs['timeout'] for c in calls[2:]],[90,90])

    def test_invalid_timeout_rejected_before_build(self):
        with tempfile.TemporaryDirectory() as temporary:
            adapter,runner=self.adapter(Path(temporary))
            with self.assertRaisesRegex(ValueError,'timeouts'):
                adapter.run(workspace=Path(temporary)/'work',cases=[RtlCase('case',Path('a.bin'))],timeout=10,build_timeout=0)
            runner.run.assert_not_called()

    def test_failed_build_preserves_snapshot_even_without_keep_build(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            adapter,runner=self.adapter(root)
            runner.dry_run=False
            firmware=root/'l1.bin';firmware.write_bytes(b'firmware')
            workspace=root/'work';workspace.mkdir()
            runner.run.side_effect=RuntimeError('failed build')
            with self.assertRaisesRegex(RuntimeError,'failed build'):
                adapter.run(workspace=workspace,cases=[RtlCase('case',firmware)],timeout=10)
            self.assertTrue((workspace/'rtl-source').is_dir())

    def test_startup_classifier_does_not_mask_crc_failure(self):
        import hashlib
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            adapter,_=self.adapter(root)
            source=root/'devctrl.sv';source.write_text('known')
            adapter.flow['startup_diagnostics']=[dict(id='startup',source_file='devctrl.sv',
                source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),instance='tb.dut',message='decode toggle',max_occurrences=1)]
            adapter.flow['progress_markers']={'crc_failure':'GPIO4'}
            build=root/'build';build.mkdir()
            (build/'sim.log').write_text(f'Error: "{source}", 9: tb.dut: at time 0 fs\ndecode toggle\nGPIO4 pluse\nTest complete!\n')
            firmware=root/'l1.bin';firmware.write_bytes(b'x')
            result=adapter._case_result(RtlCase('case',firmware),build,[],source_root=root)
            self.assertEqual(result['status'],'FAIL')
            self.assertEqual(result['known_nonfunctional_simulator_error_count'],1)

    def test_case_result_rejects_crc_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            adapter, _ = self.adapter(root)
            adapter.flow["progress_markers"] = {"crc_failure": "GPIO4"}
            build = root / "build"
            build.mkdir()
            (build / "sim.log").write_text(
                "GPIO4 pluse\nTest complete!\n", encoding="utf-8"
            )
            firmware = root / "l1.bin"
            firmware.write_bytes(b"x")
            result = adapter._case_result(
                RtlCase("PBCH", firmware), build, []
            )
            self.assertEqual(result["status"], "FAIL")

    def test_case_result_surfaces_legacy_axi_unknown_warnings(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            adapter, _ = self.adapter(root)
            build = root / "build"
            build.mkdir()
            (build / "sim.log").write_text(
                'Error: "axi_vif.sv", 124: top.m_vif.'
                'assertWriteDataUnknown: at time 10 ps\n'
                "Test complete!\n",
                encoding="utf-8",
            )
            firmware = root / "l1.bin"
            firmware.write_bytes(b"x")
            result = adapter._case_result(
                RtlCase("PBCH", firmware), build, []
            )
            self.assertEqual(result["status"], "PASS")
            self.assertEqual(
                result["qualification_status"],
                "SMOKE_PASS_WITH_AXI_UNKNOWN_WARNINGS",
            )
            self.assertEqual(result["axi_unknown_assertion_count"], 1)
            self.assertEqual(result["non_axi_simulator_error_count"], 0)

    def test_case_result_rejects_non_axi_simulator_errors(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            adapter, _ = self.adapter(root)
            build = root / "build"
            build.mkdir()
            (build / "sim.log").write_text(
                "Error: functional assertion failed\nTest complete!\n",
                encoding="utf-8",
            )
            firmware = root / "l1.bin"
            firmware.write_bytes(b"x")
            result = adapter._case_result(
                RtlCase("PBCH", firmware), build, []
            )
            self.assertEqual(result["status"], "FAIL")
            self.assertEqual(result["non_axi_simulator_error_count"], 1)

    def test_case_result_accepts_manifest_qualified_time_zero_error(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            adapter, _ = self.adapter(root)
            adapter.flow["known_nonfunctional_simulator_errors"] = [{
                "id": "legacy-reset-toggle",
                "line_contains": ["legacy_devctrl.sv", "at time 0 ps"],
                "next_line_equals": "reset decode toggled unexpectedly",
            }]
            build = root / "build"
            build.mkdir()
            (build / "sim.log").write_text(
                'Error: "legacy_devctrl.sv", 9: top: at time 0 ps\n'
                "reset decode toggled unexpectedly\n"
                "Test complete!\n",
                encoding="utf-8",
            )
            firmware = root / "l1.bin"
            firmware.write_bytes(b"x")
            result = adapter._case_result(
                RtlCase("PBCH", firmware), build, []
            )
            self.assertEqual(result["status"], "PASS")
            self.assertEqual(
                result["qualification_status"],
                "SMOKE_PASS_WITH_KNOWN_NONFUNCTIONAL_WARNINGS",
            )
            self.assertEqual(
                result["known_nonfunctional_simulator_error_count"], 1
            )
            self.assertEqual(result["non_axi_simulator_error_count"], 0)

    def test_known_time_zero_rule_does_not_hide_later_error(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            adapter, _ = self.adapter(root)
            adapter.flow["known_nonfunctional_simulator_errors"] = [{
                "id": "legacy-reset-toggle",
                "line_contains": ["legacy_devctrl.sv", "at time 0 ps"],
                "next_line_equals": "reset decode toggled unexpectedly",
            }]
            build = root / "build"
            build.mkdir()
            (build / "sim.log").write_text(
                'Error: "legacy_devctrl.sv", 9: top: at time 10 ps\n'
                "reset decode toggled unexpectedly\n"
                "Test complete!\n",
                encoding="utf-8",
            )
            firmware = root / "l1.bin"
            firmware.write_bytes(b"x")
            result = adapter._case_result(
                RtlCase("PBCH", firmware), build, []
            )
            self.assertEqual(result["status"], "FAIL")
            self.assertEqual(result["non_axi_simulator_error_count"], 1)

    def test_snapshot_metadata_edits_are_explicit_and_local(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            snapshot = Path(temporary) / "snapshot"
            (snapshot / "tcl").mkdir(parents=True)
            (snapshot / "ip/core/src").mkdir(parents=True)
            script = snapshot / "tcl/core.tcl"
            listing = snapshot / "ip/core/src/files.lst"
            script.write_text("keep\nremove me\n", encoding="utf-8")
            listing.write_text("/old/root/core.v\n", encoding="utf-8")
            RtlAdapter._adapt_snapshot_metadata(snapshot, {
                "text_edits": [{
                    "path": "tcl/core.tcl",
                    "remove_exact_lines": ["remove me"],
                }],
                "generated_path_rewrites": [{
                    "glob": "ip/**/*.lst", "old": "/old/root",
                    "new": "{snapshot}/ip",
                }],
            })
            self.assertEqual(script.read_text(encoding="utf-8"), "keep\n")
            self.assertEqual(
                listing.read_text(encoding="utf-8"),
                f"{snapshot}/ip/core.v\n",
            )


if __name__ == "__main__":
    unittest.main()
