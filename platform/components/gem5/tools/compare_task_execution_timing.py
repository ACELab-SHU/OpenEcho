#!/usr/bin/env python3
"""Compare RTL and gem5 task execution at the same lifecycle boundary.

RTL ``start execute`` -> ``execute complete`` is paired with gem5
``tile_started`` (or the legacy ``tile_start`` spelling) ->
``task_epilogue``.  Return DMA and tile release are reported
separately: including them only on the gem5 side creates very large, but
spurious, percentage errors for short tasks.

An RTL scheduler log may contain several sequential DAG executions.  They are
kept as separate sequences and the CLI selects the sequence whose task-id set
matches the gem5 replay.  This avoids silently mixing measurements from two
different DAGs that happen to share task ids.
"""

import argparse
import json
import re
from pathlib import Path


RTL_START = re.compile(
    r"^task (\d+) start execute at tile (\d+) at time (\d+(?:\.\d+)?)$")
RTL_COMPLETE = re.compile(
    r"^task (\d+) execute complete at tile (\d+) at time (\d+(?:\.\d+)?)$")


def read_rtl_sequences(path, time_unit_ps=1000.0):
    sequences = []
    records = {}
    for raw in path.read_text(
            encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        match = RTL_START.match(line)
        kind = "start"
        if match is None:
            match = RTL_COMPLETE.match(line)
            kind = "complete"
        if match is None:
            continue
        task = int(match.group(1))
        tile = int(match.group(2))
        timestamp = float(match.group(3)) * time_unit_ps / 1000.0
        # A testbench edge observer can see X->0/reset initialization as a
        # negedge at time zero.  It is not a task completion because no start
        # boundary exists.  Ignore only this universal simulation artifact;
        # later unmatched completions remain errors through require().
        if kind == "complete" and timestamp == 0 and task not in records:
            continue
        # A later DAG in the same scheduler log begins reusing task ids only
        # after the prior execution completed.  Split at that boundary instead
        # of treating it as a malformed duplicate event.
        if kind == "start" and task in records and "start" in records[task]:
            sequences.append(records)
            records = {}
        record = records.setdefault(task, {"tile": tile})
        if record["tile"] != tile:
            raise ValueError(f"RTL task {task} changed tile")
        if kind in record:
            raise ValueError(f"RTL task {task} has duplicate {kind}")
        record[kind] = timestamp
    if records:
        sequences.append(records)
    return sequences


def read_rtl(path, time_unit_ps=1000.0):
    """Read a log containing exactly one RTL DAG execution.

    Kept as the small, strict API used by unit tests and callers that know the
    log is single-DAG.  Multi-DAG callers should use ``read_rtl_sequences``.
    """
    sequences = read_rtl_sequences(path, time_unit_ps)
    if len(sequences) != 1:
        raise ValueError(
            f"RTL log contains {len(sequences)} task sequences; select one")
    return sequences[0]


def read_gem5(path, tick_ps):
    records = {}
    wanted = {
        "tile_start": "start",
        "tile_started": "start",
        "task_epilogue": "complete",
        "measured_task_timing": "released",
    }
    for line_number, raw in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw.strip():
            continue
        event = json.loads(raw)
        kind = wanted.get(event.get("event"))
        if kind is None:
            continue
        task = int(event["task_id"])
        record = records.setdefault(task, {})
        tile = event.get("tile_id", record.get("tile"))
        if tile is not None:
            if "tile" in record and record["tile"] != tile:
                raise ValueError(f"gem5 task {task} changed tile")
            record["tile"] = tile
        timestamp = float(event["tick"]) * tick_ps / 1000.0
        # Current traces intentionally emit both the native ``tile_started``
        # event and its legacy ``tile_start`` compatibility alias at the same
        # tick.  They are one boundary, not two starts.  A repeated boundary
        # at a different tick remains an error.
        if kind in record and kind == "start" and record[kind] == timestamp:
            continue
        if kind in record:
            raise ValueError(
                f"gem5 trace line {line_number}: task {task} has duplicate "
                f"{kind}")
        record[kind] = timestamp
    return records


def require(records, label, fields):
    if not records:
        raise ValueError(f"{label} contains no task timing records")
    for task, record in sorted(records.items()):
        missing = [field for field in fields if field not in record]
        if missing:
            raise ValueError(
                f"{label} task {task} lacks {', '.join(missing)}")


def compare(rtl, gem5, workload, rtl_sequences=1, selected_rtl_sequence=0):
    if set(rtl) != set(gem5):
        raise ValueError(
            f"task sets differ: RTL={sorted(rtl)} gem5={sorted(gem5)}")

    tasks = []
    for task in sorted(rtl):
        rtl_record = rtl[task]
        gem5_record = gem5[task]
        rtl_ns = rtl_record["complete"] - rtl_record["start"]
        gem5_ns = gem5_record["complete"] - gem5_record["start"]
        return_tail_ns = gem5_record.get(
            "released", gem5_record["complete"]) - gem5_record["complete"]
        delta_ns = gem5_ns - rtl_ns
        tasks.append({
            "task": task,
            "rtl_tile": rtl_record["tile"],
            "gem5_tile": gem5_record.get("tile"),
            "rtl_execution_ns": rtl_ns,
            "gem5_execution_ns": gem5_ns,
            "delta_ns": delta_ns,
            "error_percent": 100.0 * delta_ns / rtl_ns,
            "gem5_return_tail_ns": return_tail_ns,
        })

    rtl_sum = sum(task["rtl_execution_ns"] for task in tasks)
    gem5_sum = sum(task["gem5_execution_ns"] for task in tasks)
    absolute_errors = [abs(task["error_percent"]) for task in tasks]
    return {
        "schema": "venus-legacy-task-execution-timing-comparison/v1",
        "workload": workload,
        "status": "COMPLETE",
        "boundary": {
            "rtl": "start execute -> execute complete",
            "gem5": "tile_started -> task_epilogue",
            "time_unit": "ns",
            "excluded": "return DMA and tile release",
        },
        "coverage": {
            "rtl_sequences": rtl_sequences,
            "selected_rtl_sequence": selected_rtl_sequence,
            "rtl_declared_tasks": len(rtl),
            "rtl_completed_tasks": sum(
                "complete" in record for record in rtl.values()),
            "gem5_declared_tasks": len(gem5),
            "gem5_completed_tasks": sum(
                "complete" in record for record in gem5.values()),
            "compared_tasks": len(tasks),
        },
        "summary": {
            "rtl_execution_sum_ns": rtl_sum,
            "gem5_execution_sum_ns": gem5_sum,
            "execution_sum_error_percent": 100.0 * (gem5_sum - rtl_sum) / rtl_sum,
            "mean_absolute_task_error_percent": (
                sum(absolute_errors) / len(absolute_errors)),
            "max_absolute_task_error_percent": max(absolute_errors),
        },
        "tasks": tasks,
    }


def select_rtl_sequence(sequences, gem5):
    matches = [
        (index, sequence) for index, sequence in enumerate(sequences)
        if set(sequence) == set(gem5)
    ]
    if len(matches) != 1:
        raise ValueError(
            "expected exactly one RTL task sequence matching gem5 task ids "
            f"{sorted(gem5)}; found {[index for index, _ in matches]}")
    return matches[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-scheduler-log", type=Path, required=True)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    parser.add_argument("--gem5-tick-ps", type=float, default=1.0)
    parser.add_argument(
        "--rtl-time-unit-ps", type=float, default=1000.0,
        help=("picoseconds represented by one RTL log timestamp unit; the "
              "default preserves logs expressed in ns"),
    )
    parser.add_argument("--workload", default="")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    rtl_sequences = read_rtl_sequences(
        args.rtl_scheduler_log, args.rtl_time_unit_ps)
    gem5 = read_gem5(args.gem5_trace, args.gem5_tick_ps)
    selected_rtl_sequence, rtl = select_rtl_sequence(rtl_sequences, gem5)
    require(rtl, "RTL", ("tile", "start", "complete"))
    require(gem5, "gem5", ("tile", "start", "complete"))
    report = compare(
        rtl, gem5, args.workload, len(rtl_sequences), selected_rtl_sequence)
    rendered = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)


if __name__ == "__main__":
    main()
