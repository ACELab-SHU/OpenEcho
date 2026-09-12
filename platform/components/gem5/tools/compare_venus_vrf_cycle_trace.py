#!/usr/bin/env python3
"""Compare lane-0 gem5 VRF arbitration edges with an RTL VCD oracle.

The gem5 side is reconstructed from NoncoherentXBar's stable pending-source
messages.  A source remains asserted until the corresponding RR grant, just
like one bit of RTL ``req_lvl2``.  The comparison is deliberately based on
request vectors, LSU suppression, and winners rather than instruction
duration or full-DAG timing.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from parse_rtl_vfu_vcd import (
    MASTER_NAMES,
    VAR_RE,
    bit_value,
    parse_change,
    selected_codes,
    set_masters,
)


PENDING_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*Venus VRF request pending src (?P<src>\d+) "
    r"bank (?P<bank>\d+) (?:master (?P<master>\d+)|behind LSU priority)$"
)
GRANT_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*Venus VRF RR grant bank (?P<bank>\d+) "
    r"old_rr (?P<old_rr>\d+) winner_master (?P<winner>\d+) "
    r"source (?P<src>\d+) next_rr (?P<next_rr>\d+) "
    r"contenders (?P<contenders>\d+)$"
)
LSU_RE = re.compile(
    r"^\s*(?P<tick>\d+): .*Venus VRF bank (?P<bank>\d+) "
    r"blocked by LSU priority$"
)


def master_for_source(source: int, ports_per_lane: int = 16) -> int | None:
    port = source % ports_per_lane
    if port <= 7:
        return port
    if port == 12:
        return 8
    if port in (10, 11):
        return 9
    if port == 14:
        return 10
    if port in (8, 15):
        return 11
    return None


def names(masters):
    return [MASTER_NAMES.get(master, str(master)) for master in sorted(masters)]


def parse_gem5(path: Path, lane: int):
    bank_base = lane * 4
    source_base = lane * 16
    source_limit = source_base + 16
    pending = [set() for _ in range(4)]
    groups = {}

    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = PENDING_RE.match(line)
        if match:
            source = int(match["src"])
            absolute_bank = int(match["bank"])
            if not (source_base <= source < source_limit):
                continue
            if not (bank_base <= absolute_bank < bank_base + 4):
                continue
            bank = absolute_bank - bank_base
            pending[bank].add(source)
            groups.setdefault(int(match["tick"]), {})
            continue

        match = LSU_RE.match(line)
        if match:
            absolute_bank = int(match["bank"])
            if not (bank_base <= absolute_bank < bank_base + 4):
                continue
            bank = absolute_bank - bank_base
            tick = int(match["tick"])
            masters = {
                master_for_source(source) for source in pending[bank]
            }
            masters.discard(None)
            groups.setdefault(tick, {})[bank] = {
                "requests": names(masters),
                "lsu": True,
                "winner": None,
            }
            continue

        match = GRANT_RE.match(line)
        if not match:
            continue
        source = int(match["src"])
        absolute_bank = int(match["bank"])
        if not (source_base <= source < source_limit):
            continue
        if not (bank_base <= absolute_bank < bank_base + 4):
            continue
        bank = absolute_bank - bank_base
        tick = int(match["tick"])
        masters = {master_for_source(item) for item in pending[bank]}
        masters.discard(None)
        winner = int(match["winner"])
        groups.setdefault(tick, {})[bank] = {
            "requests": names(masters),
            "lsu": False,
            "winner": MASTER_NAMES.get(winner, str(winner)),
            "old_rr": int(match["old_rr"]),
            "next_rr": int(match["next_rr"]),
        }
        pending[bank].discard(source)

    return groups


def parse_rtl(path: Path):
    scopes = []
    all_names = {}
    in_header = True
    state = {}
    group_time = None
    group_changes = []
    cycle = -1
    groups = {}
    codes = {}

    def process_group():
        nonlocal cycle
        if group_time is None:
            return
        pre_clock = state.get(codes.get("clk"))
        posedge = False
        for code, value in group_changes:
            if code == codes.get("clk") and value == "1" and pre_clock != "1":
                posedge = True
            state[code] = value
        if not posedge:
            return
        cycle += 1
        req_ls = bit_value(state.get(codes.get("requester.req_ls")))
        edges = {}
        for bank in range(4):
            requests = set_masters(
                state, codes.get("requester.req_lvl2"), bank)
            if not requests:
                continue
            winners = set_masters(
                state, codes.get("requester.gnt_lvl2"), bank) or []
            edges[bank] = {
                "requests": requests,
                "lsu": None if req_ls is None else bool(req_ls & (1 << bank)),
                "winner": winners[0] if len(winners) == 1 else None,
            }
        if edges:
            groups[cycle] = {"time_fs": group_time, "banks": edges}

    with path.open(encoding="utf-8", errors="replace") as stream:
        for raw_line in stream:
            line = raw_line.strip()
            if in_header:
                if line.startswith("$scope"):
                    scopes.append(line.split()[2])
                elif line.startswith("$upscope"):
                    scopes.pop()
                else:
                    match = VAR_RE.match(line)
                    if match:
                        signal_path = ".".join(scopes + [match["name"]])
                        all_names.setdefault(match["code"], []).append(signal_path)
                if line.startswith("$enddefinitions"):
                    in_header = False
                    codes = selected_codes(all_names)
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
    return groups


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-vcd", required=True, type=Path)
    parser.add_argument("--gem5-log", required=True, type=Path)
    parser.add_argument("--lane", type=int, default=0)
    parser.add_argument("--rtl-anchor-cycle", type=int)
    parser.add_argument("--gem5-anchor-tick", type=int)
    parser.add_argument("--clock-ticks", type=int, default=2000)
    parser.add_argument("--limit", type=int, default=20)
    args = parser.parse_args()

    rtl = parse_rtl(args.rtl_vcd)
    gem5 = parse_gem5(args.gem5_log, args.lane)
    if not rtl or not gem5:
        raise SystemExit("no comparable RTL or gem5 VRF arbitration events")

    rtl_anchor = (
        args.rtl_anchor_cycle if args.rtl_anchor_cycle is not None
        else min(rtl)
    )
    gem5_anchor = (
        args.gem5_anchor_tick if args.gem5_anchor_tick is not None
        else min(gem5)
    )
    mismatches = []
    compared = 0
    relative_cycles = sorted({
        cycle - rtl_anchor for cycle in rtl if cycle >= rtl_anchor
    } | {
        (tick - gem5_anchor) // args.clock_ticks
        for tick in gem5 if tick >= gem5_anchor and
        (tick - gem5_anchor) % args.clock_ticks == 0
    })

    for relative in relative_cycles:
        rtl_entry = rtl.get(rtl_anchor + relative, {}).get("banks", {})
        gem5_tick = gem5_anchor + relative * args.clock_ticks
        gem5_entry = gem5.get(gem5_tick, {})
        for bank in sorted(set(rtl_entry) | set(gem5_entry)):
            expected = rtl_entry.get(bank)
            actual = gem5_entry.get(bank)
            compared += 1
            comparable_actual = None if actual is None else {
                key: actual[key] for key in ("requests", "lsu", "winner")
            }
            if expected != comparable_actual:
                mismatches.append({
                    "relative_cycle": relative,
                    "bank": bank,
                    "rtl_cycle": rtl_anchor + relative,
                    "rtl_time_ns": (
                        None if rtl_anchor + relative not in rtl else
                        rtl[rtl_anchor + relative]["time_fs"] / 1_000_000
                    ),
                    "gem5_tick": gem5_tick,
                    "rtl": expected,
                    "gem5": actual,
                })

    result = {
        "schema": "venus-vrf-cycle-trace-comparison/v1",
        "lane": args.lane,
        "anchors": {
            "rtl_cycle": rtl_anchor,
            "gem5_tick": gem5_anchor,
            "clock_ticks": args.clock_ticks,
        },
        "compared_bank_edges": compared,
        "mismatch_count": len(mismatches),
        "first_mismatch": mismatches[0] if mismatches else None,
        "mismatches": mismatches[:args.limit],
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 1 if mismatches else 0


if __name__ == "__main__":
    raise SystemExit(main())
