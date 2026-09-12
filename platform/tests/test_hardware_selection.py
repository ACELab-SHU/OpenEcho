from pathlib import Path
from types import SimpleNamespace
import copy
import json
import shutil
import tempfile
import unittest
from unittest.mock import patch

from ace_echo import selection, cli
from ace_echo.config import ConfigError
from ace_echo.forge import validate_forge_request, ForgeContractError

ROOT = Path(__file__).resolve().parents[1]


class SelectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        shutil.copytree(ROOT / "configs", self.root / "configs")
        (self.root / ".ace-echo").mkdir()
        self.project = self.root / ".ace-echo/project.toml"
        self.project.write_text('hardware = "venus2.0"\ntoolchain = "shared"\n')
        self.host = self.root / ".ace-echo/host-paths.json"
        self.host_data = {"schema": "ace-echo-host-paths/v1",
                          "toolchains": {"shared": {"venus_llvm_bin": str(self.root / "llvm"),
                                                   "riscv_gcc_root": str(self.root / "gcc")}},
                          "rtl": {"venus2.0": "/custom/v2", "venus1.0": "/custom/v1"}}
        self.write_host()

    def write_host(self):
        self.host.write_text(json.dumps(self.host_data))

    def test_v2_maps_to_owned_backend_and_personal_rtl(self):
        raw, backend, tools = selection.plan(self.root)
        self.assertEqual(raw["hardware"], "venus2.0")
        self.assertEqual(backend.name, "venus2p0-16x128.json")
        self.assertEqual(tools["rtl_root"], "/custom/v2")

    def test_only_version_line_changes_hardware_not_compiler(self):
        _, _, before = selection.plan(self.root)
        self.project.write_text('hardware = "venus1.0"\ntoolchain = "shared"\n')
        _, backend, after = selection.plan(self.root)
        self.assertEqual(backend.name, "venus1p0-64x512-300mhz.json")
        self.assertEqual(before["venus_llvm_bin"], after["venus_llvm_bin"])
        self.assertEqual(after["rtl_root"], "/custom/v1")

    def test_independent_compiler_selection(self):
        self.host_data["toolchains"]["alternate"] = {
            "venus_llvm_bin": "/alternate/llvm", "riscv_gcc_root": "/alternate/gcc"}
        self.write_host()
        self.project.write_text('hardware = "venus2.0"\ntoolchain = "alternate"\n')
        raw, backend, tools = selection.plan(self.root)
        self.assertEqual(raw["hardware"], "venus2.0")
        self.assertEqual(tools["venus_llvm_bin"], "/alternate/llvm")
        self.assertEqual(tools["rtl_root"], "/custom/v2")

    def test_unknown_hardware_rejected(self):
        self.project.write_text('hardware = "venus9.0"\ntoolchain = "shared"\n')
        with self.assertRaisesRegex(ConfigError, "Unknown hardware"):
            selection.plan(self.root)

    def test_unknown_toolchain_rejected(self):
        self.project.write_text('hardware = "venus2.0"\ntoolchain = "absent"\n')
        with self.assertRaisesRegex(ConfigError, "Unknown host toolchain"):
            selection.plan(self.root)

    def test_relative_host_path_rejected(self):
        self.host_data["rtl"]["venus2.0"] = "../other-user"
        self.write_host()
        with self.assertRaisesRegex(ConfigError, "absolute"):
            selection.plan(self.root)

    def test_host_cannot_override_hardware(self):
        self.host_data["toolchains"]["shared"]["rows"] = 512
        self.write_host()
        with self.assertRaisesRegex(ConfigError, "Only compiler"):
            selection.plan(self.root)

    def test_no_rtl_is_valid_for_software(self):
        self.host_data["rtl"] = {}
        self.write_host()
        self.assertNotIn("rtl_root", selection.plan(self.root)[2])

    def test_unknown_project_key_rejected(self):
        self.project.write_text('hardware = "venus2.0"\ntoolchain = "shared"\nlanes=64\n')
        with self.assertRaises(ConfigError):
            selection.plan(self.root)

    def test_explicit_backend_conflict(self):
        v2 = self.root / "configs/backends/venus2p0-16x128.json"
        context = {"backend": str(v2)}
        actual = selection.object_file(v2)
        actual["architecture"]["lanes"] = 64
        with self.assertRaisesRegex(ConfigError, "conflicts"):
            selection.check_backend(context, actual)

    def test_workload_root_relocation_allowed_but_toolchain_not(self):
        v2 = self.root / "configs/backends/venus2p0-16x128.json"
        context = {"backend": str(v2)}
        actual = selection.object_file(v2)
        actual["paths"]["workload_root"] = "/new/input/cases"
        selection.check_backend(context, actual)
        actual["engines"]["toolchain"] = {"venus_llvm_bin": "/wrong"}
        with self.assertRaisesRegex(ConfigError, "Compiler"):
            selection.check_backend(context, actual)

    def fixture_setup(self):
        for rel in ("llvm", "gcc/bin", "components/toolchain/dsl", "workloads/5g_lite",
                    ".ace-echo/deps/json/include/nlohmann", "scripts"):
            (self.root / rel).mkdir(parents=True, exist_ok=True)
        for name in ("clang", "opt", "llc", "llvm-objcopy", "llvm-objdump"):
            p = self.root / "llvm" / name
            p.write_text("test-only executable placeholder")
            p.chmod(0o755)
        gcc = self.root / "gcc/bin/riscv32-unknown-elf-gcc"
        gcc.write_text("test-only GCC placeholder")
        gcc.chmod(0o755)
        (self.root / "components/toolchain/dsl/config.mk").write_text("fixture")
        (self.root / "workloads/5g_lite/task-index.json").write_text("{}")
        (self.root / ".ace-echo/deps/json/include/nlohmann/json.hpp").write_text("fixture")
        shutil.copy2(ROOT / "scripts/bootstrap.py", self.root / "scripts/bootstrap.py")
        self.bootstrap = selection.bootstrap_module(self.root)
        def git(path, *args):
            if args[0] == "status":
                return ""
            return self.bootstrap.JSON_COMMIT if path.name == "json" else "a" * 40
        for patcher in (
            patch("ace_echo.selection.bootstrap_module", return_value=self.bootstrap),
            patch.object(self.bootstrap, "git", side_effect=git),
            patch.object(self.bootstrap.subprocess, "check_output", return_value=b"config.mk\0"),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def test_prepare_reuses_cache_and_separates_versions(self):
        self.fixture_setup()
        config, v2 = selection.prepare(self.root)
        again, _ = selection.prepare(self.root)
        self.assertEqual(config.source, again.source)
        self.assertEqual(config.run_root, self.root / "runs/venus2.0")
        self.project.write_text('hardware = "venus1.0"\ntoolchain = "shared"\n')
        other, v1 = selection.prepare(self.root)
        self.assertNotEqual(v1["artifact_key"], v2["artifact_key"])
        self.assertNotEqual(config.source, other.source)
        self.assertEqual(other.run_root, self.root / "runs/venus1.0")

    def test_cache_tamper_rejected(self):
        self.fixture_setup()
        config, _ = selection.prepare(self.root)
        config.source.write_text(config.source.read_text() + "\n# changed\n")
        with self.assertRaisesRegex(ConfigError, "modified"):
            selection.prepare(self.root)

    def test_changed_compiler_invalidates_cache(self):
        self.fixture_setup()
        config, old = selection.prepare(self.root)
        (self.root / "llvm/llc").write_text("different test compiler")
        changed, new = selection.prepare(self.root)
        self.assertNotEqual(config.source, changed.source)
        self.assertNotEqual(old["artifact_key"], new["artifact_key"])

    def test_cli_all_architecture_commands_inherit_selection(self):
        self.fixture_setup()
        for argv in (["doctor", "--scope", "fast"], ["compile", "dag", "--target", "sample"],
                     ["--dry-run", "scheduler", "build"],
                     ["--dry-run", "run", "dag", "--dag-json", "/missing/dag.json",
                      "--combined-bin", "/missing/dag.bin", "--case-dir", "/missing/tasks"],
                     ["--dry-run", "rtl", "run", "--case", "probe=/missing/l1.bin"],
                     ["rtl-preflight"], ["hardware", "show"]):
            with patch.object(cli, "PROJECT_ROOT", self.root):
                args = cli.parser().parse_args(argv)
                config, context = cli.selected_config(args)
                self.assertEqual(context["hardware"], "venus2.0")
                if hasattr(args, "backend"):
                    self.assertEqual(args.backend, Path(context["backend"]))

    def test_explicit_legacy_config_remains_available(self):
        args = cli.parser().parse_args(["--config", str(ROOT / "configs/local.toml"), "doctor"])
        config, context = cli.selected_config(args)
        self.assertIsNone(context)


class ArtifactIdentityTests(unittest.TestCase):
    def test_missing_wrong_hardware_and_modified_inputs_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            handoff = root / "artifacts/toolchain"
            handoff.mkdir(parents=True)
            binary = handoff / "dag1.bin"
            binary.write_bytes(b"compiled")
            context = {"artifact_key": "v2-key", "backend_id": "venus2"}
            with self.assertRaisesRegex(ConfigError, "No hardware"):
                selection.check_artifact(binary, context)
            selection.save(root / "hardware-selection.json", context)
            selection.save(root / "run.json", {"status": "PASS"})
            work = SimpleNamespace(path=root, artifacts=root / "artifacts",
                                   record=SimpleNamespace(scope="compile-dag"))
            selection.stamp(work)
            selection.check_artifact(binary, context)
            with self.assertRaisesRegex(ConfigError, "mismatch"):
                selection.check_artifact(binary, dict(context, artifact_key="v1-key"))
            binary.write_bytes(b"corrupted")
            with self.assertRaisesRegex(ConfigError, "modified"):
                selection.check_artifact(binary, context)

    def test_incomplete_producer_is_not_accepted(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            handoff = root / "artifacts/toolchain"
            handoff.mkdir(parents=True)
            binary = handoff / "dag1.bin"
            binary.write_bytes(b"compiled")
            context = {"artifact_key": "v2-key", "backend_id": "venus2"}
            selection.save(root / "hardware-selection.json", context)
            selection.save(root / "run.json", {"status": "RUNNING"})
            selection.stamp(SimpleNamespace(path=root, artifacts=root / "artifacts",
                           record=SimpleNamespace(scope="compile-dag")))
            with self.assertRaisesRegex(ConfigError, "completed PASS"):
                selection.check_artifact(binary, context)


class ForgeSelectionTests(unittest.TestCase):
    def test_project_binding_preserves_request_and_golden_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "request.json"
            request = json.loads((ROOT / "examples/forge-vector-smoke.example.json").read_text())
            request["backend_manifest"] = "@project"
            path.write_text(json.dumps(request))
            backend = ROOT / "configs/backends/venus2p0-16x128.json"
            resolved, profile = validate_forge_request(path, backend_override=backend)
            self.assertEqual(profile["backend_id"], "venus2p0-16x128")
            self.assertEqual(resolved["reference"]["state"], "PROVISIONAL_GOLDEN")
            self.assertEqual(json.loads(path.read_text()), request)
            with self.assertRaisesRegex(ForgeContractError, "requires"):
                validate_forge_request(path)
            request["backend_manifest"] = str(backend)
            path.write_text(json.dumps(request))
            with self.assertRaisesRegex(ForgeContractError, "@project"):
                validate_forge_request(path, backend_override=backend)


if __name__ == "__main__":
    unittest.main()
