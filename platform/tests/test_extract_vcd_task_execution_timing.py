from __future__ import annotations

from pathlib import Path
import importlib.util
import tempfile
import unittest


TOOL = (Path(__file__).parents[1] / "components/gem5/tools" /
        "extract_vcd_task_execution_timing.py")
SPEC = importlib.util.spec_from_file_location("extract_vcd_timing", TOOL)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class ExtractVcdTaskExecutionTimingTests(unittest.TestCase):
    def test_extracts_matched_edges_and_same_timestamp_task_clear(self) -> None:
        vcd = """$timescale 100 ps $end
$scope module testbench $end
$scope module tb $end
$scope module dut $end
$scope module u_venus_cluster0 $end
$scope module u_venus_cluster0_tile0 $end
$var wire 1 ! tile_soft_reset_n $end
$scope module u_venus_tile_manager $end
$var wire 6 \" venustile_currenttaskidreg $end
$upscope $end
$upscope $end
$upscope $end
$upscope $end
$upscope $end
$enddefinitions $end
#0
x!
bxxxxxx \"
#10
0!
b000011 \"
#25
1!
#1025
0!
b000000 \"
"""
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "task.vcd"
            path.write_text(vcd, encoding="utf-8")
            self.assertEqual(MODULE.extract(path), [
                "task 3 start execute at tile 0 at time 2.5",
                "task 3 execute complete at tile 0 at time 102.5",
            ])


if __name__ == "__main__":
    unittest.main()
