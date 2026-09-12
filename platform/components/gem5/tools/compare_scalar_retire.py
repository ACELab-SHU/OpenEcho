#!/usr/bin/env python3
"""Compare RTL scalar-retire timing with a gem5 MinorExecute trace.

The RTL input is the generic venus_full_dag_perf JSONL emitted by the
tile-level monitor.  The gem5 input is produced with:

  --debug-flags=MinorExecute --debug-file=<path>

Only "Completed inst" records are consumed, so the script remains useful even
though MinorExecute also contains detailed issue and queue diagnostics.
"""

import argparse
import collections
import json
import re


GEM_COMPLETED = re.compile(
    r"^\s*(\d+): .*Completed inst: (\d+)/.* "
    r"pc: 0x([0-9a-fA-F]+) \(([^)]+)\)"
)
GEM_DISCARDED = re.compile(
    r"^\s*(\d+): .*Discarding inst: (\d+)/.* "
    r"pc: 0x([0-9a-fA-F]+)"
)


def rtl_class(inst):
    opcode = inst & 0x7F
    funct3 = (inst >> 12) & 7
    funct7 = (inst >> 25) & 0x7F
    if opcode == 0x03:
        return "load"
    if opcode == 0x23:
        return "store"
    if opcode == 0x2F:
        funct5 = (inst >> 27) & 0x1F
        if funct5 == 0x02:
            return "load"   # LR.W
        if funct5 == 0x03:
            return "store"  # SC.W
        return "atomic"
    if opcode == 0x63:
        return "branch"
    if opcode == 0x6F:
        return "jal"
    if opcode == 0x67:
        return "jalr"
    if opcode == 0x33 and funct7 == 1:
        if funct3 <= 3:
            return "mul"
        return "divrem"
    if opcode in (0x2B, 0x5B):
        return "venus"
    if opcode == 0x0F:
        return "fence"
    if opcode == 0x73:
        return "system"
    return "alu"


def gem_class(mnemonic):
    op = mnemonic.split()[0].lower()
    if op.startswith("l") and op not in ("lui",):
        return "load"
    if op.startswith("s") and op not in ("sll", "slli", "slt", "slti",
                                         "sltiu", "sltu", "sra", "srai",
                                         "srl", "srli", "sub"):
        return "store"
    if op.startswith("b"):
        return "branch"
    if op == "jal":
        return "jal"
    if op == "jalr":
        return "jalr"
    if op.startswith("mul"):
        return "mul"
    if op.startswith("div") or op.startswith("rem"):
        return "divrem"
    if op == "unknown":
        # The generic decoder does not retain a useful class for raw
        # encodings that scalar600 handles through its decoder defaults.
        return "unknown"
    if op.startswith("v"):
        return "venus"
    if op.startswith("fence"):
        return "fence"
    if op in ("ecall", "ebreak", "wfi") or op.startswith("csr"):
        return "system"
    return "alu"


def venus_needs_extension(inst):
    opcode = inst & 0x7F
    if opcode not in (0x2B, 0x5B):
        return False
    func3 = (inst >> 12) & 0x7
    func5 = (inst >> 27) & 0x1F
    bits_26_15 = (inst >> 15) & 0xFFF
    return not (
        func3 == 7 and (
            func5 in (0b00100, 0b00101)
            or (func5 == 0b00001 and bits_26_15 == 0)
        )
    )


def load_rtl(path, task):
    raw = []
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue
            if (event.get("event") != "scalar_retire" or
                    event.get("task_id") != task):
                continue
            inst = int(event["inst"], 16)
            raw.append((
                int(event["task_cycle"]),
                int(event["pc"], 16),
                rtl_class(inst),
                f"0x{inst:08x}",
            ))
    # scalar600_wb_stage defines wb_debug_valid as a change detector over the
    # entire EX->WB bus.  A zero-divisor DIV/REM can keep the same instruction
    # in WB while its result bus changes on consecutive cycles; the monitor
    # consequently emits several scalar_retire records for one instruction.
    # This is not a second ID/EX admission.  Collapse only consecutive held
    # RV32M divide/remainder records, retaining the first completion cycle.
    # A real repeated execution still has an intervening control instruction.
    held_wb_filtered = []
    previous_raw = None
    for entry in raw:
        inst = int(entry[3], 16)
        is_divrem = (
            (inst & 0x7f) == 0x33
            and ((inst >> 25) & 0x7f) == 1
            and ((inst >> 12) & 7) >= 4
        )
        if previous_raw is not None and is_divrem:
            if (entry[0] == previous_raw[0] + 1 and
                    entry[1:] == previous_raw[1:]):
                previous_raw = entry
                continue
        held_wb_filtered.append(entry)
        previous_raw = entry
    raw = held_wb_filtered
    records = []
    index = 0
    while index < len(raw):
        entry = raw[index]
        records.append(entry)
        inst = int(entry[3], 16)
        has_observed_extension = (
            venus_needs_extension(inst)
            and index + 1 < len(raw)
            and raw[index + 1][1] == entry[1] + 4
        )
        index += 2 if has_observed_extension else 1
    return records


def load_gem(path, tick_scale, thread):
    records = []
    discarded = collections.Counter()
    with open(path, encoding="utf-8", errors="replace") as stream:
        for line in stream:
            discard_match = GEM_DISCARDED.match(line)
            if discard_match:
                tick, record_thread, pc = discard_match.groups()
                discarded[(int(tick), int(record_thread), int(pc, 16))] += 1
                continue
            match = GEM_COMPLETED.match(line)
            if not match:
                continue
            tick, record_thread, pc, mnemonic = match.groups()
            # The generic RISC-V decoder names an all-zero 32-bit word as a
            # compressed c.addi4spn before returning IllegalInstFault.
            # scalar600 instead consumes it as an internal bubble and its
            # RTL retire monitor deliberately filters wb_instr==0.
            if mnemonic.split()[0].lower() == "c_addi4spn":
                continue
            discard_key = (int(tick), int(record_thread), int(pc, 16))
            if discarded[discard_key]:
                discarded[discard_key] -= 1
                continue
            if int(record_thread) != thread:
                continue
            records.append((
                int(tick) // tick_scale,
                int(pc, 16),
                gem_class(mnemonic),
                mnemonic,
            ))
    if records:
        base = records[0][0]
        records = [(cycle - base, pc, cls, inst)
                   for cycle, pc, cls, inst in records]
    return records


def summarize(label, records, top):
    print(f"{label}: records={len(records)}")
    if len(records) < 2:
        return
    totals = collections.defaultdict(lambda: [0, 0, 0])
    pc_totals = collections.defaultdict(lambda: [0, 0, 0])
    previous = records[0]
    for current in records[1:]:
        gap = current[0] - previous[0]
        key = (previous[2], current[2])
        for table_key, table in ((key, totals), ((current[1], current[2]),
                                                pc_totals)):
            entry = table[table_key]
            entry[0] += 1
            entry[1] += gap
            entry[2] = max(entry[2], gap)
        previous = current

    print("  top predecessor->current pairs by total cycles:")
    for key, (count, cycles, maximum) in sorted(
            totals.items(), key=lambda item: item[1][1], reverse=True)[:top]:
        print(f"    {key[0]:>7}->{key[1]:<7} count={count:7d} "
              f"cycles={cycles:9d} avg={cycles/count:7.2f} max={maximum}")
    print("  top PCs by total incoming-gap cycles:")
    for (pc, cls), (count, cycles, maximum) in sorted(
            pc_totals.items(), key=lambda item: item[1][1],
            reverse=True)[:top]:
        print(f"    pc=0x{pc:08x} {cls:<7} count={count:7d} "
              f"cycles={cycles:9d} avg={cycles/count:7.2f} max={maximum}")


def first_divergence(rtl, gem, cycle_tolerance, max_events,
                     allow_gem5_tail):
    """Compare the architectural retirement stream before aggregating it.

    The aggregate gap tables below remain useful to find expensive classes,
    but they can hide a compensating early/late event.  This check deliberately
    keys an event on the PC and decoded instruction class and then compares the
    cycle relative to each stream's first retirement.  It is therefore a
    diagnostic for the first observable scalar-side difference, not an IPC
    approximation.
    """
    if not rtl:
        print("FIRST-DIVERGENCE: RTL has no scalar_retire events")
        return 1
    if not gem:
        print("FIRST-DIVERGENCE: gem5 has no Completed inst events")
        return 1

    start = next(
        (index for index, event in enumerate(rtl)
         if event[1] == gem[0][1] and
         (event[2] == gem[0][2] or gem[0][2] == "unknown")),
        None,
    )
    if start is None:
        print(
            f"FIRST-DIVERGENCE: gem5 first event pc=0x{gem[0][1]:08x} "
            f"class={gem[0][2]} is absent from RTL"
        )
        return 1
    rtl = rtl[start:]
    rtl_base = rtl[0][0]
    gem_base = gem[0][0]
    limit = min(len(rtl), len(gem), max_events or max(len(rtl), len(gem)))
    for index in range(limit):
        rtl_event = rtl[index]
        gem_event = gem[index]
        rtl_cycle = rtl_event[0] - rtl_base
        gem_cycle = gem_event[0] - gem_base
        identity_match = (
            rtl_event[1] == gem_event[1] and
            (rtl_event[2] == gem_event[2] or
             gem_event[2] == "unknown")
        )
        timing_match = abs(rtl_cycle - gem_cycle) <= cycle_tolerance
        if not identity_match or not timing_match:
            print(
                "FIRST-DIVERGENCE: index={index} "
                "rtl=(cycle={rtl_cycle}, raw_cycle={rtl_raw}, pc=0x{rtl_pc:08x}, "
                "class={rtl_cls}, inst={rtl_inst}) "
                "gem5=(cycle={gem_cycle}, raw_cycle={gem_raw}, pc=0x{gem_pc:08x}, "
                "class={gem_cls}, inst={gem_inst}) ".format(
                    index=index,
                    rtl_cycle=rtl_cycle,
                    rtl_raw=rtl_event[0],
                    rtl_pc=rtl_event[1],
                    rtl_cls=rtl_event[2],
                    rtl_inst=rtl_event[3],
                    gem_cycle=gem_cycle,
                    gem_raw=gem_event[0],
                    gem_pc=gem_event[1],
                    gem_cls=gem_event[2],
                    gem_inst=gem_event[3],
                )
            )
            return 1

    unmatched_gem5 = max(0, len(gem) - len(rtl))
    length_mismatch = (
        len(rtl) > len(gem) or unmatched_gem5 > allow_gem5_tail
    )
    if length_mismatch and (not max_events or limit < max(len(rtl), len(gem))):
        print(
            "FIRST-DIVERGENCE: stream length rtl={rtl_count} gem5={gem_count} "
            "after {matched} matched events".format(
                rtl_count=len(rtl), gem_count=len(gem), matched=limit
            )
        )
        return 1

    checked = limit
    suffix = " (prefix limited)" if max_events and checked < min(len(rtl), len(gem)) else ""
    print(f"FIRST-DIVERGENCE: MATCH {checked} scalar-retire event(s){suffix}")
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-jsonl", required=True)
    parser.add_argument("--rtl-task", type=int, required=True)
    parser.add_argument("--gem5-log", required=True)
    parser.add_argument("--gem5-ticks-per-cycle", type=int, default=1000)
    parser.add_argument(
        "--gem5-thread", type=int,
        help="MinorCPU thread id (defaults to --rtl-task)",
    )
    parser.add_argument("--top", type=int, default=20)
    parser.add_argument(
        "--first-divergence", action="store_true",
        help=("compare retire events in order by PC/class and relative cycle; "
              "return non-zero at the first mismatch"),
    )
    parser.add_argument(
        "--cycle-tolerance", type=int, default=0,
        help="allowed absolute relative-cycle difference with --first-divergence",
    )
    parser.add_argument(
        "--max-events", type=int, default=0,
        help="optional prefix limit for --first-divergence (0 means whole stream)",
    )
    parser.add_argument(
        "--allow-gem5-tail", type=int, default=0,
        help=("allow this many gem5-only termination instructions after the "
              "complete RTL retire stream"),
    )
    args = parser.parse_args()

    rtl = load_rtl(args.rtl_jsonl, args.rtl_task)
    gem_thread = args.rtl_task if args.gem5_thread is None else args.gem5_thread
    gem = load_gem(args.gem5_log, args.gem5_ticks_per_cycle, gem_thread)
    if args.first_divergence:
        return first_divergence(
            rtl, gem, args.cycle_tolerance, args.max_events,
            args.allow_gem5_tail,
        )
    summarize("RTL", rtl, args.top)
    summarize("gem5", gem, args.top)


if __name__ == "__main__":
    raise SystemExit(main())
