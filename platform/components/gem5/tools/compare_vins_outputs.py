#!/usr/bin/env python3
"""Report the first per-instruction value mismatch between RTL and gem5.

The comparison is intentionally unaware of DAG names and task semantics.  A
caller selects any one RTL task directory and supplies the corresponding gem5
dump directory plus an optional instruction-number offset.
"""

import argparse
import re
from pathlib import Path


FILE_RE = re.compile(r"^(?P<op>[A-Za-z0-9]+)_(?P<id>[0-9]+)(?P<suffix>.*)\.txt$")


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl-dir", required=True, type=Path)
    parser.add_argument("--gem5-dir", required=True, type=Path)
    parser.add_argument(
        "--gem5-offset", type=int, default=0,
        help="global gem5 instruction id corresponding to RTL instruction 0",
    )
    parser.add_argument("--max-mismatches", type=int, default=1)
    parser.add_argument(
        "--match-by-id", action="store_true",
        help=("pair dumps by instruction number even when RTL and gem5 use "
              "different display names (for example SCATTER/VSHUFFLE)"),
    )
    parser.add_argument(
        "--strict-x", action="store_true",
        help="compare RTL x/z values literally instead of treating them as unknown",
    )
    return parser.parse_args()


def indexed_files(directory):
    result = {}
    for path in directory.glob("*.txt"):
        match = FILE_RE.match(path.name)
        if not match:
            continue
        key = (int(match.group("id")), match.group("op"), match.group("suffix"))
        result[key] = path
    return result


def values(path):
    return [line.strip() for line in path.read_text(errors="replace").splitlines()
            if line.strip()]


def element_equal(rtl_value, gem5_value, strict_x):
    if not strict_x and any(char in rtl_value.lower() for char in ("x", "z")):
        return True
    return rtl_value == gem5_value


def files_by_id(files, side):
    result = {}
    for (instr_id, op, suffix), path in files.items():
        key = (instr_id, suffix)
        if key in result:
            raise SystemExit(
                f"ambiguous {side} dumps for instruction {instr_id}{suffix}: "
                f"{result[key][1].name}, {path.name}"
            )
        result[key] = (op, path)
    return result


def main():
    args = parse_args()
    rtl = indexed_files(args.rtl_dir)
    gem5 = indexed_files(args.gem5_dir)
    if not rtl:
        raise SystemExit(f"no instruction dumps found in {args.rtl_dir}")

    mismatches = 0
    compared = 0
    opcode_name_differences = 0
    gem5_by_id = files_by_id(gem5, "gem5") if args.match_by_id else None
    for (rtl_id, op, suffix), rtl_path in sorted(rtl.items()):
        gem_key = (rtl_id + args.gem5_offset, op, suffix)
        gem_op = op
        if args.match_by_id:
            entry = gem5_by_id.get((gem_key[0], suffix))
            gem_op, gem_path = entry if entry is not None else (op, None)
            if gem_path is not None and gem_op != op:
                opcode_name_differences += 1
        else:
            gem_path = gem5.get(gem_key)
        if gem_path is None:
            print(f"MISSING gem5: rtl={rtl_path.name} expected id={gem_key[0]}")
            mismatches += 1
        else:
            rtl_values = values(rtl_path)
            gem_values = values(gem_path)
            compared += 1
            equal_length = len(rtl_values) == len(gem_values)
            equal_values = all(
                element_equal(rtl_value, gem_value, args.strict_x)
                for rtl_value, gem_value in zip(rtl_values, gem_values)
            )
            if not equal_length or not equal_values:
                common = min(len(rtl_values), len(gem_values))
                first = next((i for i in range(common)
                              if not element_equal(rtl_values[i], gem_values[i],
                                                   args.strict_x)), common)
                if first < common:
                    detail = (f"element={first} rtl={rtl_values[first]} "
                              f"gem5={gem_values[first]}")
                else:
                    detail = (f"common elements equal; lengths "
                              f"rtl={len(rtl_values)} gem5={len(gem_values)}")
                print(f"MISMATCH instruction={rtl_id} op={op}{suffix}: {detail}")
                mismatches += 1

        if mismatches >= args.max_mismatches:
            break

    if mismatches == 0:
        detail = f", opcode display-name differences={opcode_name_differences}"
        print(f"MATCH: {compared} instruction dump(s){detail}")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
