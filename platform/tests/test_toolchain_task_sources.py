import json
from pathlib import Path
import tempfile
import unittest

from ace_echo.adapters.toolchain import ToolchainAdapter


class ToolchainTaskSourceTests(unittest.TestCase):
    def test_manifest_resolves_common_sources_in_declared_order(self):
        with tempfile.TemporaryDirectory() as temporary:
            workload = Path(temporary)
            common = workload / "tasks" / "common"
            target = workload / "tasks" / "composition"
            common.mkdir(parents=True)
            target.mkdir(parents=True)
            first = common / "Task_first.c"
            second = common / "Task_second.c"
            first.write_text("int Task_first(void) { return 0; }\n")
            second.write_text("int Task_second(void) { return 0; }\n")
            (target / "task-sources.json").write_text(json.dumps({
                "schema": "ace-echo-task-source-set/v1",
                "sources": [
                    "tasks/common/Task_second.c",
                    "tasks/common/Task_first.c",
                ],
            }))

            sources = ToolchainAdapter._task_sources(workload, target)

        self.assertEqual([source.name for source in sources],
                         ["Task_second.c", "Task_first.c"])

    def test_manifest_rejects_sources_outside_workload(self):
        with tempfile.TemporaryDirectory() as temporary:
            workload = Path(temporary) / "workload"
            target = workload / "tasks" / "composition"
            target.mkdir(parents=True)
            (target / "task-sources.json").write_text(json.dumps({
                "schema": "ace-echo-task-source-set/v1",
                "sources": ["../outside.c"],
            }))

            with self.assertRaises(ValueError):
                ToolchainAdapter._task_sources(workload, target)

    def test_dag_source_defaults_to_target_named_bas(self):
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "demo"
            target.mkdir()
            expected = target / "demo.bas"
            expected.write_text("finish\n")

            actual = ToolchainAdapter._dag_source(target, "demo")

        self.assertEqual(actual, expected)

    def test_dag_source_manifest_selects_runtime_input_variant(self):
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "demo"
            target.mkdir()
            expected = target / "runtime.bas"
            expected.write_text("dag_input short samples[1]\n")
            (target / "dag-source.json").write_text(json.dumps({
                "schema": "ace-echo-dag-source/v1",
                "source": "runtime.bas",
            }))

            actual = ToolchainAdapter._dag_source(target, "demo")

        self.assertEqual(actual, expected.resolve())

    def test_dag_source_manifest_rejects_escape(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            target = root / "demo"
            target.mkdir()
            (root / "outside.bas").write_text("finish\n")
            (target / "dag-source.json").write_text(json.dumps({
                "schema": "ace-echo-dag-source/v1",
                "source": "../outside.bas",
            }))

            with self.assertRaises(ValueError):
                ToolchainAdapter._dag_source(target, "demo")


if __name__ == "__main__":
    unittest.main()
