#!/usr/bin/env python3
"""Compare an RTL scalar-retirement JSONL stream with gem5 Exec tracing.

The RTL scalar core retires the two words of a 64-bit Venus instruction
separately.  gem5's RiscvVenusDecoder fuses those words into one instruction.
This tool applies the same classification before comparing PCs and scalar
register writeback values.  It is intentionally independent of DAG names and
task contents.
"""

import argparse
import json
import re
from pathlib import Path


EXEC_RE = re.compile(
    r"^\s*\d+: .*: T(?P<thread>\d+) : 0x(?P<pc>[0-9a-fA-F]+)"
)
DATA_RE = re.compile(r"\bD=0x(?P<data>[0-9a-fA-F]+)")


def venus_needs_extension(inst):
    opcode = inst & 0x7F
    if opcode not in (0x2B, 0x5B):
        return False
    func3 = (inst >> 12) & 0x7
    func5 = (inst >> 27) & 0x1F
    bits_26_15 = (inst >> 15) & 0xFFF
    # OPMISC CPU-extension forms occupy only the first 32-bit word; unlike
    # ordinary Venus instructions the following word remains independent.
    msb_only = func3 == 7 and (
        func5 in (0b00100, 0b00101)
        or (func5 == 0b00001 and bits_26_15 == 0)
    )
    return not msb_only


def read_rtl(path, task):
    raw = []
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            event = json.loads(line)
            if event.get("event") != "scalar_retire":
                continue
            if event.get("task_id") != task:
                continue
            rtl_wdata = event["rf_wdata"]
            wdata_digits = rtl_wdata[2:] if rtl_wdata.lower().startswith("0x") \
                else rtl_wdata
            wdata_known = not re.search(r"[xXzZ]", wdata_digits)
            raw.append({
                "pc": int(event["pc"], 16),
                "inst": int(event["inst"], 16),
                "rf_we": bool(event["rf_we"]),
                "rf_wdata": (int(rtl_wdata, 16) & 0xFFFFFFFF
                               if wdata_known else None),
                "rf_wdata_raw": rtl_wdata,
                "rf_wdata_known": wdata_known,
                "counter": event["scalar_instr_counter"],
                "sim_time": event.get("sim_time"),
            })

    fused = []
    index = 0
    while index < len(raw):
        entry = raw[index]
        fused.append(entry)
        # Some RTL configurations retire both words of a 64-bit Venus
        # instruction, while an all-zero extension word can be absent from
        # the retirement stream.  Fuse only an actually observed adjacent
        # PC; never consume the next valid scalar instruction blindly.
        has_observed_extension = (
            venus_needs_extension(entry["inst"])
            and index + 1 < len(raw)
            and raw[index + 1]["pc"] == entry["pc"] + 4
        )
        index += 2 if has_observed_extension else 1
    return raw, fused


def read_gem5(path, thread):
    result = []
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            match = EXEC_RE.match(line)
            if not match or int(match.group("thread")) != thread:
                continue
            data_match = DATA_RE.search(line)
            data = data_match.group("data") if data_match else None
            entry = {
                "pc": int(match.group("pc"), 16),
                "data": None if data is None else int(data, 16) & 0xFFFFFFFF,
                "line": line.rstrip(),
            }
            # gem5 emits both the architectural macro-op and its memory
            # micro-op for LR/SC.  RTL has one retirement.  Merge adjacent
            # records at the same PC and retain the observable micro-op data.
            if result and result[-1]["pc"] == entry["pc"]:
                if entry["data"] is not None:
                    result[-1] = entry
            else:
                result.append(entry)
    return result


def compare(rtl, gem5):
    if not rtl or not gem5:
        print(f"FAIL empty trace rtl={len(rtl)} gem5={len(gem5)}")
        return 1

    # Replay ELFs normally enter at PC 4 while RTL executes the PC-0 x1 clear.
    start = next((i for i, entry in enumerate(rtl)
                  if entry["pc"] == gem5[0]["pc"]), None)
    if start is None:
        print(f"FAIL gem5 first PC 0x{gem5[0]['pc']:x} is absent from RTL")
        return 1

    checked = 0

    def print_context(index, start_index, radius=4):
        first = max(0, index - radius)
        last = min(len(gem5), len(rtl) - start_index, index + radius + 1)
        print("context: idx rtl_pc gem5_pc rtl_wdata gem5_wdata")
        for nearby in range(first, last):
            expected = rtl[start_index + nearby]
            actual = gem5[nearby]
            marker = ">" if nearby == index else " "
            rtl_data = (f"0x{expected['rf_wdata']:08x}"
                        if expected["rf_we"] and
                        expected["rf_wdata_known"] else
                        (expected["rf_wdata_raw"]
                         if expected["rf_we"] else "-"))
            gem_data = (f"0x{actual['data']:08x}"
                        if actual["data"] is not None else "-")
            print(
                f"{marker} {nearby:6d} 0x{expected['pc']:08x} "
                f"0x{actual['pc']:08x} {rtl_data:>10} {gem_data:>10}"
            )

    unknown_writebacks = 0
    for index, (expected, actual) in enumerate(zip(rtl[start:], gem5)):
        if expected["pc"] != actual["pc"]:
            print(
                f"FAIL pc index={index} rtl_counter={expected['counter']} "
                f"rtl=0x{expected['pc']:x} gem5=0x{actual['pc']:x}"
            )
            print_context(index, start)
            return 1
        if expected["rf_we"] and not expected["rf_wdata_known"]:
            unknown_writebacks += 1
        elif (expected["rf_we"] and actual["data"] is not None
                and expected["rf_wdata"] != actual["data"]):
            print(
                f"FAIL writeback index={index} pc=0x{expected['pc']:x} "
                f"rtl=0x{expected['rf_wdata']:08x} "
                f"gem5=0x{actual['data']:08x}"
            )
            print_context(index, start)
            return 1
        checked += 1

    rtl_tail = max(0, len(rtl) - start - checked)
    gem5_tail = max(0, len(gem5) - checked)
    print(
        f"PASS aligned={checked} rtl_entry_skip={start} "
        f"rtl_tail={rtl_tail} gem5_tail={gem5_tail} "
        f"rtl_unknown_writebacks={unknown_writebacks}"
    )
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-jsonl", type=Path, required=True)
    parser.add_argument("--gem5-exec", type=Path, required=True)
    parser.add_argument("--task", type=int, required=True)
    parser.add_argument(
        "--gem-thread", type=int,
        help="gem5 Exec T number (defaults to the task id)",
    )
    args = parser.parse_args()

    raw, fused = read_rtl(args.rtl_jsonl, args.task)
    thread = args.task if args.gem_thread is None else args.gem_thread
    gem5 = read_gem5(args.gem5_exec, thread)
    print(
        f"RTL raw retirements={len(raw)} fused={len(fused)}; "
        f"gem5 thread T{thread}={len(gem5)}"
    )
    return compare(fused, gem5)


if __name__ == "__main__":
    raise SystemExit(main())
