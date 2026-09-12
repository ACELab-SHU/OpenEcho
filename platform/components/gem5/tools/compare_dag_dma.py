#!/usr/bin/env python3
"""Compare generic gem5 DAG output dumps with an RTL L2 DMA payload log."""

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path


RETURN_HEADER = re.compile(
    r"^src:\s*([0-9a-fA-F]+).*len:\s*([0-9a-fA-F]+).*"
    r"ret_source_tile:\s*(\d+).*task_id:\s*(\d+).*retid:\s*(\d+)"
)


def read_rtl_returns(path):
    transactions = defaultdict(dict)
    current = None
    beats = []

    def finish():
        nonlocal current, beats
        if current is None:
            return
        task, port, length = current
        values = []
        known = []
        for line in beats:
            pairs = [line[i:i + 2] for i in range(0, len(line), 2)]
            for pair in reversed(pairs):
                if "x" in pair.lower():
                    values.append(0)
                    known.append(False)
                else:
                    values.append(int(pair, 16))
                    known.append(True)
        transactions[task][port] = (bytes(values[:length]), known[:length])
        current = None
        beats = []

    for raw in path.read_text(encoding="utf-8").splitlines():
        match = RETURN_HEADER.match(raw)
        if match:
            finish()
            current = (int(match.group(4)), int(match.group(5)),
                       int(match.group(2), 16))
            continue
        if raw.startswith("src:"):
            finish()
            continue
        line = raw.strip()
        if current is not None and line and line != "data:" and \
                re.fullmatch(r"[0-9a-fA-FxX]+", line):
            if len(line) % 2:
                raise ValueError(f"odd RTL payload line length: {line}")
            beats.append(line)
    finish()
    return transactions


def manifest_outputs(path, dump_dir):
    manifest = json.loads(path.read_text(encoding="utf-8"))
    result = defaultdict(dict)
    # Version-2 dynamic-vreturn manifests deliberately use an empty
    # ``outputs`` list: the producer, not the manifest, defines each output
    # source and valid byte count.  In that case infer the comparison contract
    # from the captured GEM5 payloads rather than silently comparing nothing.
    if manifest.get("outputs"):
        for output in manifest["outputs"]:
            result[output["task"]][output["port"]] = output["length"]
    else:
        pattern = re.compile(r"task_(\d+)_port_(\d+)\.bin$")
        for output in dump_dir.glob("task_*_port_*.bin"):
            match = pattern.match(output.name)
            if match:
                result[int(match.group(1))][int(match.group(2))] = output.stat().st_size
    return result


def signature(outputs):
    return tuple(outputs[port] if isinstance(outputs[port], int)
                 else len(outputs[port][0])
                 for port in sorted(outputs))


def map_tasks(rtl, gem):
    candidates = defaultdict(list)
    for task, outputs in gem.items():
        candidates[signature(outputs)].append(task)
    mapping = {}
    used = set()
    for rtl_task, outputs in sorted(rtl.items()):
        sig = signature(outputs)
        choices = [task for task in candidates[sig] if task not in used]
        if rtl_task in choices:
            chosen = rtl_task
        elif len(choices) == 1:
            chosen = choices[0]
        else:
            print(f"SKIP rtl_task={rtl_task} output_lengths={sig} "
                  f"mapping_candidates={choices}")
            continue
        mapping[rtl_task] = chosen
        used.add(chosen)
    return mapping


def compare(rtl, gem_lengths, dump_dir, strict_x=False):
    mapping = map_tasks(rtl, gem_lengths)
    failures = 0
    for rtl_task, gem_task in sorted(mapping.items()):
        for port, (expected, known) in sorted(rtl[rtl_task].items()):
            path = dump_dir / f"task_{gem_task}_port_{port}.bin"
            actual = path.read_bytes()
            if len(actual) != len(expected):
                print(f"FAIL rtl_task={rtl_task} gem_task={gem_task} port={port} "
                      f"length rtl={len(expected)} gem5={len(actual)}")
                failures += 1
                continue
            if strict_x:
                unknown = next((i for i, is_known in enumerate(known)
                                if not is_known), None)
                if unknown is not None:
                    print(f"FAIL rtl_task={rtl_task} gem_task={gem_task} port={port} "
                          f"offset=0x{unknown:x} rtl=X gem5=0x{actual[unknown]:02x}")
                    failures += 1
                    continue
            mismatch = next((i for i, is_known in enumerate(known)
                             if is_known and actual[i] != expected[i]), None)
            checked = sum(known)
            if mismatch is None:
                print(f"PASS rtl_task={rtl_task} gem_task={gem_task} port={port} "
                      f"known_bytes={checked}/{len(expected)}")
            else:
                print(f"FAIL rtl_task={rtl_task} gem_task={gem_task} port={port} "
                      f"offset=0x{mismatch:x} rtl=0x{expected[mismatch]:02x} "
                      f"gem5=0x{actual[mismatch]:02x}")
                failures += 1
    return failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--rtl-dma-log", type=Path, required=True)
    parser.add_argument("--gem5-dump-dir", type=Path, required=True)
    parser.add_argument(
        "--strict-x", action="store_true",
        help="treat any unknown RTL payload byte as a mismatch instead of a wildcard")
    args = parser.parse_args()
    rtl = read_rtl_returns(args.rtl_dma_log)
    gem = manifest_outputs(args.manifest, args.gem5_dump_dir)
    print(f"RTL tasks with captured outputs: {len(rtl)}")
    return 1 if compare(rtl, gem, args.gem5_dump_dir, args.strict_x) else 0


if __name__ == "__main__":
    raise SystemExit(main())
