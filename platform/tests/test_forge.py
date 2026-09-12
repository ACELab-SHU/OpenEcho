from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from ace_echo.forge import (
    BACKEND_SCHEMA,
    FINAL_SCHEMA,
    ForgeContractError,
    REFERENCE_SCHEMA,
    REQUEST_SCHEMA,
    prepare_forge_run,
    prune_completed_run,
    validate_forge_request,
)


class ForgeTests(unittest.TestCase):
    def backend(self, root: Path) -> Path:
        path = root / "backend.json"
        path.write_text(json.dumps({
            "schema": BACKEND_SCHEMA,
            "backend_id": "test-venus",
            "architecture": {
                "rows": 128,
                "lanes": 16,
                "banks_per_lane": 4,
                "bank_width_bits": 16,
                "workspace_alignment_bytes": 64,
                "workspace_limit_bytes": 409600,
            },
            "abi": {
                "task_input_descriptor": {
                    "physical_type_bits": 2,
                    "logical_to_physical_type": {
                        "0": 0, "1": 1, "2": 1,
                        "4": 0, "5": 1, "6": 1,
                    },
                    "rtl_unsupported_logical_types": [4, 5, 6],
                },
                "scheduler_output": {
                    "max_task_outputs": 16,
                    "max_dag_outputs": 8,
                    "task_return_length_bits": 32,
                    "vreturn_length_unit": "bytes",
                    "variable_length_outputs": True,
                    "dma_beat_bytes": 64,
                    "descriptor_response_lag_ports": 0,
                    "descriptor_transfer_chunk_bytes": 64,
                },
            },
            "execution_policy": {"forbidden_engines": ["vemu"]},
            "paths": {
                "dsl_root": "dsl",
                "workload_root": "workloads",
                "gem5_root": "gem5",
                "rtl_root": "rtl",
            },
            "engines": {
                "scheduler": {"build_target": "venus"},
                "gem5": {"fast": {}, "verification": {}},
                "rtl": {
                    "source_immutable": True,
                    "flow": {
                        "snapshot_excludes": [".git"],
                        "firmware_destinations": ["software/l1.bin"],
                        "fresh_build_target": "sim_compileall",
                        "replay_target": "sim_nocompile",
                        "make_variables": {"SIM_NAME": "test_"},
                        "runtime_case_variable": "TARGET_BIN_FILE_NAME",
                        "unique_build_variable": "COPY_ID",
                        "required_environment": {"VCS_HOME": "/tools/vcs"},
                        "evidence_paths": ["sim.log"],
                    },
                },
            },
        }) + "\n", encoding="utf-8")
        return path

    def reference(self, root: Path) -> Path:
        path = root / "reference.json"
        path.write_text(json.dumps({
            "schema": REFERENCE_SCHEMA,
            "adapter_id": "python-reference",
            "commands": {
                key: ["python3", f"{key}.py"]
                for key in ("prepare", "build", "run", "extract")
            },
            "input_schema": {},
            "output_schema": {},
            "comparison": {"mode": "bit_exact"},
        }) + "\n", encoding="utf-8")
        return path

    def request(self, root: Path, state: str = "PROVISIONAL_GOLDEN") -> Path:
        backend = self.backend(root)
        reference = self.reference(root)
        data = {
            "schema": REQUEST_SCHEMA,
            "intent": "port a reference application",
            "source": {"kind": "executable_source"},
            "backend_manifest": backend.name,
            "reference": {"state": state, "adapter": reference.name},
            "workload": {
                "kind": "application",
                "name": "demo",
                "required_correctness_scopes": ["final_output_exact"],
            },
            "optimization": {
                "primary_metric": "gem5_ticks",
                "max_candidates": 20,
                "plateau_candidates": 5,
            },
            "cleanup": {
                "after_application_complete": True,
                "retain_top_correct": 3,
            },
            "validation": {
                "vemu_forbidden": True,
                "rtl_source_immutable": True,
                "compare_all_visible_outputs": True,
            },
        }
        if state == "AUTHORITATIVE_GOLDEN":
            data["reference"]["approval"] = {
                "reviewer": "human",
                "decision": "accept",
                "golden_digest": "sha256:" + "a" * 64,
            }
        path = root / "request.json"
        path.write_text(json.dumps(data) + "\n", encoding="utf-8")
        return path

    def test_request_and_run_are_valid(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            parsed, backend = validate_forge_request(request)
            self.assertEqual(backend["backend_id"], "test-venus")
            run = prepare_forge_run(request, root / "runs")
            manifest = json.loads((run / "forge-run.json").read_text())
            self.assertEqual(manifest["golden_gate"], "REVIEW_REQUIRED")
            self.assertTrue(manifest["vemu_forbidden"])
            self.assertEqual(manifest["full_software_engine"], "gem5.verification")
            self.assertEqual(manifest["diagnostic_engine"], "gem5.verification")

    def test_fast_policy_is_recorded_without_promoting_golden(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["execution_policy"]["full_software_engine"] = "gem5.fast"
            backend["execution_policy"]["diagnostic_engine"] = "gem5.verification"
            backend_path.write_text(json.dumps(backend))
            run = prepare_forge_run(request, root / "runs")
            manifest = json.loads((run / "forge-run.json").read_text())
            self.assertEqual(manifest["full_software_engine"], "gem5.fast")
            self.assertEqual(manifest["diagnostic_engine"], "gem5.verification")
            self.assertEqual(manifest["golden_gate"], "REVIEW_REQUIRED")

    def test_diagnostic_engine_cannot_silently_use_fast(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["execution_policy"]["diagnostic_engine"] = "gem5.fast"
            backend_path.write_text(json.dumps(backend))
            with self.assertRaisesRegex(ForgeContractError, "diagnostic_engine"):
                validate_forge_request(request)

    def test_source_implementation_is_not_yet_a_golden_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root, "SOURCE_IMPLEMENTATION")
            run = prepare_forge_run(request, root / "runs")
            manifest = json.loads((run / "forge-run.json").read_text())
            self.assertEqual(
                manifest["golden_gate"], "PENDING_REFERENCE_EXECUTION"
            )

    def test_authoritative_golden_requires_human_approval(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root, "AUTHORITATIVE_GOLDEN")
            data = json.loads(request.read_text())
            del data["reference"]["approval"]
            request.write_text(json.dumps(data))
            with self.assertRaises(ForgeContractError):
                validate_forge_request(request)

    def test_backend_cannot_expose_vemu_engine(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["engines"]["vemu"] = {"binary": "Emulator"}
            backend_path.write_text(json.dumps(backend))
            with self.assertRaises(ForgeContractError):
                validate_forge_request(request)

    def test_backend_rejects_invalid_output_descriptor_lag(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["abi"]["scheduler_output"][
                "descriptor_response_lag_ports"
            ] = 16
            backend_path.write_text(json.dumps(backend))
            with self.assertRaisesRegex(
                ForgeContractError, "descriptor_response_lag_ports"
            ):
                validate_forge_request(request)

    def test_backend_requires_positive_max_dag_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["abi"]["scheduler_output"]["max_dag_outputs"] = 0
            backend_path.write_text(json.dumps(backend))
            with self.assertRaisesRegex(
                ForgeContractError, "max_dag_outputs"
            ):
                validate_forge_request(request)

    def test_backend_rejects_unaligned_descriptor_transfer_chunk(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["abi"]["scheduler_output"][
                "descriptor_transfer_chunk_bytes"
            ] = 96
            backend_path.write_text(json.dumps(backend))
            with self.assertRaisesRegex(
                ForgeContractError, "descriptor_transfer_chunk_bytes"
            ):
                validate_forge_request(request)

    def test_backend_rejects_fixed_slot_semantics_as_length_unit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            backend_path = root / "backend.json"
            backend = json.loads(backend_path.read_text())
            backend["abi"]["scheduler_output"][
                "vreturn_length_unit"
            ] = "fixed_64_byte_slot"
            backend_path.write_text(json.dumps(backend))
            with self.assertRaisesRegex(
                ForgeContractError, "vreturn_length_unit"
            ):
                validate_forge_request(request)

    def test_prune_rejects_incomplete_application(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            run = prepare_forge_run(self.request(root), root / "runs")
            (run / "final-report.json").write_text(json.dumps({
                "schema": FINAL_SCHEMA,
                "status": "FAIL",
                "application_complete": False,
            }))
            with self.assertRaises(ForgeContractError):
                prune_completed_run(run)

    def test_prune_keeps_baseline_winner_rtl_and_top_three(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            request = self.request(root)
            run = prepare_forge_run(request, root / "runs")
            candidates = run / "candidates"
            definitions = [
                ("c0", "baseline", True, False, 100),
                ("c1", "candidate", True, False, 90),
                ("c2", "candidate", True, False, 80),
                ("c3", "candidate", True, False, 70),
                ("c4", "candidate", False, False, 60),
                ("c5", "winner", True, True, 50),
            ]
            for candidate_id, role, correct, rtl, score in definitions:
                candidate = candidates / candidate_id
                (candidate / "logs").mkdir(parents=True)
                (candidate / "logs" / "gem5.log").write_text("large")
                (candidate / "candidate.json").write_text(json.dumps({
                    "candidate_id": candidate_id,
                    "role": role,
                    "correct": correct,
                    "rtl_qualified": rtl,
                    "fitness": "RANKED" if correct else "INELIGIBLE",
                    "score": score,
                }))
            (run / "final-report.json").write_text(json.dumps({
                "schema": FINAL_SCHEMA,
                "status": "PASS",
                "application_complete": True,
                "metric_direction": "minimize",
                "retain_top_correct": 3,
            }))
            report = prune_completed_run(run)
            self.assertEqual(
                set(report["kept_candidate_ids"]), {"c0", "c2", "c3", "c5"}
            )
            self.assertFalse((candidates / "c1" / "logs").exists())
            self.assertFalse((candidates / "c4" / "logs").exists())
            self.assertTrue((candidates / "c1" / "candidate.json").exists())


if __name__ == "__main__":
    unittest.main()
