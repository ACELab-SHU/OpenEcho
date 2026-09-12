import json
from pathlib import Path
import tempfile
import unittest

from ace_echo.adapters.toolchain import ToolchainAdapter


class TaskIndexTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.location = 'applications/rx/dags/PBCH/operators/fft'
        self.target = self.root / self.location
        self.target.mkdir(parents=True)
        (self.target / 'fft.bas').write_text('finish\n')
        (self.target / 'Task_fft.c').write_text('int Task_fft(void) { return 0; }\n')
        self.index = self.root / 'task-index.json'
        self.save(self.location)

    def save(self, location):
        self.index.write_text(json.dumps({'schema': 'echo-hub-task-index/v1', 'tasks': {'fft': {'path': location}}}))

    def test_logical_name_resolves_to_grouped_dag(self):
        actual = ToolchainAdapter._workload_path(self.root, 'tasks/fft')
        self.assertEqual(actual, self.target)
        self.assertEqual(ToolchainAdapter._dag_source(actual, 'fft'), self.target / 'fft.bas')

    def test_original_source_manifest_resolves_without_rewriting_frozen_json(self):
        manifest = self.target / 'task-sources.json'
        manifest.write_text(json.dumps({'schema': 'ace-echo-task-source-set/v1', 'sources': ['tasks/fft/Task_fft.c']}))
        frozen = manifest.read_bytes()
        self.assertEqual(ToolchainAdapter._task_sources(self.root, self.target), [self.target / 'Task_fft.c'])
        self.assertEqual(manifest.read_bytes(), frozen)

    def test_no_index_preserves_legacy_layout(self):
        self.index.unlink()
        self.assertEqual(ToolchainAdapter._workload_path(self.root, 'tasks/fft'), self.root / 'tasks/fft')

    def test_index_escape_rejected(self):
        for entry in ['../outside', '/tmp/outside']:
            self.save(entry)
            with self.assertRaises(ValueError):
                ToolchainAdapter._workload_path(self.root, 'tasks/fft')

    def test_bad_schema_rejected(self):
        self.index.write_text('{"schema":"wrong","tasks":{}}')
        with self.assertRaises(ValueError):
            ToolchainAdapter._workload_path(self.root, 'tasks/fft')

    def test_direct_escape_rejected(self):
        with self.assertRaises(ValueError):
            ToolchainAdapter._workload_path(self.root, '../outside')


if __name__ == '__main__':
    unittest.main()
