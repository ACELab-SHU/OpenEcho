#!/usr/bin/env python3
"""Aggregate aligned RTL/gem5 scalar retirement gaps by instruction pair."""

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

from compare_scalar_traces import read_rtl


EXEC_RE = re.compile(
    r"^(?P<tick>\d+): .*: T(?P<thread>\d+) : "
    r"0x(?P<pc>[0-9a-fA-F]+).*?: (?P<asm>.*?)\s+: (?P<opclass>\S+)\s+:"
)


def classify_bits(bits):
    opcode = bits & 0x7F
    if opcode == 0x03:
        return "load"
    if opcode == 0x23:
        return "store"
    if opcode == 0x63:
        return "branch"
    if opcode in (0x67, 0x6F):
        return "jal"
    if opcode in (0x2B, 0x5B):
        return "venus"
    if opcode in (0x33, 0x3B):
        funct7 = (bits >> 25) & 0x7F
        funct3 = (bits >> 12) & 0x7
        if funct7 == 0x01 and funct3 >= 4:
            return "divrem"
    return "alu"


def classify_asm(asm, opclass):
    mnemonic = asm.strip().split(None, 1)[0]
    if mnemonic in ("lb", "lh", "lw", "lbu", "lhu"):
        return "load"
    if mnemonic in ("sb", "sh", "sw"):
        return "store"
    if mnemonic in ("beq", "bne", "blt", "bge", "bltu", "bgeu"):
        return "branch"
    if mnemonic in ("jal", "jalr"):
        return "jal"
    if mnemonic in ("div", "divu", "rem", "remu"):
        return "divrem"
    if opclass.startswith("Venus") or opclass == "Venus":
        return "venus"
    return "alu"


def read_gem5(path, thread):
    result = []
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            match = EXEC_RE.match(line)
            if not match or int(match.group("thread")) != thread:
                continue
            result.append({
                "pc": int(match.group("pc"), 16),
                "cycle": int(match.group("tick")) / 2000,
                "kind": classify_asm(
                    match.group("asm"), match.group("opclass")
                ),
            })
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-jsonl", type=Path, required=True)
    parser.add_argument("--gem5-exec", type=Path, required=True)
    parser.add_argument("--task", type=int, required=True)
    parser.add_argument("--gem-thread", type=int)
    args = parser.parse_args()

    _, rtl = read_rtl(args.rtl_jsonl, args.task)
    thread = args.task if args.gem_thread is None else args.gem_thread
    gem5 = read_gem5(args.gem5_exec, thread)
    if not rtl or not gem5:
        raise SystemExit(f"empty trace rtl={len(rtl)} gem5={len(gem5)}")

    rtl_start = next(
        (index for index, entry in enumerate(rtl)
         if entry["pc"] == gem5[0]["pc"]),
        None,
    )
    if rtl_start is None:
        raise SystemExit("gem5 entry PC is absent from RTL trace")

    count = min(len(rtl) - rtl_start, len(gem5))
    rtl = rtl[rtl_start:rtl_start + count]
    gem5 = gem5[:count]
    for index, (expected, actual) in enumerate(zip(rtl, gem5)):
        if expected["pc"] != actual["pc"]:
            raise SystemExit(
                f"PC mismatch at {index}: rtl=0x{expected['pc']:x} "
                f"gem5=0x{actual['pc']:x}"
            )

    totals = defaultdict(lambda: [0, 0.0, 0.0])
    pc_totals = defaultdict(lambda: [0, 0.0, 0.0])
    for index in range(1, count):
        previous_kind = classify_bits(rtl[index - 1]["inst"])
        current_kind = classify_bits(rtl[index]["inst"])
        pair = f"{previous_kind}->{current_kind}"
        rtl_gap = (
            rtl[index]["sim_time"] - rtl[index - 1]["sim_time"]
        ) / 2
        gem5_gap = gem5[index]["cycle"] - gem5[index - 1]["cycle"]
        aggregate = totals[pair]
        aggregate[0] += 1
        aggregate[1] += rtl_gap
        aggregate[2] += gem5_gap
        pc_aggregate = pc_totals[
            (rtl[index]["pc"], previous_kind, current_kind)
        ]
        pc_aggregate[0] += 1
        pc_aggregate[1] += rtl_gap
        pc_aggregate[2] += gem5_gap

    print("pair count rtl_cycles gem5_cycles delta gem5_avg rtl_avg")
    for pair, (pair_count, rtl_total, gem5_total) in sorted(
        totals.items(), key=lambda item: abs(item[1][2] - item[1][1]),
        reverse=True,
    ):
        print(
            f"{pair:16s} {pair_count:6d} {rtl_total:10.0f} "
            f"{gem5_total:11.0f} {gem5_total - rtl_total:+10.0f} "
            f"{gem5_total / pair_count:8.2f} "
            f"{rtl_total / pair_count:7.2f}"
        )

    print("\npc pair count rtl_cycles gem5_cycles delta gem5_avg rtl_avg")
    for (pc, previous_kind, current_kind), (
        pair_count, rtl_total, gem5_total
    ) in sorted(
        pc_totals.items(),
        key=lambda item: abs(item[1][2] - item[1][1]),
        reverse=True,
    )[:40]:
        print(
            f"0x{pc:08x} {previous_kind:7s}->{current_kind:7s} "
            f"{pair_count:6d} {rtl_total:10.0f} {gem5_total:11.0f} "
            f"{gem5_total - rtl_total:+10.0f} "
            f"{gem5_total / pair_count:8.2f} "
            f"{rtl_total / pair_count:7.2f}"
        )


if __name__ == "__main__":
    main()
