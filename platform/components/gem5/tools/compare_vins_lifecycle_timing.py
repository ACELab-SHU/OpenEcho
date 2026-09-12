#!/usr/bin/env python3
"""Compare RTL and gem5 vector-instruction fire/recycle lifecycles.

The RTL input is the backend-declared run-local edge log.  Instructions are
matched by task and chronological ordinal; the running-ID is retained as an
independent conformance check because the eight IDs are intentionally reused.
"""

from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict, deque
from pathlib import Path


RTL_TASK_START = re.compile(
    r"^task (\d+) start execute at tile (\d+) at time (\d+(?:\.\d+)?)$")
RTL_TASK_COMPLETE = re.compile(
    r"^task (\d+) execute complete at tile (\d+) at time "
    r"(\d+(?:\.\d+)?)$")
RTL_VINS_START = re.compile(
    r"^vins (\d+) start at task (\d+) tile (\d+) at time "
    r"(\d+(?:\.\d+)?)$")
RTL_VINS_COMPLETE = re.compile(
    r"^vins (\d+) complete at task (\d+) tile (\d+) at time "
    r"(\d+(?:\.\d+)?)$")
RTL_BOUND_VINS = re.compile(
    r"ACE_ECHO_VINS (\d+) (start|complete) tile (\d+) time "
    r"(\d+(?:\.\d+)?)")


def read_rtl(path: Path, time_unit_ps: float,
             vins_path: Path | None = None) -> dict[int, list[dict]]:
    task_starts: dict[int, float] = {}
    task_intervals: list[dict] = []
    open_tasks: dict[tuple[int, int], deque[dict]] = defaultdict(deque)
    task_lines = path.read_text(
        encoding="utf-8", errors="replace"
    ).splitlines()
    for line_number, raw in enumerate(task_lines, 1):
        line = raw.strip()
        match = RTL_TASK_START.match(line)
        if match:
            task, tile = map(int, match.groups()[:2])
            timestamp_ps = float(match.group(3)) * time_unit_ps
            task_starts[task] = timestamp_ps
            interval = {
                "task": task,
                "tile": tile,
                "start_ps": timestamp_ps,
                "line": line_number,
            }
            open_tasks[(task, tile)].append(interval)
            task_intervals.append(interval)
            continue
        match = RTL_TASK_COMPLETE.match(line)
        if match:
            task, tile = map(int, match.groups()[:2])
            timestamp_ps = float(match.group(3)) * time_unit_ps
            key = (task, tile)
            if timestamp_ps == 0 and not open_tasks[key]:
                continue
            if not open_tasks[key]:
                raise ValueError(
                    f"RTL line {line_number}: unmatched task completion {key}"
                )
            open_tasks[key].popleft()["complete_ps"] = timestamp_ps
    for intervals in open_tasks.values():
        for interval in intervals:
            interval["complete_ps"] = float("inf")

    pending: dict[tuple[int, int, int], deque[dict]] = defaultdict(deque)
    records: dict[int, list[dict]] = defaultdict(list)
    for task in task_starts:
        records.setdefault(task, [])
    raw_events: list[dict] = []
    vins_lines = (
        vins_path.read_text(encoding="utf-8", errors="replace").splitlines()
        if vins_path is not None else task_lines
    )
    for line_number, raw in enumerate(
            vins_lines, 1):
        line = raw.strip()
        match = RTL_VINS_START.match(line)
        kind = "start"
        if match is None:
            match = RTL_VINS_COMPLETE.match(line)
            kind = "complete"
        if match is not None:
            running_id, task, tile = map(int, match.groups()[:3])
            timestamp_ps = float(match.group(4)) * time_unit_ps
        else:
            bound_match = RTL_BOUND_VINS.search(line)
            if bound_match is None:
                continue
            running_id = int(bound_match.group(1))
            kind = bound_match.group(2)
            tile = int(bound_match.group(3))
            timestamp_ps = float(bound_match.group(4)) * time_unit_ps
            candidates = [
                interval for interval in task_intervals
                if interval["tile"] == tile
                and interval["start_ps"] <= timestamp_ps
                and timestamp_ps <= interval.get("complete_ps", -1)
            ]
            if timestamp_ps == 0 and not candidates:
                continue
            # A bind-observer simulator log can cover many DAGs while the
            # task-edge log intentionally selects one focused interval.  VINS
            # outside every selected interval are unrelated evidence and must
            # not be assigned to the focused task.
            if not candidates and vins_path is not None:
                continue
            if len(candidates) != 1:
                raise ValueError(
                    f"RTL VINS line {line_number}: expected one active task "
                    f"for tile {tile} at {timestamp_ps} ps; found "
                    f"{len(candidates)}"
                )
            task = int(candidates[0]["task"])
        raw_events.append({
            "running_id": running_id,
            "task": task,
            "tile": tile,
            "timestamp_ps": timestamp_ps,
            "kind": kind,
            "line": line_number,
        })

    for event in raw_events:
        running_id = event["running_id"]
        task = event["task"]
        tile = event["tile"]
        timestamp_ps = event["timestamp_ps"]
        kind = event["kind"]
        line_number = event["line"]
        key = (task, tile, running_id)
        if kind == "start":
            pending[key].append({
                "running_id": running_id,
                "tile": tile,
                "start_ps": timestamp_ps,
                "line": line_number,
            })
            continue
        if timestamp_ps == 0 and not pending[key]:
            continue
        if not pending[key]:
            raise ValueError(
                f"RTL line {line_number}: unmatched VINS completion {key}")
        record = pending[key].popleft()
        record["complete_ps"] = timestamp_ps
        records[task].append(record)
    unmatched = {key: list(value) for key, value in pending.items() if value}
    if unmatched:
        raise ValueError(f"RTL contains unmatched VINS starts: {unmatched}")
    for task, task_records in records.items():
        if task not in task_starts:
            raise ValueError(f"RTL VINS task {task} has no task start")
        task_records.sort(key=lambda record: (record["start_ps"], record["line"]))
        for ordinal, record in enumerate(task_records):
            record["ordinal"] = ordinal
            record["relative_start_ps"] = (
                record["start_ps"] - task_starts[task]
            )
    return dict(records)


def read_gem5(trace_path: Path, monitor_path: Path,
              tick_ps: float) -> dict[int, list[dict]]:
    bounds: dict[int, dict[str, float]] = defaultdict(dict)
    for raw in trace_path.read_text(encoding="utf-8").splitlines():
        if not raw.strip():
            continue
        event = json.loads(raw)
        task = int(event.get("task_id", -1))
        if event.get("event") in {"tile_start", "tile_started"}:
            bounds[task]["start"] = float(event["tick"])
        elif event.get("event") == "task_epilogue":
            bounds[task]["complete"] = float(event["tick"])
    monitor = json.loads(monitor_path.read_text(encoding="utf-8"))
    instructions = monitor[0]["Venus_instr"]
    records: dict[int, list[dict]] = {}
    for task, boundary in sorted(bounds.items()):
        if "start" not in boundary or "complete" not in boundary:
            continue
        selected = [
            instruction for instruction in instructions
            if boundary["start"] <= float(instruction["fire_tick"])
            <= boundary["complete"]
        ]
        selected.sort(key=lambda instruction: (
            float(instruction["fire_tick"]),
            int(instruction["venus_instr_counter"]),
        ))
        records[task] = [{
            "ordinal": ordinal,
            "running_id": int(instruction["id"]),
            "op": instruction["op_s"],
            "vfu": instruction["vfu_s"],
            "vl": int(instruction["vl"]),
            "vew": instruction["vew_s"],
            "start_ps": float(instruction["fire_tick"]) * tick_ps,
            "complete_ps": float(instruction["recycle_tick"]) * tick_ps,
            "relative_start_ps": (
                float(instruction["fire_tick"] - boundary["start"])
                * tick_ps
            ),
        } for ordinal, instruction in enumerate(selected)]
    return records


def compare(rtl: dict[int, list[dict]], gem5: dict[int, list[dict]]) -> dict:
    if set(rtl) != set(gem5):
        raise ValueError(
            f"VINS task sets differ: RTL={sorted(rtl)} gem5={sorted(gem5)}")
    tasks = []
    for task in sorted(rtl):
        rtl_records = rtl[task]
        gem5_records = gem5[task]
        if len(rtl_records) != len(gem5_records):
            raise ValueError(
                f"task {task} instruction counts differ: "
                f"RTL={len(rtl_records)} gem5={len(gem5_records)}")
        rows = []
        for rtl_record, gem5_record in zip(rtl_records, gem5_records):
            rtl_duration = (
                rtl_record["complete_ps"] - rtl_record["start_ps"]
            ) / 1000.0
            gem5_duration = (
                gem5_record["complete_ps"] - gem5_record["start_ps"]
            ) / 1000.0
            rows.append({
                "ordinal": rtl_record["ordinal"],
                "rtl_running_id": rtl_record["running_id"],
                "gem5_running_id": gem5_record["running_id"],
                "running_id_match": (
                    rtl_record["running_id"] == gem5_record["running_id"]
                ),
                "op": gem5_record["op"],
                "vfu": gem5_record["vfu"],
                "vl": gem5_record["vl"],
                "vew": gem5_record["vew"],
                "rtl_relative_start_ns": (
                    rtl_record["relative_start_ps"] / 1000.0
                ),
                "gem5_relative_start_ns": (
                    gem5_record["relative_start_ps"] / 1000.0
                ),
                "start_delta_ns": (
                    gem5_record["relative_start_ps"]
                    - rtl_record["relative_start_ps"]
                ) / 1000.0,
                "rtl_duration_ns": rtl_duration,
                "gem5_duration_ns": gem5_duration,
                "duration_delta_ns": gem5_duration - rtl_duration,
            })
        tasks.append({"task": task, "instructions": rows})
    return {
        "schema": "venus-vins-lifecycle-timing-comparison/v1",
        "boundary": "sequencer running-ID assertion -> deassertion",
        "tasks": tasks,
    }


def select_task(records: dict[int, list[dict]], task: int | None,
                logical_task: int = 0) -> dict[int, list[dict]]:
    """Select one measured task and give both traces a common logical ID.

    Focused replay manifests commonly renumber an extracted task to zero,
    while the immutable RTL oracle retains its original DAG task ID.  This
    helper changes only comparison keys; instruction order, running IDs and
    timestamps remain untouched.
    """
    if task is None:
        return records
    if task not in records:
        raise ValueError(
            f"selected task {task} is absent; available={sorted(records)}")
    return {logical_task: records[task]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-edge-log", type=Path, required=True)
    parser.add_argument(
        "--rtl-vins-log", type=Path,
        help="Optional simulator log containing bind-observer VINS edges",
    )
    parser.add_argument("--rtl-time-unit-ps", type=float, default=1.0)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    parser.add_argument("--gem5-monitor", type=Path, required=True)
    parser.add_argument("--gem5-tick-ps", type=float, default=1.0)
    parser.add_argument(
        "--rtl-task", type=int,
        help="Select one RTL task and remap it to logical task zero",
    )
    parser.add_argument(
        "--gem5-task", type=int,
        help="Select one gem5 task and remap it to logical task zero",
    )
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = compare(
        select_task(read_rtl(
            args.rtl_edge_log, args.rtl_time_unit_ps, args.rtl_vins_log
        ), args.rtl_task),
        select_task(read_gem5(
            args.gem5_trace, args.gem5_monitor, args.gem5_tick_ps
        ), args.gem5_task),
    )
    rendered = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)


if __name__ == "__main__":
    main()
