#!/usr/bin/env python3
"""Compare scalar600 RTL and gem5 architectural writeback in program order."""

import argparse
import json
import re


GEM_EXEC = re.compile(
    r"^\s*(\d+): .*system\.cpu: T(\d+) : 0x([0-9a-fA-F]+) .*"
    r" : ([^:]+?)\s*: [^:]+ :\s*(.*)$"
)
GEM_DEST = re.compile(r"(?:^|\s)D=0x([0-9a-fA-F]+)")


def is_divrem(inst):
    return (
        (inst & 0x7f) == 0x33
        and ((inst >> 25) & 0x7f) == 1
        and ((inst >> 12) & 7) >= 4
    )


def parse_rtl_hex(value):
    digits = value.lower()
    if digits.startswith("0x"):
        digits = digits[2:]
    parsed = 0
    known_mask = 0
    for digit in digits:
        parsed <<= 4
        known_mask <<= 4
        if digit not in "xz":
            parsed |= int(digit, 16)
            known_mask |= 0xf
    return parsed & 0xffffffff, known_mask & 0xffffffff


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
            rf_wdata, rf_known_mask = parse_rtl_hex(event["rf_wdata"])
            raw.append({
                "cycle": int(event["task_cycle"]),
                "pc": int(event["pc"], 16),
                "inst": int(event["inst"], 16),
                "rf_we": bool(event["rf_we"]),
                "rf_waddr": int(event["rf_waddr"]),
                "rf_wdata": rf_wdata,
                "rf_known_mask": rf_known_mask,
            })

    # A zero-divisor instruction can be held in scalar600 WB while the
    # combinational result changes.  wb_debug_valid then pulses for one ID/EX
    # admission more than once.  Keep the first completion cycle but the last
    # value actually left in the register file before the next instruction.
    records = []
    for event in raw:
        if (records and is_divrem(event["inst"]) and
                event["cycle"] == records[-1]["last_cycle"] + 1 and
                event["pc"] == records[-1]["pc"] and
                event["inst"] == records[-1]["inst"]):
            records[-1]["last_cycle"] = event["cycle"]
            records[-1]["rf_we"] = event["rf_we"]
            records[-1]["rf_waddr"] = event["rf_waddr"]
            records[-1]["rf_wdata"] = event["rf_wdata"]
            records[-1]["rf_known_mask"] = event["rf_known_mask"]
            continue
        record = dict(event)
        record["last_cycle"] = event["cycle"]
        records.append(record)
    return records


def load_gem(path, thread):
    records = []
    with open(path, encoding="utf-8", errors="replace") as stream:
        for line in stream:
            match = GEM_EXEC.match(line)
            if not match:
                continue
            tick, record_thread, pc, mnemonic, detail = match.groups()
            if int(record_thread) != thread:
                continue
            dest = GEM_DEST.search(detail)
            records.append({
                "tick": int(tick),
                "pc": int(pc, 16),
                "mnemonic": mnemonic.strip(),
                "dest": int(dest.group(1), 16) & 0xffffffff if dest else None,
            })
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-jsonl", required=True)
    parser.add_argument("--rtl-task", type=int, required=True)
    parser.add_argument("--gem5-log", required=True)
    parser.add_argument("--gem5-thread", type=int, default=0)
    parser.add_argument("--allow-gem5-tail", type=int, default=0)
    args = parser.parse_args()

    rtl = load_rtl(args.rtl_jsonl, args.rtl_task)
    gem = load_gem(args.gem5_log, args.gem5_thread)
    if not rtl or not gem:
        parser.error(f"empty stream: rtl={len(rtl)} gem5={len(gem)}")
    start = next((index for index, event in enumerate(rtl)
                  if event["pc"] == gem[0]["pc"]), None)
    if start is None:
        print("WRITEBACK: gem5 first PC is absent from RTL")
        raise SystemExit(1)
    rtl = rtl[start:]

    limit = min(len(rtl), len(gem))
    for index in range(limit):
        reference = rtl[index]
        observed = gem[index]
        if reference["pc"] != observed["pc"]:
            print(
                f"WRITEBACK: index={index} identity mismatch "
                f"rtl_pc=0x{reference['pc']:08x} "
                f"gem5_pc=0x{observed['pc']:08x}"
            )
            raise SystemExit(1)
        if not reference["rf_we"]:
            continue
        if observed["dest"] is None:
            print(
                f"WRITEBACK: index={index} pc=0x{reference['pc']:08x} "
                "RTL writes a register but gem5 has no destination value"
            )
            raise SystemExit(1)
        if ((reference["rf_wdata"] ^ observed["dest"]) &
                reference["rf_known_mask"]):
            print(
                f"WRITEBACK: index={index} pc=0x{reference['pc']:08x} "
                f"rd=x{reference['rf_waddr']} "
                f"rtl=0x{reference['rf_wdata']:08x} "
                f"known_mask=0x{reference['rf_known_mask']:08x} "
                f"gem5=0x{observed['dest']:08x} "
                f"inst=0x{reference['inst']:08x} "
                f"mnemonic={observed['mnemonic']}"
            )
            raise SystemExit(1)

    if len(rtl) > len(gem) or len(gem) - len(rtl) > args.allow_gem5_tail:
        print(
            f"WRITEBACK: length mismatch rtl={len(rtl)} gem5={len(gem)} "
            f"matched={limit}"
        )
        raise SystemExit(1)
    print(f"WRITEBACK: MATCH {limit} scalar instruction(s)")


if __name__ == "__main__":
    main()
