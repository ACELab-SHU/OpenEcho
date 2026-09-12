#!/usr/bin/env python3
"""Extract matched Venus task execution boundaries from a narrow RTL VCD.

The input is expected to contain, for each observed tile, the backend's
``tile_soft_reset_n`` and ``venustile_currenttaskidreg`` signals.  This tool
does not infer timing from Scheduler allocation or DMA activity: a 0->1 reset
transition is the task execution start and the paired 1->0 transition is the
execution completion, matching the RTL observer used by Venus2.
"""

from __future__ import annotations

import argparse
from decimal import Decimal
from pathlib import Path
import re


TIMESCALE = re.compile(r"^(\d+)\s*(s|ms|us|ns|ps|fs)$")
UNIT_PS = {
    "s": Decimal("1000000000000"),
    "ms": Decimal("1000000000"),
    "us": Decimal("1000000"),
    "ns": Decimal("1000"),
    "ps": Decimal("1"),
    "fs": Decimal("0.001"),
}


def _parse_timescale(tokens: list[str]) -> Decimal:
    compact = "".join(tokens).replace("$timescale", "").replace("$end", "")
    match = TIMESCALE.fullmatch(compact.strip())
    if match is None:
        raise ValueError(f"unsupported VCD timescale: {compact!r}")
    amount, unit = match.groups()
    return Decimal(amount) * UNIT_PS[unit]


def _format_ns(value: Decimal) -> str:
    rendered = format(value.normalize(), "f")
    return "0" if rendered == "-0" else rendered


def extract(path: Path) -> list[str]:
    lines = path.read_text(encoding="utf-8", errors="strict").splitlines()
    scopes: list[str] = []
    code_to_path: dict[str, str] = {}
    timescale_tokens: list[str] = []
    collecting_timescale = False
    data_index = None
    for index, raw in enumerate(lines):
        line = raw.strip()
        if line.startswith("$timescale"):
            collecting_timescale = True
        if collecting_timescale:
            timescale_tokens.extend(line.split())
            if "$end" in line:
                collecting_timescale = False
            continue
        if line.startswith("$scope "):
            parts = line.split()
            scopes.append(parts[2])
        elif line.startswith("$upscope"):
            if not scopes:
                raise ValueError("VCD contains an unmatched $upscope")
            scopes.pop()
        elif line.startswith("$var "):
            parts = line.split()
            if len(parts) < 6:
                raise ValueError(f"malformed VCD variable: {line}")
            code = parts[3]
            reference = parts[4]
            code_to_path[code] = ".".join([*scopes, reference])
        elif line.startswith("$enddefinitions"):
            data_index = index + 1
            break
    if data_index is None:
        raise ValueError("VCD contains no $enddefinitions")
    if not timescale_tokens:
        raise ValueError("VCD contains no $timescale")
    tick_ps = _parse_timescale(timescale_tokens)

    reset_codes: dict[str, tuple[int, str]] = {}
    task_codes: dict[str, tuple[int, str]] = {}
    tile_pattern = re.compile(r"(?:^|\.)u_venus_cluster\d+_tile(\d+)(?:\.|$)")
    for code, signal in code_to_path.items():
        match = tile_pattern.search(signal)
        if match is None:
            continue
        tile = int(match.group(1))
        if signal.endswith(".tile_soft_reset_n"):
            reset_codes[code] = (tile, signal)
        elif signal.endswith(
                ".u_venus_tile_manager.venustile_currenttaskidreg"):
            task_codes[code] = (tile, signal)
    reset_by_tile = {tile: code for code, (tile, _) in reset_codes.items()}
    task_by_tile = {tile: code for code, (tile, _) in task_codes.items()}
    if not reset_by_tile or set(reset_by_tile) != set(task_by_tile):
        raise ValueError(
            "VCD must contain reset and current-task signals for the same "
            f"tiles; reset={sorted(reset_by_tile)} task={sorted(task_by_tile)}"
        )

    values: dict[str, str] = {}
    timestamp = 0
    pending: list[tuple[str, str]] = []
    output: list[str] = []
    active: dict[int, int] = {}
    seen_start: set[int] = set()
    seen_complete: set[int] = set()

    def commit() -> None:
        nonlocal pending
        if not pending:
            return
        previous = dict(values)
        for code, value in pending:
            values[code] = value
        time_ns = Decimal(timestamp) * tick_ps / Decimal(1000)
        for tile, reset_code in sorted(reset_by_tile.items()):
            old = previous.get(reset_code)
            new = values.get(reset_code)
            if old == new or old not in ("0", "1") or new not in ("0", "1"):
                continue
            task_bits = values.get(task_by_tile[tile], "")
            if not task_bits or any(bit not in "01" for bit in task_bits):
                raise ValueError(
                    f"tile {tile} reset changed at {time_ns} ns with unknown "
                    f"task id {task_bits!r}"
                )
            task = int(task_bits, 2)
            if old == "0" and new == "1":
                if task in seen_start or tile in active:
                    raise ValueError(f"duplicate/overlapping start for task {task}")
                active[tile] = task
                seen_start.add(task)
                output.append(
                    f"task {task} start execute at tile {tile} at time "
                    f"{_format_ns(time_ns)}"
                )
            elif old == "1" and new == "0":
                started = active.pop(tile, None)
                if started is None:
                    raise ValueError(f"tile {tile} completed without a start")
                if task != started:
                    # Some managers clear the task register in the same RTL
                    # time slot.  The active task remains the authoritative
                    # identity for the matched falling edge.
                    task = started
                if task in seen_complete:
                    raise ValueError(f"duplicate completion for task {task}")
                seen_complete.add(task)
                output.append(
                    f"task {task} execute complete at tile {tile} at time "
                    f"{_format_ns(time_ns)}"
                )
        pending = []

    for raw in lines[data_index:]:
        line = raw.strip()
        if not line or line.startswith("$"):
            continue
        if line.startswith("#"):
            commit()
            timestamp = int(line[1:])
            continue
        if line[0] in "01xXzZ":
            pending.append((line[1:].strip(), line[0].lower()))
        elif line[0] in "bB":
            bits, code = line[1:].split(None, 1)
            pending.append((code.strip(), bits.lower()))
    commit()
    if active:
        raise ValueError(f"VCD ended with active tiles: {active}")
    if seen_start != seen_complete or not seen_start:
        raise ValueError(
            f"unpaired or empty task boundaries: start={sorted(seen_start)} "
            f"complete={sorted(seen_complete)}"
        )
    return output


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vcd", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    rendered = "\n".join(extract(args.vcd)) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
