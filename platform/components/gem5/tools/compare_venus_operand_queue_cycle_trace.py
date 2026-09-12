#!/usr/bin/env python3
"""Compare one gem5 operand queue with a curated RTL queue VCD.

The comparison uses registered queue usage as the primary invariant and also
reports same-edge issue/pop/ready signals.  It is intended to locate the
occupancy divergence that precedes a visible VRF request-vector mismatch.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from parse_rtl_vfu_vcd import VAR_RE, parse_change


QUEUE_RE = re.compile(
    r"^(?P<tick>\d+): .*VenusLane_(?P<lane>\d+): "
    r"operand queue usage type (?P<operand>\d+) "
    r"(?P<action>issued|popped|restored): q (?P<usage>\d+) "
    r"delta (?P<delta>[+-]\d+) depth (?P<depth>\d+)$"
)


def scalar(value):
    if value is None or any(bit not in "01" for bit in value):
        return None
    return int(value, 2)


def vector_bit(value, bit):
    integer = scalar(value)
    return None if integer is None else bool(integer & (1 << bit))


def parse_rtl(path: Path, operand: int):
    scopes = []
    all_names = {}
    state = {}
    codes = {}
    group_time = None
    group_changes = []
    cycle = -1
    samples = {}

    wanted = {
        "clk_i",
        "operand_issued_o",
        "operand_queue_ready_i",
        "operand_valid_i",
        "operand_valid_o",
        "operand_ready_i",
        "ibuf_pop",
        "ibuf_usage_q",
    }

    def value(name):
        raw = state.get(codes.get(name))
        if raw is not None and raw.startswith("b"):
            return raw[1:]
        return raw

    def process_group():
        nonlocal cycle
        if group_time is None:
            return
        pre_clock = state.get(codes.get("clk_i"))
        posedge = False
        for code, changed in group_changes:
            if (code == codes.get("clk_i") and changed == "1" and
                    pre_clock != "1"):
                posedge = True
            state[code] = changed
        if not posedge:
            return
        cycle += 1
        samples[cycle] = {
            "time_fs": group_time,
            "usage": scalar(value("ibuf_usage_q")),
            "issued": vector_bit(value("operand_issued_o"), operand),
            "queue_ready": vector_bit(
                value("operand_queue_ready_i"), operand),
            "pop": vector_bit(value("ibuf_pop"), 0),
            "operand_valid_i": vector_bit(value("operand_valid_i"), 0),
            "operand_valid_o": vector_bit(value("operand_valid_o"), 0),
            "operand_ready_i": vector_bit(value("operand_ready_i"), 0),
        }

    header = True
    dollar = "$"
    with path.open(encoding="utf-8", errors="replace") as stream:
        for raw_line in stream:
            line = raw_line.strip()
            if header:
                if line.startswith(dollar + "scope"):
                    scopes.append(line.split()[2])
                elif line.startswith(dollar + "upscope"):
                    scopes.pop()
                else:
                    match = VAR_RE.match(line)
                    if match:
                        leaf = match["name"].split()[0]
                        if leaf in wanted:
                            all_names[match["code"]] = leaf
                if line.startswith(dollar + "enddefinitions"):
                    header = False
                    for code, leaf in all_names.items():
                        if leaf in codes:
                            raise ValueError(
                                f"ambiguous RTL signal leaf {leaf}")
                        codes[leaf] = code
                continue
            if line.startswith("#"):
                process_group()
                group_time = int(line[1:])
                group_changes = []
                continue
            change = parse_change(line)
            if change is not None:
                group_changes.append(change)
    process_group()
    return samples


def parse_gem5(path: Path, operand: int, lane: int):
    events = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = QUEUE_RE.match(line)
        if (not match or int(match["operand"]) != operand or
                int(match["lane"]) != lane):
            continue
        tick = int(match["tick"])
        entry = events.setdefault(tick, {
            "usage": int(match["usage"]),
            "depth": int(match["depth"]),
            "delta": 0,
            "actions": [],
        })
        if entry["usage"] != int(match["usage"]):
            raise ValueError(f"usage snapshot changed within tick {tick}")
        entry["delta"] = int(match["delta"])
        entry["actions"].append(match["action"])
    return events


def gem5_at(events, tick):
    prior_ticks = [event_tick for event_tick in events if event_tick <= tick]
    if not prior_ticks:
        return None
    prior_tick = max(prior_ticks)
    prior = events[prior_tick]
    usage = prior["usage"]
    if tick > prior_tick:
        usage += prior["delta"]
    exact = events.get(tick)
    return {
        "usage": usage,
        "issued": exact is not None and "issued" in exact["actions"],
        "pop": exact is not None and "popped" in exact["actions"],
        "restored": exact is not None and "restored" in exact["actions"],
        "actions": [] if exact is None else exact["actions"],
        "delta": 0 if exact is None else exact["delta"],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-vcd", required=True, type=Path)
    parser.add_argument("--gem5-log", required=True, type=Path)
    parser.add_argument("--operand", type=int, default=1)
    parser.add_argument("--lane", type=int, default=0)
    parser.add_argument("--rtl-anchor-cycle", required=True, type=int)
    parser.add_argument("--gem5-anchor-tick", required=True, type=int)
    parser.add_argument("--clock-ticks", type=int, default=2000)
    parser.add_argument("--start-relative", type=int, default=0)
    parser.add_argument("--end-relative", type=int, default=80)
    parser.add_argument("--limit", type=int, default=20)
    args = parser.parse_args()

    rtl = parse_rtl(args.rtl_vcd, args.operand)
    gem5 = parse_gem5(args.gem5_log, args.operand, args.lane)
    rows = []
    event_rows = []
    for relative in range(args.start_relative, args.end_relative + 1):
        rtl_cycle = args.rtl_anchor_cycle + relative
        expected = rtl.get(rtl_cycle)
        gem5_tick = args.gem5_anchor_tick + relative * args.clock_ticks
        actual = gem5_at(gem5, gem5_tick)
        if expected is None or actual is None:
            continue
        if expected["usage"] != actual["usage"]:
            rows.append({
                "relative_cycle": relative,
                "rtl_cycle": rtl_cycle,
                "rtl_time_ns": expected["time_fs"] / 1_000_000,
                "gem5_tick": gem5_tick,
                "rtl": expected,
                "gem5": actual,
            })
        if (expected["issued"] != actual["issued"] or
                expected["pop"] != actual["pop"]):
            event_rows.append({
                "relative_cycle": relative,
                "rtl_cycle": rtl_cycle,
                "rtl_time_ns": expected["time_fs"] / 1_000_000,
                "gem5_tick": gem5_tick,
                "rtl": expected,
                "gem5": actual,
            })

    result = {
        "schema": "venus-operand-queue-cycle-comparison/v1",
        "operand": args.operand,
        "lane": args.lane,
        "anchors": {
            "rtl_cycle": args.rtl_anchor_cycle,
            "gem5_tick": args.gem5_anchor_tick,
            "clock_ticks": args.clock_ticks,
        },
        "usage_mismatch_count": len(rows),
        "first_usage_mismatch": rows[0] if rows else None,
        "mismatches": rows[:args.limit],
        "event_mismatch_count": len(event_rows),
        "first_event_mismatch": event_rows[0] if event_rows else None,
        "event_mismatches": event_rows[:args.limit],
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
