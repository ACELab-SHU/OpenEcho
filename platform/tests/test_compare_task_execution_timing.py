#!/usr/bin/env python3

import sys
import tempfile
import unittest
from pathlib import Path


TOOLS = Path(__file__).resolve().parents[1] / "components/gem5/tools"
sys.path.insert(0, str(TOOLS))

from compare_task_execution_timing import (  # noqa: E402
    read_gem5,
    read_rtl,
    read_rtl_sequences,
    select_rtl_sequence,
)


class CompareTaskExecutionTimingTest(unittest.TestCase):
    def test_picosecond_edge_log_ignores_only_reset_completion(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "task_execution_timing.log"
            path.write_text(
                "task 0 execute complete at tile 0 at time 0\n"
                "vins 0 start at task 0 tile 0 at time 1000\n"
                "task 0 start execute at tile 0 at time 1234.5\n"
                "task 0 execute complete at tile 0 at time 3234.5\n",
                encoding="utf-8",
            )

            records = read_rtl(path, time_unit_ps=1.0)

            self.assertEqual(records, {
                0: {"tile": 0, "start": 1.2345, "complete": 3.2345},
            })

    def test_native_tile_started_event_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "trace.jsonl"
            path.write_text(
                '{"event":"tile_started","task_id":3,"tile_id":1,'
                '"tick":1000}\n'
                '{"event":"task_epilogue","task_id":3,"tile_id":1,'
                '"tick":3000}\n',
                encoding="utf-8",
            )

            self.assertEqual(read_gem5(path, tick_ps=1.0), {
                3: {"tile": 1, "start": 1.0, "complete": 3.0},
            })

    def test_same_tick_tile_start_alias_is_not_a_duplicate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "trace.jsonl"
            path.write_text(
                '{"event":"tile_started","task_id":3,"tile_id":1,'
                '"tick":1000}\n'
                '{"event":"tile_start","task_id":3,"tile_id":1,'
                '"tick":1000}\n'
                '{"event":"task_epilogue","task_id":3,"tile_id":1,'
                '"tick":3000}\n',
                encoding="utf-8",
            )

            self.assertEqual(read_gem5(path, tick_ps=1.0), {
                3: {"tile": 1, "start": 1.0, "complete": 3.0},
            })

    def test_multi_dag_rtl_log_selects_matching_task_set(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "task_execution_timing.log"
            path.write_text(
                "task 0 start execute at tile 0 at time 1000\n"
                "task 0 execute complete at tile 0 at time 2000\n"
                "task 1 start execute at tile 0 at time 2100\n"
                "task 1 execute complete at tile 0 at time 3000\n"
                "task 0 start execute at tile 0 at time 4000\n"
                "task 0 execute complete at tile 0 at time 5000\n",
                encoding="utf-8",
            )

            sequences = read_rtl_sequences(path, time_unit_ps=1.0)
            index, selected = select_rtl_sequence(
                sequences, {0: {"start": 0, "complete": 1}})

            self.assertEqual(len(sequences), 2)
            self.assertEqual(index, 1)
            self.assertEqual(set(selected), {0})


if __name__ == "__main__":
    unittest.main()
