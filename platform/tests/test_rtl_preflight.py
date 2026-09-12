from __future__ import annotations

import json
from pathlib import Path
import socket
import tempfile
import unittest
from unittest import mock

from ace_echo.rtl_preflight import run_rtl_preflight


class RtlPreflightTests(unittest.TestCase):
    def fixture(self, root: Path, call: str) -> Path:
        rtl = root / "rtl"
        rtl.mkdir()
        (rtl / "model.sv").write_text(
            "function vins_enqueue(pe_req_t req, vid_t vid);\nendfunction\n",
            encoding="utf-8",
        )
        (rtl / "tb.sv").write_text(call + "\n", encoding="utf-8")
        backend = root / "backend.json"
        backend.write_text(json.dumps({
            "backend_id": "test",
            "paths": {"rtl_root": "rtl"},
            "engines": {"rtl": {"preflight": {
                "systemverilog_function_arity": [{
                    "function": "vins_enqueue",
                    "declaration_file": "model.sv",
                    "call_files": ["tb.sv"],
                }],
            }}},
        }), encoding="utf-8")
        return backend

    def test_matching_function_arity_passes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            backend = self.fixture(
                Path(temporary), "model.vins_enqueue(packet(req, 1), vid);"
            )
            report = run_rtl_preflight(backend)
            self.assertEqual(report["status"], "PASS")
            self.assertTrue(report["read_only"])

    def test_mismatched_function_arity_fails_with_line_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            backend = self.fixture(
                Path(temporary), "// ignored vins_enqueue(a, b)\nmodel.vins_enqueue(req);"
            )
            report = run_rtl_preflight(backend)
            self.assertEqual(report["status"], "FAIL")
            mismatch = report["checks"][0]["mismatches"][0]
            self.assertEqual(mismatch["arity"], 1)
            self.assertEqual(mismatch["line"], 2)

    def test_unreachable_configured_license_blocks_before_build(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            backend = self.fixture(
                root, "model.vins_enqueue(packet(req, 1), vid);"
            )
            manifest = json.loads(backend.read_text(encoding="utf-8"))
            tool_dir = root / "tools"
            tool_dir.mkdir()
            for name in ("vlogan", "vcs"):
                executable = tool_dir / name
                executable.write_text("#!/bin/sh\n", encoding="utf-8")
                executable.chmod(0o755)
            manifest["engines"]["rtl"].update({
                "flow": {
                    "path_prepend": [str(tool_dir)],
                    "required_environment": {
                        "LM_LICENSE_FILE": "27000@license-host",
                        "SNPSLMD_LICENSE_FILE": "27000@license-host",
                    },
                },
            })
            backend.write_text(json.dumps(manifest), encoding="utf-8")
            with mock.patch.object(
                socket, "create_connection", side_effect=ConnectionRefusedError
            ):
                report = run_rtl_preflight(backend)
            self.assertEqual(report["status"], "BLOCKED")
            license_checks = [
                item for item in report["checks"]
                if item["id"].startswith("rtl-license:")
            ]
            self.assertEqual(len(license_checks), 2)
            self.assertTrue(all(
                item["status"] == "BLOCKED" for item in license_checks
            ))

    def test_reachable_configured_license_passes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            backend = self.fixture(
                root, "model.vins_enqueue(packet(req, 1), vid);"
            )
            manifest = json.loads(backend.read_text(encoding="utf-8"))
            tool_dir = root / "tools"
            tool_dir.mkdir()
            for name in ("vlogan", "vcs"):
                executable = tool_dir / name
                executable.write_text("#!/bin/sh\n", encoding="utf-8")
                executable.chmod(0o755)
            manifest["engines"]["rtl"].update({
                "flow": {
                    "path_prepend": [str(tool_dir)],
                    "required_environment": {
                        "LM_LICENSE_FILE": "27000@license-host",
                        "SNPSLMD_LICENSE_FILE": "27000@license-host",
                    },
                },
            })
            backend.write_text(json.dumps(manifest), encoding="utf-8")
            connection = mock.MagicMock()
            connection.__enter__.return_value = connection
            with mock.patch.object(
                socket, "create_connection", return_value=connection
            ):
                report = run_rtl_preflight(backend)
            self.assertEqual(report["status"], "PASS")

    def test_sandbox_localhost_license_requires_host_recheck(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            backend = self.fixture(
                root, "model.vins_enqueue(packet(req, 1), vid);"
            )
            manifest = json.loads(backend.read_text(encoding="utf-8"))
            tool_dir = root / "tools"
            tool_dir.mkdir()
            for name in ("vlogan", "vcs"):
                executable = tool_dir / name
                executable.write_text("#!/bin/sh\n", encoding="utf-8")
                executable.chmod(0o755)
            manifest["engines"]["rtl"].update({
                "flow": {
                    "path_prepend": [str(tool_dir)],
                    "required_environment": {
                        "LM_LICENSE_FILE": "27000@localhost",
                        "SNPSLMD_LICENSE_FILE": "27000@localhost",
                    },
                },
            })
            backend.write_text(json.dumps(manifest), encoding="utf-8")
            with mock.patch.dict(
                "os.environ", {"CODEX_SANDBOX_NETWORK_DISABLED": "1"}
            ), mock.patch(
                "ace_echo.rtl_preflight._local_listen_port",
                return_value=False,
            ):
                report = run_rtl_preflight(backend)
            self.assertEqual(report["status"], "BLOCKED")
            self.assertTrue(
                report["execution_context"]["sandbox_network_disabled"]
            )
            license_checks = [
                item for item in report["checks"]
                if item["id"].startswith("rtl-license:")
            ]
            self.assertTrue(all(
                item["status"] == "BLOCKED"
                and item["reason_code"] == "REQUIRES_HOST_RECHECK"
                for item in license_checks
            ))


if __name__ == "__main__":
    unittest.main()
