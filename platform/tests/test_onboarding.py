from pathlib import Path
import importlib.util
import json
import tempfile
import unittest
from unittest.mock import patch

from ace_echo.config import load_config
from ace_echo.doctor import run_doctor, Check

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("onboarding_bootstrap", ROOT / "scripts/bootstrap.py")
bootstrap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bootstrap)


class DoctorScopes(unittest.TestCase):
    def report(self, scope):
        config = load_config(ROOT / "configs/local.toml")
        def exists(name, path, kind="file"):
            optional = name.startswith("Scheduler") or name == "Gem5 verification binary"
            return Check(name, "FAIL" if optional else "PASS", str(path), kind)
        with patch("ace_echo.doctor._exists", side_effect=exists), patch("ace_echo.doctor.shutil.which", return_value="/tools/bin/tool"):
            return run_doctor(config, scope=scope)

    def test_fast_does_not_require_debug_or_scheduler(self):
        self.assertEqual(self.report("fast")["status"], "PASS")

    def test_software_does_not_require_debug(self):
        self.assertEqual(self.report("software")["status"], "PASS")

    def test_diagnostic_requires_debug(self):
        self.assertEqual(self.report("diagnostic")["status"], "FAIL")

    def test_full_preserves_existing_checks(self):
        self.assertEqual(self.report("full")["status"], "FAIL")

    def test_invalid_scope(self):
        with self.assertRaises(ValueError):
            self.report("imaginary")


class BootstrapTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.backend = ROOT / "configs/backends/venus1p0-64x512-300mhz.json"
        for rel in ["tools/llvm", "tools/gcc/bin", "components/toolchain/dsl", "workloads/5g_lite", ".ace-echo/deps/json/include/nlohmann"]:
            (self.root / rel).mkdir(parents=True)
        for name in ("clang", "opt", "llc", "llvm-objcopy", "llvm-objdump"):
            p = self.root / "tools/llvm" / name
            p.write_text("fake executable for setup-only test")
            p.chmod(0o755)
        gcc = self.root / "tools/gcc/bin/riscv32-unknown-elf-gcc"
        gcc.write_text("fake executable for setup-only test")
        gcc.chmod(0o755)
        (self.root / "components/toolchain/dsl/config.mk").write_text("fixture")
        (self.root / "workloads/5g_lite/task-index.json").write_text("{}")
        (self.root / ".ace-echo/deps/json/include/nlohmann/json.hpp").write_text("fixture header")
        self.tools = self.root / "tools.json"
        self.tools.write_text(json.dumps({"schema": "ace-echo-host-tools/v1", "venus_llvm_bin": str(self.root / "tools/llvm"), "riscv_gcc_root": str(self.root / "tools/gcc")}))

    def configure(self, output=None):
        def git(path, *args):
            if args[0] == "status":
                return ""
            return bootstrap.JSON_COMMIT if path.name == "json" else "a"*40
        with patch.object(bootstrap, "git", side_effect=git), patch.object(bootstrap.subprocess, "check_output", return_value=b"config.mk\0"):
            bootstrap.configure(self.root, self.tools, self.backend, output or self.root / ".ace-echo/host")

    def test_relocation_preserves_architecture_and_gates(self):
        self.configure()
        original = json.loads(self.backend.read_text())
        actual = json.loads((self.root / ".ace-echo/host/backend.json").read_text())
        for key in ("architecture", "abi", "execution_policy", "static_gates", "performance"):
            self.assertEqual(original[key], actual[key])
        for name, contract in original["clock_contracts"].items():
            if isinstance(contract, dict):
                for key, value in contract.items():
                    if key != "firmware":
                        self.assertEqual(value, actual["clock_contracts"][name][key])
        self.assertTrue(all(Path(p).is_absolute() for p in actual["paths"].values()))
        self.assertIn("RTL_NOT_INSTALLED", actual["paths"]["rtl_root"])
        self.assertEqual((self.root / "components/toolchain/dsl/config.mk").read_text(), "fixture")
        config = load_config(self.root / ".ace-echo/host/local.toml")
        self.assertEqual(config.projects.workload_root, self.root / "workloads/5g_lite")

    def test_no_overwrite(self):
        self.configure()
        with self.assertRaises(FileExistsError):
            self.configure()

    def test_output_escape_rejected(self):
        with self.assertRaises(ValueError):
            self.configure(self.root / "not-local-config")

    def test_missing_tool_rejected(self):
        (self.root / "tools/llvm/llc").unlink()
        with self.assertRaises(FileNotFoundError):
            self.configure()

    def test_missing_config_key_is_actionable(self):
        self.tools.write_text(json.dumps({"schema": "ace-echo-host-tools/v1"}))
        with self.assertRaisesRegex(ValueError, "venus_llvm_bin"):
            self.configure()

    def test_job_limit(self):
        with self.assertRaises(ValueError):
            bootstrap.build_gem5(self.root, 1000, "opt")

    def test_binary_receipt_mismatch_rejected(self):
        binary = self.root / "gem5.opt"
        binary.write_bytes(b"fixture")
        receipt = self.root / "receipt.json"
        receipt.write_text(json.dumps({"exit_code": 0, "source_status": "", "source_tree": "tree",
                                      "binary_sha256": "wrong", "argv": ["build/RISCV/gem5.opt"]}))
        with patch.object(bootstrap, "git", return_value="tree"), self.assertRaises(ValueError):
            bootstrap.install_gem5(self.root, binary, receipt, "opt")

    def test_install_same_source_binary_then_refuse_overwrite(self):
        binary = self.root / "gem5.opt"
        binary.write_bytes(b"fixture")
        receipt = self.root / "receipt.json"
        receipt.write_text(json.dumps({"exit_code": 0, "source_status": "", "source_tree": "tree",
                                      "binary_sha256": bootstrap.sha(binary), "argv": ["build/RISCV/gem5.opt"]}))
        with patch.object(bootstrap, "git", side_effect=lambda root, *args: "tree" if args[0] == "rev-parse" else ""):
            bootstrap.install_gem5(self.root, binary, receipt, "opt")
            self.assertEqual((self.root / "components/gem5/build/RISCV/gem5.opt").read_bytes(), binary.read_bytes())
            with self.assertRaises(FileExistsError):
                bootstrap.install_gem5(self.root, binary, receipt, "opt")


if __name__ == "__main__":
    unittest.main()
