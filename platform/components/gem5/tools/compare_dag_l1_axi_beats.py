#!/usr/bin/env python3
"""Check GEM5's physical L1-DMA beat plan against RTL streamer semantics.

The RTL scheduler log records logical descriptors, not every AXI handshake.
``dma_streamer.sv`` deterministically expands each legal descriptor into
64-byte physical beats: a short descriptor or terminal tail is aligned down,
the source payload is selected by its byte offset, and the destination WSTRB
is shifted by its byte offset.  This tool expands the *observed RTL
descriptors* with those rules and compares the result to VenusL1Dma debug
output.

It intentionally does not claim AR/AW burst-handshake or cycle equivalence;
those require a fresh native RTL AXI trace.
"""

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from venus_l1_dag import parse_rtl_dma_transactions


BEAT_BYTES = 64
FULL_WSTRB = (1 << BEAT_BYTES) - 1

GEM5_BEAT = re.compile(
    r"\b(?P<channel>read|write) beat "
    r"bus_src=(?P<bus_src>0x[0-9a-fA-F]+) "
    r"bus_dst=(?P<bus_dst>0x[0-9a-fA-F]+) "
    r"bus_bytes=(?P<bus_bytes>\d+) "
    r"logical_src=(?P<logical_src>0x[0-9a-fA-F]+) "
    r"logical_dst=(?P<logical_dst>0x[0-9a-fA-F]+) "
    r"logical_bytes=(?P<logical_bytes>\d+)"
    r"(?: wstrb=(?P<wstrb>0x[0-9a-fA-F]+))?"
)


@dataclass(frozen=True)
class Beat:
    """One physical 64-byte DMA beat in increasing descriptor order."""

    transaction: int
    bus_src: int
    bus_dst: int
    logical_src: int
    logical_dst: int
    logical_bytes: int
    wstrb: int


@dataclass(frozen=True)
class Gem5Beat:
    line: int
    channel: str
    bus_src: int
    bus_dst: int
    bus_bytes: int
    logical_src: int
    logical_dst: int
    logical_bytes: int
    wstrb: Optional[int]


def describe_descriptor(entry):
    ident = entry.get("fid") if entry["direction"] == "fire" else entry.get("retid")
    return (
        f"{entry['direction']} task={entry['task']} tile={entry['tile']} "
        f"id={ident} src=0x{entry['source']:08x} "
        f"dst=0x{entry['destination']:08x} bytes={entry['length']}"
    )


def expand_descriptor(entry, transaction):
    """Apply the legal DMA descriptor-to-AXI-beat rules from dma_streamer."""

    source = entry["source"]
    destination = entry["destination"]
    length = entry["length"]
    if length == 0:
        return []
    if length >= BEAT_BYTES:
        if source % BEAT_BYTES or destination % BEAT_BYTES:
            raise ValueError(
                "RTL DMA_UNALIGNED_ERR descriptor in evidence: "
                + describe_descriptor(entry)
            )
    else:
        if ((source & (BEAT_BYTES - 1)) + length > BEAT_BYTES or
                (destination & (BEAT_BYTES - 1)) + length > BEAT_BYTES):
            raise ValueError(
                "RTL DMA_NARROW_CROSS_ERR descriptor in evidence: "
                + describe_descriptor(entry)
            )

    result = []
    offset = 0
    while offset < length:
        logical_bytes = min(BEAT_BYTES, length - offset)
        logical_src = source + offset
        logical_dst = destination + offset
        source_offset = logical_src & (BEAT_BYTES - 1)
        destination_offset = logical_dst & (BEAT_BYTES - 1)
        if logical_bytes == BEAT_BYTES:
            if source_offset or destination_offset:
                raise ValueError(
                    "unrepresentable unaligned full beat in "
                    + describe_descriptor(entry)
                )
            wstrb = FULL_WSTRB
        else:
            if (source_offset + logical_bytes > BEAT_BYTES or
                    destination_offset + logical_bytes > BEAT_BYTES):
                raise ValueError(
                    "narrow beat crosses a 64-byte AXI line in "
                    + describe_descriptor(entry)
                )
            wstrb = ((1 << logical_bytes) - 1) << destination_offset
        result.append(Beat(
            transaction=transaction,
            bus_src=logical_src - source_offset,
            bus_dst=logical_dst - destination_offset,
            logical_src=logical_src,
            logical_dst=logical_dst,
            logical_bytes=logical_bytes,
            wstrb=wstrb,
        ))
        offset += logical_bytes
    return result


def expected_beats(rtl):
    result = []
    for transaction, entry in enumerate(rtl):
        result.extend(expand_descriptor(entry, transaction))
    return result


def parse_gem5(path):
    result = {"read": [], "write": []}
    for lineno, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        match = GEM5_BEAT.search(raw)
        if not match:
            continue
        fields = match.groupdict()
        result[fields["channel"]].append(Gem5Beat(
            line=lineno,
            channel=fields["channel"],
            bus_src=int(fields["bus_src"], 0),
            bus_dst=int(fields["bus_dst"], 0),
            bus_bytes=int(fields["bus_bytes"], 10),
            logical_src=int(fields["logical_src"], 0),
            logical_dst=int(fields["logical_dst"], 0),
            logical_bytes=int(fields["logical_bytes"], 10),
            wstrb=(int(fields["wstrb"], 0) if fields["wstrb"] else None),
        ))
    return result


def format_expected(beat):
    return (
        f"bus_src=0x{beat.bus_src:08x} bus_dst=0x{beat.bus_dst:08x} "
        f"bus_bytes=64 logical_src=0x{beat.logical_src:08x} "
        f"logical_dst=0x{beat.logical_dst:08x} "
        f"logical_bytes={beat.logical_bytes} wstrb=0x{beat.wstrb:016x}"
    )


def format_actual(beat):
    strobe = "missing" if beat.wstrb is None else f"0x{beat.wstrb:016x}"
    return (
        f"line={beat.line} bus_src=0x{beat.bus_src:08x} "
        f"bus_dst=0x{beat.bus_dst:08x} bus_bytes={beat.bus_bytes} "
        f"logical_src=0x{beat.logical_src:08x} "
        f"logical_dst=0x{beat.logical_dst:08x} "
        f"logical_bytes={beat.logical_bytes} wstrb={strobe}"
    )


def compare_channel(channel, expected, actual):
    if len(expected) != len(actual):
        return (
            f"{channel} beat count RTL-plan={len(expected)} Gem5={len(actual)}",
            min(len(expected), len(actual)),
        )
    for index, (wanted, observed) in enumerate(zip(expected, actual)):
        fields = ("bus_src", "bus_dst", "logical_src", "logical_dst",
                  "logical_bytes")
        bad = [field for field in fields if getattr(wanted, field) != getattr(observed, field)]
        if observed.bus_bytes != BEAT_BYTES:
            bad.append("bus_bytes")
        if channel == "write" and observed.wstrb != wanted.wstrb:
            bad.append("wstrb")
        if bad:
            return (
                f"{channel} beat {index} (descriptor {wanted.transaction}) differs "
                f"in {', '.join(bad)}; expected {format_expected(wanted)}; "
                f"Gem5 {format_actual(observed)}",
                index,
            )
    return None, len(expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rtl-dma-log", type=Path, required=True)
    parser.add_argument("--gem5-dma-debug", type=Path, required=True)
    args = parser.parse_args()

    rtl = parse_rtl_dma_transactions(args.rtl_dma_log)
    expected = expected_beats(rtl)
    actual = parse_gem5(args.gem5_dma_debug)
    read_error, read_checked = compare_channel("read", expected, actual["read"])
    write_error, write_checked = compare_channel("write", expected, actual["write"])
    errors = [error for error in (read_error, write_error) if error]

    print("L1 64B beat/WSTRB plan: " + ("PASS" if not errors else "MISMATCH"))
    print(f"- RTL descriptors={len(rtl)}, expanded physical beats={len(expected)}")
    print(f"- Gem5 read beats={len(actual['read'])}, write beats={len(actual['write'])}")
    print(f"- checked reads={read_checked}, writes={write_checked}")
    if errors:
        print("- first divergence: " + errors[0])
        return 1
    print("- 64B bus addresses, logical byte selection, and write WSTRB match "
          "the RTL streamer rule derived from observed RTL descriptors")
    print("- AR/AW burst handshakes and cycle timing are intentionally out of scope")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
