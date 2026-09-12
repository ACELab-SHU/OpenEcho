#!/usr/bin/env python3
"""Compare backend-neutral Shuffle FSM phase/grant summaries.

The RTL side is emitted by a run-local bind observer.  The gem5 side is
emitted by ``VENUS_GEM5_SHUFFLE_PHASE_SUMMARY=1``.  Records are assigned to
tasks only by their measured task execution intervals; no task, PC, address,
or DAG identity participates in the timing model.
"""

import argparse
from collections import defaultdict
import json
import re
from pathlib import Path


RTL_PHASE = re.compile(
    r"ACE_ECHO_SHUFFLE_PHASE id (\d+) vm_r (\d+) vew (\d+) vl (\d+) "
    r"start (\d+) complete (\d+) cycles ([0-9,]+) grants ([0-9,]+)"
)
GEM5_PHASE = re.compile(
    r"ACE_ECHO_GEM5_SHUFFLE_PHASE id (\d+) vm_r (\d+) vew (\d+) "
    r"vl (\d+) start (\d+) complete (\d+) cycles ([0-9,]+) "
    r"grants ([0-9,]+)"
)
RTL_START = re.compile(
    r"^task (\d+) start execute at tile \d+ at time (\d+)$"
)
RTL_COMPLETE = re.compile(
    r"^task (\d+) execute complete at tile \d+ at time (\d+)$"
)


def read_rtl_task_intervals(path):
    intervals = {}
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        match = RTL_START.match(line)
        field = "start"
        if match is None:
            match = RTL_COMPLETE.match(line)
            field = "complete"
        if match is None:
            continue
        task = int(match.group(1))
        timestamp = int(match.group(2))
        if field == "complete" and timestamp == 0 and task not in intervals:
            continue
        intervals.setdefault(task, {})[field] = timestamp
    return intervals


def read_gem5_task_intervals(path):
    intervals = {}
    wanted = {"tile_start": "start", "task_epilogue": "complete"}
    for raw in path.read_text(encoding="utf-8").splitlines():
        if not raw.strip():
            continue
        event = json.loads(raw)
        field = wanted.get(event.get("event"))
        if field is None:
            continue
        intervals.setdefault(int(event["task_id"]), {})[field] = int(
            event["tick"]
        )
    return intervals


def task_for_timestamp(intervals, timestamp):
    matches = [
        task for task, interval in intervals.items()
        if interval.get("start", timestamp + 1) <= timestamp
        <= interval.get("complete", timestamp - 1)
    ]
    if len(matches) != 1:
        raise ValueError(
            f"phase timestamp {timestamp} belongs to {len(matches)} tasks"
        )
    return matches[0]


def read_phases(path, pattern, intervals):
    records = []
    ignored_non_gather_scatter = 0
    text = path.read_text(encoding="utf-8", errors="replace")
    for match in pattern.finditer(text):
        cycles = [int(value) for value in match.group(7).split(",")]
        grants = [int(value) for value in match.group(8).split(",")]
        if len(cycles) != 11 or len(grants) != 11:
            raise ValueError("Shuffle summary must contain states 0 through 10")
        # STAGE2 is the reduction/CLBMV path in the RTL Shuffle engine, not
        # the three-grant gather/scatter pipeline reported by gem5's
        # VenusShufflePipline.  A task can finish with such an operation, so
        # the bind observer sees it even though it is intentionally outside
        # this comparator's scope.  Classify by the measured FSM state, never
        # by task, PC, address, or instruction payload.
        if cycles[2] != 0 or grants[2] != 0:
            ignored_non_gather_scatter += 1
            continue
        start = int(match.group(5))
        records.append({
            "task": task_for_timestamp(intervals, start),
            "id": int(match.group(1)),
            "vm_r": int(match.group(2)),
            "vew": int(match.group(3)),
            "vl": int(match.group(4)),
            "start": start,
            "complete": int(match.group(6)),
            "cycles": cycles,
            "grants": grants,
        })
    if not records:
        raise ValueError(f"no Shuffle phase summaries in {path}")
    return records, ignored_non_gather_scatter


def aggregate(records):
    groups = defaultdict(lambda: {
        "count": 0, "cycles": [0] * 11, "grants": [0] * 11,
    })
    for record in records:
        key = (
            record["task"], record["vm_r"], record["vew"], record["vl"]
        )
        group = groups[key]
        group["count"] += 1
        group["cycles"] = [
            left + right for left, right in
            zip(group["cycles"], record["cycles"])
        ]
        group["grants"] = [
            left + right for left, right in
            zip(group["grants"], record["grants"])
        ]
    return groups


def compare(rtl_records, gem5_records, ignored_rtl=0, ignored_gem5=0):
    rtl = aggregate(rtl_records)
    gem5 = aggregate(gem5_records)
    if set(rtl) != set(gem5):
        only_rtl = sorted(set(rtl) - set(gem5))
        only_gem5 = sorted(set(gem5) - set(rtl))
        raise ValueError(
            f"Shuffle phase group sets differ: RTL-only={only_rtl}, "
            f"gem5-only={only_gem5}"
        )
    groups = []
    for key in sorted(rtl):
        rtl_group = rtl[key]
        gem5_group = gem5[key]
        if rtl_group["count"] != gem5_group["count"]:
            raise ValueError(f"Shuffle command count differs for {key}")
        rtl_total = sum(rtl_group["cycles"])
        gem5_total = sum(gem5_group["cycles"])
        groups.append({
            "task": key[0],
            "vm_r": key[1],
            "vew": key[2],
            "vl": key[3],
            "count": rtl_group["count"],
            "rtl_cycles": rtl_group["cycles"],
            "gem5_cycles": gem5_group["cycles"],
            "cycle_delta": [
                right - left for left, right in
                zip(rtl_group["cycles"], gem5_group["cycles"])
            ],
            "rtl_grants": rtl_group["grants"],
            "gem5_grants": gem5_group["grants"],
            "grant_exact": rtl_group["grants"] == gem5_group["grants"],
            "rtl_total_cycles": rtl_total,
            "gem5_total_cycles": gem5_total,
            "total_cycle_error_percent": (
                100.0 * (gem5_total - rtl_total) / rtl_total
            ),
        })
    return {
        "schema": "venus-shuffle-phase-timing-comparison/v1",
        "phase_order": [
            "IDLE", "STAGE1", "STAGE2", "STAGE3", "STAGE4",
            "STAGE5", "STAGE6", "WAIT", "STAGE7", "STAGE8",
            "STAGE9",
        ],
        "group_count": len(groups),
        "command_count": sum(group["count"] for group in groups),
        "ignored_non_gather_scatter": {
            "rtl": ignored_rtl,
            "gem5": ignored_gem5,
        },
        "all_grants_exact": all(group["grant_exact"] for group in groups),
        "groups": groups,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-log", type=Path, required=True)
    parser.add_argument("--rtl-task-boundaries", type=Path, required=True)
    parser.add_argument("--gem5-log", type=Path, required=True)
    parser.add_argument("--gem5-task-trace", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    rtl_intervals = read_rtl_task_intervals(args.rtl_task_boundaries)
    gem5_intervals = read_gem5_task_intervals(args.gem5_task_trace)
    rtl_records, ignored_rtl = read_phases(
        args.rtl_log, RTL_PHASE, rtl_intervals
    )
    gem5_records, ignored_gem5 = read_phases(
        args.gem5_log, GEM5_PHASE, gem5_intervals
    )
    report = compare(
        rtl_records, gem5_records, ignored_rtl, ignored_gem5
    )
    rendered = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
