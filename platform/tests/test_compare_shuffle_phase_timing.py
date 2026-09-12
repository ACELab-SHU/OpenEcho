import importlib.util
from pathlib import Path
import tempfile
import unittest


TOOL = (
    Path(__file__).parents[1] / "components/gem5/tools" /
    "compare_shuffle_phase_timing.py"
)
SPEC = importlib.util.spec_from_file_location("shuffle_compare", TOOL)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ShufflePhaseTimingTests(unittest.TestCase):
    def test_aggregate_comparison_preserves_phase_deltas(self):
        base = {
            "task": 3, "id": 1, "vm_r": 0, "vew": 0, "vl": 64,
            "start": 10, "complete": 20,
            "cycles": [0, 1, 0, 4, 5, 6, 1, 1, 0, 0, 0],
            "grants": [0, 0, 0, 64, 64, 64, 0, 0, 0, 0, 0],
        }
        gem5 = dict(base)
        gem5["cycles"] = [0, 1, 0, 5, 5, 6, 1, 1, 0, 0, 0]

        report = MODULE.compare([base], [gem5])

        self.assertTrue(report["all_grants_exact"])
        self.assertEqual(report["groups"][0]["cycle_delta"][3], 1)
        self.assertEqual(report["groups"][0]["rtl_total_cycles"], 18)
        self.assertEqual(report["groups"][0]["gem5_total_cycles"], 19)

    def test_read_phases_excludes_rtl_stage2_reduction_path(self):
        line = (
            "ACE_ECHO_SHUFFLE_PHASE id 0 vm_r 0 vew 0 vl 24 "
            "start 12 complete 20 cycles 0,1,3,0,0,0,0,1,0,0,0 "
            "grants 0,0,64,0,0,0,0,0,0,0,0\n"
            "ACE_ECHO_SHUFFLE_PHASE id 1 vm_r 1 vew 0 vl 24 "
            "start 13 complete 21 cycles 0,1,0,0,0,0,1,1,2,2,2 "
            "grants 0,0,0,0,0,0,0,0,24,24,24\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sim.log"
            path.write_text(line, encoding="utf-8")
            records, ignored = MODULE.read_phases(
                path, MODULE.RTL_PHASE,
                {7: {"start": 10, "complete": 30}},
            )

        self.assertEqual(len(records), 1)
        self.assertEqual(ignored, 1)


if __name__ == "__main__":
    unittest.main()
