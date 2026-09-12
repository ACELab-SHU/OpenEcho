#!/usr/bin/env python3
"""Compare RTL and gem5 DAG task lifecycle records.

The checker deliberately keeps functional byte comparison separate from L1:
RTL ``execute complete`` is compared with gem5 ``task_epilogue``, and the
four lifecycle boundaries remain independently visible.  Times are printed
after normalising each run to its first allocation, so unrelated testbench
boot/reset lead-in cannot hide a scheduling or DMA-handshake mismatch.
"""

import argparse
import json
import re
from pathlib import Path


RTL_PATTERNS = {
    "allocated": re.compile(
        r"^task (\d+) is allocated to tile (\d+) at time (\d+)$"),
    "started": re.compile(
        r"^task (\d+) start execute at tile (\d+) at time (\d+)$"),
    "complete": re.compile(
        r"^task (\d+) execute complete at tile (\d+) at time (\d+)$"),
    "released": re.compile(
        r"^task (\d+) is released from tile (\d+) at time (\d+)$"),
}

GEM_EVENTS = {
    "tile_allocated": "allocated",
    "tile_started": "started",
    "task_epilogue": "complete",
    "tile_released": "released",
}
EVENT_ORDER = ("allocated", "started", "complete", "released")


def parse_rtl(path):
    records = {}
    order = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        for event, pattern in RTL_PATTERNS.items():
            match = pattern.match(line)
            if not match:
                continue
            task, tile, timestamp = map(int, match.groups())
            entry = records.setdefault(task, {"tile": tile})
            if entry["tile"] != tile:
                raise ValueError(f"RTL task {task} changed tile")
            if event in entry:
                raise ValueError(f"RTL task {task} has duplicate {event}")
            entry[event] = timestamp
            order.append((event, task))
            break
    return records, order


def parse_gem5(path, tick_ps):
    records = {}
    order = []
    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw.strip():
            continue
        event = json.loads(raw)
        kind = GEM_EVENTS.get(event.get("event"))
        if kind is None:
            continue
        task = event["task_id"]
        entry = records.setdefault(task, {})
        tile = event.get("tile_id")
        if tile is None:
            tile = entry.get("tile")
        if tile is None:
            raise ValueError(
                f"gem5 trace line {lineno} has no tile_id and no prior "
                f"task-local tile record")
        timestamp = event["tick"] * tick_ps / 1000.0
        if "tile" not in entry:
            entry["tile"] = tile
        if entry["tile"] != tile:
            raise ValueError(f"gem5 task {task} changed tile")
        if kind in entry:
            raise ValueError(f"gem5 task {task} has duplicate {kind}")
        entry[kind] = timestamp
        order.append((kind, task))
    return records, order


def fmt_ns(value):
    if value is None:
        return "—"
    if float(value).is_integer():
        return str(int(value))
    return f"{value:.3f}"


def require_complete(label, records):
    for task, record in sorted(records.items()):
        missing = [event for event in EVENT_ORDER if event not in record]
        if missing:
            raise ValueError(f"{label} task {task} lacks {', '.join(missing)}")


def first_allocation(records):
    return min(record["allocated"] for record in records.values())


def compare(rtl, rtl_order, gem, gem_order, tolerance_ns):
    failures = []
    if set(rtl) != set(gem):
        failures.append(
            f"task sets differ: RTL={sorted(rtl)} gem5={sorted(gem)}")
        return failures
    for task in sorted(rtl):
        if rtl[task]["tile"] != gem[task]["tile"]:
            failures.append(
                f"task {task} tile RTL={rtl[task]['tile']} "
                f"gem5={gem[task]['tile']}")

    rtl_alloc_order = [task for event, task in rtl_order if event == "allocated"]
    gem_alloc_order = [task for event, task in gem_order if event == "allocated"]
    if rtl_alloc_order != gem_alloc_order:
        failures.append(
            f"allocation order RTL={rtl_alloc_order} gem5={gem_alloc_order}")

    rtl_zero = first_allocation(rtl)
    gem_zero = first_allocation(gem)
    for task in sorted(rtl):
        for event in EVENT_ORDER:
            rtl_relative = rtl[task][event] - rtl_zero
            gem_relative = gem[task][event] - gem_zero
            if abs(rtl_relative - gem_relative) > tolerance_ns:
                failures.append(
                    f"first event-time mismatch: task {task} {event} "
                    f"RTL+{fmt_ns(rtl_relative)}ns "
                    f"gem5+{fmt_ns(gem_relative)}ns")
                return failures
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-scheduler-log", type=Path, required=True)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    parser.add_argument(
        "--gem5-tick-ps", type=float, default=1.0,
        help="duration of one gem5 tick in ps (default: 1)")
    parser.add_argument(
        "--tolerance-ns", type=float, default=0.0,
        help="uniform normalised-event tolerance in ns (default: 0)")
    parser.add_argument(
        "--report-only", action="store_true",
        help="print mismatches but return success for evidence collection")
    args = parser.parse_args()

    rtl, rtl_order = parse_rtl(args.rtl_scheduler_log)
    gem, gem_order = parse_gem5(args.gem5_trace, args.gem5_tick_ps)
    require_complete("RTL", rtl)
    require_complete("gem5", gem)

    print("RTL execute complete is compared with gem5 task_epilogue.")
    print("| Task | Tile | RTL alloc/start/complete/release (ns) | "
          "gem5 alloc/start/epilogue/release (ns) |")
    print("| ---: | ---: | --- | --- |")
    for task in sorted(set(rtl) | set(gem)):
        rtl_record = rtl.get(task, {})
        gem_record = gem.get(task, {})
        rtl_times = "/".join(fmt_ns(rtl_record.get(event))
                             for event in EVENT_ORDER)
        gem_times = "/".join(fmt_ns(gem_record.get(event))
                             for event in EVENT_ORDER)
        tile = rtl_record.get("tile", gem_record.get("tile", "—"))
        print(f"| {task} | {tile} | {rtl_times} | {gem_times} |")

    failures = compare(rtl, rtl_order, gem, gem_order, args.tolerance_ns)
    if failures:
        print("\nL1 lifecycle: MISMATCH")
        for failure in failures:
            print(f"- {failure}")
        return 0 if args.report_only else 1
    print("\nL1 lifecycle: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
