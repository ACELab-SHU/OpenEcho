from pathlib import Path
import unittest


class IndependenceTests(unittest.TestCase):
    def test_runtime_config_has_no_old_runtime_project(self):
        root = Path(__file__).resolve().parents[1]
        text = (root / "configs/local.toml").read_text(encoding="utf-8")
        self.assertNotIn("Venus_3/gem5-freertos", text)
        self.assertNotIn("Venus_3/scheduler", text)
        self.assertNotIn("Venus_3/venus_soc", text)
        self.assertNotIn("MultiVemu/VEMU", text)

    def test_no_vemu_emulator_invocation_in_orchestrator(self):
        root = Path(__file__).resolve().parents[1] / "src/ace_echo"
        source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in root.rglob("*.py")
        )
        self.assertNotIn("Debug/Emulator", source)
        self.assertNotIn("make Emulator", source)


if __name__ == "__main__":
    unittest.main()
