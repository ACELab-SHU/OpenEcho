from pathlib import Path
import tempfile
import unittest

from ace_echo.config import ConfigError, load_config


class ConfigTests(unittest.TestCase):
    def test_checked_in_config_uses_owned_runtime_components(self):
        root = Path(__file__).resolve().parents[1]
        config = load_config(root / "configs/local.toml")
        self.assertEqual(config.projects.gem5_root, root / "components/gem5")
        self.assertEqual(config.projects.scheduler_root,
                         root / "components/scheduler")
        self.assertEqual(config.projects.toolchain_root,
                         root / "components/toolchain")
        self.assertEqual(config.projects.workload_root,
                         root / "workloads/5g_lite")
        self.assertNotIn("Venus_3", str(config.projects.gem5_root))

    def test_rejects_unknown_schema(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.toml"
            path.write_text("schema_version = 2\n", encoding="utf-8")
            with self.assertRaises(ConfigError):
                load_config(path)


if __name__ == "__main__":
    unittest.main()
