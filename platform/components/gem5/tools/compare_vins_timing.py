#!/usr/bin/env python3
"""Compare RTL and gem5 per-instruction Venus lifecycle timing.

The RTL full-DAG monitor numbers instructions locally within a task, while
gem5 uses a process-global instruction counter.  The result dump filenames
provide the authoritative task-to-global-counter mapping for gem5.
"""

import argparse
import json
import re
from pathlib import Path


RESULT_ID_RE = re.compile(r"_(\d+)(?:_vrf)?\.txt$")
IDENTITY_FIELDS = (
    "vl",
    "vew",
    "use_vs1",
    "vs1_head",
    "use_vs2",
    "vs2_head",
    "use_vd1",
    "vd1_head",
    "use_vd2",
    "vd2_head",
    "vm_r",
    "vm_w",
)


def read_rtl(rtl_dir: Path, task: int):
    records = []
    for path in sorted(rtl_dir.glob("venus_full_dag_perf_cluster*_tile*.jsonl")):
        with path.open() as stream:
            for line_number, raw in enumerate(stream, 1):
                event = json.loads(raw)
                if event.get("event") != "venus_instr":
                    continue
                if event.get("task_id") != task:
                    continue
                event["_path"] = str(path)
                event["_line"] = line_number
                records.append(event)
    records.sort(key=lambda event: event["task_instr_counter"])
    for expected, event in enumerate(records):
        actual = event["task_instr_counter"]
        if actual != expected:
            raise ValueError(
                f"RTL task-local sequence is not contiguous: "
                f"expected {expected}, found {actual}"
            )
    return records


def read_gem5(
    monitor_path: Path,
    result_root: Path,
    task: int,
    counter_start=None,
    counter_count=None,
):
    with monitor_path.open() as stream:
        payload = json.load(stream)
    records = {
        event["venus_instr_counter"]: event
        for event in payload[0]["Venus_instr"]
    }

    if counter_start is not None:
        if counter_count is None:
            raise ValueError(
                "--gem5-counter-count is required with "
                "--gem5-counter-start"
            )
        ids = set(range(counter_start, counter_start + counter_count))
    else:
        if result_root is None:
            raise ValueError(
                "--gem5-results is required unless "
                "--gem5-counter-start/--gem5-counter-count are supplied"
            )
        result_dir = result_root / f"task_{task}"
        ids = set()
        for path in result_dir.glob("*.txt"):
            match = RESULT_ID_RE.search(path.name)
            if match:
                ids.add(int(match.group(1)))
        if not ids:
            raise ValueError(f"no gem5 result IDs found in {result_dir}")

    missing = sorted(ids - records.keys())
    if missing:
        raise ValueError(
            f"gem5 monitor is missing result IDs: {missing[:8]}"
        )
    return [records[instr_id] for instr_id in sorted(ids)]


def compare(rtl, gem5, ticks_per_cycle):
    if len(rtl) != len(gem5):
        divergence = {
            "kind": "instruction_count",
            "rtl": len(rtl),
            "gem5": len(gem5),
        }
        return divergence, {"instruction_count": divergence}
    if not rtl:
        return None, {}

    rtl_fire_origin = rtl[0]["fire_tick"]
    gem5_fire_origin = gem5[0]["fire_tick"] // ticks_per_cycle
    dimensions = {}
    chronological = []
    for sequence, (rtl_event, gem5_event) in enumerate(zip(rtl, gem5)):
        identity_difference = {
            field: {
                "rtl": rtl_event.get(field),
                "gem5": gem5_event.get(field),
            }
            for field in IDENTITY_FIELDS
            if rtl_event.get(field) != gem5_event.get(field)
        }
        if identity_difference:
            divergence = {
                "kind": "instruction_identity",
                "sequence": sequence,
                "rtl_op": rtl_event.get("op_s"),
                "gem5_op": gem5_event.get("op_s"),
                "gem5_instruction_id":
                    gem5_event.get("venus_instr_counter"),
                "fields": identity_difference,
            }
            dimensions.setdefault("instruction_identity", divergence)

        rtl_timing = {
            "fire": rtl_event["fire_tick"] - rtl_fire_origin,
            "duration": rtl_event["consumed_ticks"],
            "recycle": rtl_event["recycle_tick"] - rtl_fire_origin,
        }
        gem5_timing = {
            "fire":
                gem5_event["fire_tick"] // ticks_per_cycle -
                gem5_fire_origin,
            "duration":
                gem5_event["consumed_ticks"] // ticks_per_cycle,
            "recycle":
                gem5_event["recycle_tick"] // ticks_per_cycle -
                gem5_fire_origin,
        }
        for dimension in ("fire", "duration", "recycle"):
            if rtl_timing[dimension] == gem5_timing[dimension]:
                continue
            divergence = {
                "kind": f"{dimension}_timing",
                "sequence": sequence,
                "rtl_op": rtl_event.get("op_s"),
                "gem5_op": gem5_event.get("op_s"),
                "gem5_instruction_id":
                    gem5_event.get("venus_instr_counter"),
                "rtl_cycle": rtl_timing[dimension],
                "gem5_cycle": gem5_timing[dimension],
                "delta":
                    gem5_timing[dimension] - rtl_timing[dimension],
            }
            dimensions.setdefault(dimension, divergence)
            if dimension != "duration":
                chronological.append((
                    min(rtl_timing[dimension], gem5_timing[dimension]),
                    0 if dimension == "fire" else 1,
                    divergence,
                ))

    if "instruction_identity" in dimensions:
        first = dimensions["instruction_identity"]
    elif chronological:
        first = min(chronological, key=lambda item: (item[0], item[1]))[2]
    elif "duration" in dimensions:
        first = dimensions["duration"]
    else:
        first = None
    return first, dimensions


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-dir", type=Path, required=True)
    parser.add_argument("--gem5-monitor", type=Path, required=True)
    parser.add_argument("--gem5-results", type=Path)
    parser.add_argument(
        "--gem5-counter-start", type=int,
        help="first global gem5 VINS counter for a bounded task replay",
    )
    parser.add_argument(
        "--gem5-counter-count", type=int,
        help="number of consecutive gem5 VINS counters to compare",
    )
    parser.add_argument("--task", type=int, required=True)
    parser.add_argument(
        "--gem5-task", type=int,
        help="gem5 replay task id when it differs from the RTL task id",
    )
    parser.add_argument("--gem5-ticks-per-cycle", type=int, default=2000)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    rtl = read_rtl(args.rtl_dir, args.task)
    if args.gem5_counter_count is not None:
        if args.gem5_counter_count > len(rtl):
            raise ValueError(
                "--gem5-counter-count exceeds the RTL task instruction "
                f"count: {args.gem5_counter_count} > {len(rtl)}"
            )
        rtl = rtl[:args.gem5_counter_count]
    gem5_task = args.task if args.gem5_task is None else args.gem5_task
    gem5 = read_gem5(
        args.gem5_monitor,
        args.gem5_results,
        gem5_task,
        args.gem5_counter_start,
        args.gem5_counter_count,
    )
    divergence, dimensions = compare(
        rtl, gem5, args.gem5_ticks_per_cycle
    )
    report = {
        "schema": "venus-vins-timing-comparison/v1",
        "task": args.task,
        "gem5_task": gem5_task,
        "rtl_instruction_count": len(rtl),
        "gem5_instruction_count": len(gem5),
        "gem5_ticks_per_cycle": args.gem5_ticks_per_cycle,
        "pass": divergence is None,
        "first_divergence": divergence,
        "first_divergence_by_dimension": dimensions,
    }
    rendered = json.dumps(report, indent=2, sort_keys=True)
    if args.output:
        args.output.write_text(rendered + "\n")
    print(rendered)
    raise SystemExit(0 if divergence is None else 1)


if __name__ == "__main__":
    main()
