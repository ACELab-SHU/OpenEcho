#!/usr/bin/env python3
"""Compare a VEMU task's logged vreturn payloads with GEM5 DAG captures.

VEMU's emulator log prints each return payload as little-endian 32-bit words;
GEM5's version-2 DAG mode writes the same payloads as one file per output
port and records the actual vreturn address and length in a JSONL trace.  The
tool deliberately compares the producer-declared dynamic lengths, rather than
any scheduler output-capacity field.
"""

import argparse
import json
import re
from pathlib import Path


RETURN_HEADER = re.compile(
    r'^Task ID: (?P<task>\d+), Task Name:"(?P<name>[^"]+)", '
    r'Return ID: (?P<port>\d+), Length: (?P<length>\d+), '
    r'Address: (?P<address>[0-9a-fA-F]+)$')
RETURN_WORD = re.compile(
    r'^Task ID: (?P<task>\d+), Task Name:"(?P<name>[^"]+)", '
    r'Return ID: (?P<port>\d+), val: (?P<word>[0-9a-fA-F]{8})$')


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vemu-log", type=Path, required=True)
    parser.add_argument("--vemu-task", type=int, required=True)
    parser.add_argument("--gem5-trace", type=Path, required=True)
    parser.add_argument("--gem5-dump-dir", type=Path, required=True)
    parser.add_argument("--gem5-task", type=int, required=True)
    parser.add_argument(
        "--address-mask", type=lambda value: int(value, 0),
        default=0xffffffff,
        help=("canonical address mask applied to both traces; use 0x7fffffff "
              "when VEMU marks local addresses with bit 31"),
    )
    return parser.parse_args()


def read_vemu_returns(path, task_id):
    returns = {}
    current = None

    def finish():
        nonlocal current
        if current is None:
            return
        port, length, address, words = current
        payload = b"".join(
            int(word, 16).to_bytes(4, byteorder="little") for word in words)
        if len(payload) < length:
            raise ValueError(
                f"VEMU task {task_id} port {port}: logged {len(payload)} "
                f"bytes but return length is {length}")
        returns[port] = (address, payload[:length])
        current = None

    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        header = RETURN_HEADER.match(raw)
        if header:
            finish()
            if int(header["task"]) == task_id:
                current = (int(header["port"]), int(header["length"]),
                           int(header["address"], 16), [])
            continue

        word = RETURN_WORD.match(raw)
        if current is not None and word and int(word["task"]) == task_id and \
                int(word["port"]) == current[0]:
            current[3].append(word["word"])
    finish()

    if not returns:
        raise ValueError(f"no VEMU returns found for task {task_id} in {path}")
    return returns


def read_gem5_returns(path, task_id):
    returns = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        event = json.loads(raw)
        if event.get("task_id") != task_id:
            continue
        # ``dma_output`` is the legacy direct-output trace.  The current
        # RTL-aligned v5 scheduler emits the equivalent observation at DMA
        # admission, before the return beats complete and the DMT slot is
        # committed.  Accept both schemas so this checker validates payload
        # bytes rather than depending on a scheduler trace-version detail.
        if event.get("event") == "dma_output":
            port = int(event["output_port"])
        elif event.get("event") == "return_admit":
            port = int(event["retid"])
        else:
            continue
        returns[port] = (int(event["source"], 0), int(event["bytes"]))
    if not returns:
        raise ValueError(f"no GEM5 returns found for task {task_id} in {path}")
    return returns


def first_difference(expected, actual):
    return next((index for index, (left, right) in enumerate(zip(expected, actual))
                 if left != right), None)


def main():
    args = parse_args()
    vemu = read_vemu_returns(args.vemu_log, args.vemu_task)
    gem5 = read_gem5_returns(args.gem5_trace, args.gem5_task)
    failures = 0

    if set(vemu) != set(gem5):
        print(f"FAIL ports vemu={sorted(vemu)} gem5={sorted(gem5)}")
        failures += 1

    for port in sorted(set(vemu) & set(gem5)):
        vemu_address, expected = vemu[port]
        gem5_address, gem5_length = gem5[port]
        path = args.gem5_dump_dir / f"task_{args.gem5_task}_port_{port}.bin"
        if not path.is_file():
            print(f"FAIL port={port} missing GEM5 dump: {path}")
            failures += 1
            continue
        actual = path.read_bytes()
        normalized_vemu = vemu_address & args.address_mask
        normalized_gem5 = gem5_address & args.address_mask
        if len(expected) != gem5_length or len(expected) != len(actual):
            print(f"FAIL port={port} length vemu={len(expected)} "
                  f"gem5_trace={gem5_length} gem5_dump={len(actual)}")
            failures += 1
            continue
        if normalized_vemu != normalized_gem5:
            print(f"FAIL port={port} address vemu=0x{vemu_address:x} "
                  f"gem5=0x{gem5_address:x} mask=0x{args.address_mask:x}")
            failures += 1
            continue
        mismatch = first_difference(expected, actual)
        if mismatch is None:
            print(f"PASS port={port} bytes={len(actual)} "
                  f"address=0x{normalized_gem5:x}")
        else:
            print(f"FAIL port={port} offset=0x{mismatch:x} "
                  f"vemu=0x{expected[mismatch]:02x} "
                  f"gem5=0x{actual[mismatch]:02x}")
            failures += 1

    if failures:
        return 1
    print(f"MATCH: VEMU task {args.vemu_task} and GEM5 task {args.gem5_task} "
          "are bit-exact for all returned payloads")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
