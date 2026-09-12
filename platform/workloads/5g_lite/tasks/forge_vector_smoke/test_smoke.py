import json
from pathlib import Path
import struct
import tempfile
import unittest

from reference import bas, cases, golden
from run import compare, timing


class SmokeContractTests(unittest.TestCase):
    def test_all_frozen_cases(self):
        for a, b in cases().values():
            outputs = golden(a, b)
            self.assertEqual(outputs["restored"], struct.pack("<32h", *a))
            self.assertEqual(len(outputs["sum"]), 64)
            self.assertIn("Task_forgeRestore(sum, b)", bas(a, b))

    def test_out_of_domain_rejected(self):
        for a in ([0]*31, [1025]*32, [True]*32):
            with self.assertRaises(ValueError):
                golden(a, [0]*32)

    def test_corruption_and_truncation_fail(self):
        for actual in (b"123", b"12345", b"0234"):
            with self.assertRaises(ValueError):
                compare(b"1234", actual)

    def test_actual_clock_not_nominal_assumption(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "m5out").mkdir()
            (root / "m5out/config.ini").write_text("[seq]\ntype=VenusSequencer\nclk_domain=clock\n[clock]\nclock=3333\n")
            (root / "m5out/stats.txt").write_text("simFreq 1000000000000\nsimTicks 13332\n")
            events = [{"event": "tile_allocated", "task_id": 0, "tick": 3333},
                      {"event": "tile_released", "task_id": 0, "tick": 6666},
                      {"event": "tile_allocated", "task_id": 1, "tick": 6666},
                      {"event": "tile_released", "task_id": 1, "tick": 9999},
                      {"event": "dag_complete", "task_id": 0, "tick": 13332}]
            (root / "venus_dag_trace.jsonl").write_text("\n".join(map(json.dumps, events)))
            self.assertEqual(timing(root)["cycles"], 3)
            self.assertEqual(timing(root)["raw_ticks"], 9999)
            events.pop()
            (root / "venus_dag_trace.jsonl").write_text("\n".join(map(json.dumps, events)))
            with self.assertRaises(ValueError):
                timing(root)


if __name__ == "__main__":
    unittest.main()
