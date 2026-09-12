"""Fail-closed read-only Venus DMA reconstruction from a narrow VCD.

Expected bytes are deliberately not an input. Packed-field offsets are supplied
by a separately hash-qualified backend observer contract. AXI samples use the
stable values BEFORE the rising-edge timestamp, not the post-NBA values.
"""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass, field
from pathlib import Path
import re
from typing import Iterator, TextIO


class CaptureError(ValueError):
    pass


def _bits(value: str, width: int) -> str:
    value = value.lower()
    if not value or len(value) > width or re.fullmatch("[01xz]+", value) is None:
        raise CaptureError(f"invalid {width}-bit VCD value {value!r}")
    return value.rjust(width, value[0] if value[0] in "xz" else "0")


def rising_samples(stream: TextIO, contract: dict) -> Iterator[tuple[int, dict[str, str]]]:
    """Stream pre-edge snapshots; reject malformed or ambiguous clock evidence."""
    scopes: list[str] = []
    codes: dict[str, tuple[str, int]] = {}
    names: set[str] = set()
    timescale = None
    directive = ""
    for raw in stream:
        line = raw.strip()
        if not line:
            continue
        if directive:
            directive += " " + line
        else:
            directive = line
        if not directive.endswith("$end"):
            continue
        parts = directive.split()
        if parts[0] == "$timescale":
            timescale = "".join(parts[1:-1])
        elif parts[0] == "$scope":
            scopes.append(parts[2])
        elif parts[0] == "$upscope":
            if not scopes:
                raise CaptureError("unpaired VCD scope")
            scopes.pop()
        elif parts[0] == "$var" and ".".join(scopes) == contract["hierarchy"]:
            if len(parts) < 6:
                raise CaptureError("malformed VCD variable")
            name, code, width = parts[4], parts[3], int(parts[2])
            if name in contract["signals"]:
                if name in names or code in codes or width != contract["signals"][name]:
                    raise CaptureError(f"ambiguous/wrong-width signal {name}")
                codes[code] = (name, width)
                names.add(name)
        elif parts[0] == "$enddefinitions":
            break
        directive = ""
    else:
        raise CaptureError("missing VCD definitions")
    if names != set(contract["signals"]) or timescale != contract["timescale"]:
        raise CaptureError("incomplete signal set or wrong timescale")
    values: dict[str, str] = {}
    timestamp = None
    changes: list[tuple[str, str]] = []
    clock = contract["clock"]

    def commit():
        before = dict(values)
        clock_changes = []
        for name, value in changes:
            if name == clock and value != values.get(clock):
                clock_changes.append(value)
            values[name] = value
        changes.clear()
        if len(clock_changes) > 1:
            raise CaptureError(f"ambiguous clock delta transitions at {timestamp}")
        if before.get(clock) in ("0", "1") and values.get(clock) not in ("0", "1"):
            raise CaptureError(f"clock became unknown at {timestamp}")
        if before.get(clock) == "0" and values.get(clock) == "1":
            if set(before) != names:
                raise CaptureError("uninitialized sampled signal")
            return timestamp, before
        return None

    in_comment = False
    for raw in stream:
        line = raw.strip()
        if not line:
            continue
        if in_comment:
            in_comment = "$end" not in line
            continue
        if line.startswith("$comment"):
            in_comment = "$end" not in line
            continue
        if line in ("$dumpvars", "$end"):
            continue
        if line.startswith("$"):
            raise CaptureError(f"unsupported/gapped VCD directive {line}")
        if line.startswith("#"):
            current = int(line[1:])
            if current < 0 or timestamp is not None and current < timestamp:
                raise CaptureError("VCD time reversed")
            if timestamp is None or current > timestamp:
                sample = commit()
                if sample is not None:
                    yield sample
                timestamp = current
            continue
        if timestamp is None:
            raise CaptureError("VCD values without timestamp")
        if line[0] in "01xXzZ":
            value, code = line[0], line[1:].strip()
        elif line[0] in "bB":
            try:
                value, code = line[1:].split()
            except ValueError as exc:
                raise CaptureError("malformed vector") from exc
        else:
            raise CaptureError(f"unsupported VCD value {line[:80]}")
        if code in codes:
            name, width = codes[code]
            changes.append((name, _bits(value, width)))
    sample = commit()
    if sample is not None:
        yield sample


@dataclass
class Burst:
    address: int
    id: int
    beats: int
    time: int
    accepted: int = 0
    last_time: int | None = None


@dataclass
class Transfer:
    index: int
    source: int
    destination: int
    length: int
    start_time_ps: int
    payload: bytearray = field(default_factory=bytearray)
    bursts: int = 0
    beats: int = 0
    responses: int = 0
    stalled_w_cycles: int = 0
    end_time_ps: int | None = None

    def record(self):
        return {"index": self.index, "source": self.source,
                "destination": self.destination, "length": self.length,
                "payload": bytes(self.payload), "observed_bytes": len(self.payload),
                "start_time_ps": self.start_time_ps, "end_time_ps": self.end_time_ps,
                "bursts": self.bursts, "accepted_beats": self.beats,
                "responses": self.responses, "stalled_w_cycles": self.stalled_w_cycles}


class AcceptedWriteDecoder:
    """Backend-scoped single-descriptor DMA with ordered AXI4 write data.

    W-before-AW is rejected as unsupported evidence for this backend (never
    silently attached to a predicted address). Multiple AW/B outstanding with
    ordered data and same-ID responses are supported.
    """
    def __init__(self, contract: dict):
        if contract.get("data_bytes") != 64 or contract.get("max_burst_beats") != 64:
            raise CaptureError("unqualified DMA geometry")
        self.contract = contract
        self.transfers: list[dict] = []
        self.current: Transfer | None = None
        self.aw: deque[Burst] = deque()
        self.responses: dict[int, deque[Burst]] = {}
        self.pending_stall: dict[str, tuple] = {}
        self.seen_reset = False
        self.previous_go = 0
        self.previous_done = 0
        self.last_time = -1
        self.clock_samples = 0

    def sample(self, time: int, values: dict[str, str]):
        if time <= self.last_time:
            raise CaptureError("non-increasing sample time")
        self.last_time = time
        self.clock_samples += 1

        def bits(name):
            signal, lo, width = self.contract["fields"][name]
            raw = values[signal]
            hi = len(raw) - lo
            result = raw[hi-width:hi]
            if len(result) != width:
                raise CaptureError(f"invalid field {name}")
            return result

        def number(name):
            value = bits(name)
            if re.fullmatch("[01]+", value) is None:
                raise CaptureError(f"unknown {name} at {time}")
            return int(value, 2)

        reset = number("reset")
        if not reset:
            if self.current or self.aw or any(self.responses.values()):
                raise CaptureError("reset interrupted transfer")
            self.seen_reset = True
            self.previous_go = self.previous_done = 0
            self.pending_stall.clear()
            return
        if not self.seen_reset:
            raise CaptureError("capture lacks reset initialization")
        go, active, done = number("go"), number("active"), number("done")
        if number("error") or number("error_valid"):
            raise CaptureError(f"DMA error at {time}")
        if go and not self.previous_go:
            if self.current is not None:
                raise CaptureError("overlapping descriptors")
            length = number("length")
            if not 0 < length <= 2**32 - 1:
                raise CaptureError("unsupported empty/overflow descriptor")
            src, dst = number("source"), number("destination")
            if max(src, dst) + length > 2**32:
                raise CaptureError("descriptor address overflow")
            self.current = Transfer(len(self.transfers), src, dst, length, time)
        if go and self.current is not None:
            if (number("source"), number("destination"), number("length")) != (
                    self.current.source, self.current.destination, self.current.length):
                raise CaptureError("descriptor changed while go asserted")
        self.previous_go = go
        av, wv, bv = number("awvalid"), number("wvalid"), number("bvalid")
        ar = number("awready") if av else 0
        wr = number("wready") if wv else 0
        br = number("bready") if bv else 0
        # Validate held VALID/payload across backpressure; disabled W bytes may
        # be unknown and are not semantically compared.
        signatures = {}
        if av:
            signatures["aw"] = tuple(number(n) for n in ("awaddr", "awid", "awlen", "awsize", "awburst"))
        if wv:
            strobe, data = number("wstrb"), bits("wdata")
            valid_data = tuple(data[len(data)-8*(i+1):len(data)-8*i] for i in range(64) if strobe >> i & 1)
            signatures["w"] = (strobe, number("wlast"), valid_data)
        if bv:
            signatures["b"] = (number("bid"), number("bresp"))
        for channel, signature in self.pending_stall.items():
            if signatures.get(channel) != signature:
                raise CaptureError(f"{channel} changed/dropped during backpressure")
        self.pending_stall = {c: signatures[c] for c, v, r in
                              (("aw", av, ar), ("w", wv, wr), ("b", bv, br)) if v and not r}
        if av or wv or bv:
            if self.current is None or not active:
                raise CaptureError("AXI traffic outside active descriptor")
        if av and ar:
            address, ident, length, size, burst_type = signatures["aw"]
            beats = length + 1
            if (size != self.contract["awsize"] or burst_type != self.contract["burst_type"]
                    or beats > self.contract["max_burst_beats"] or address % 64
                    or address % 4096 + beats * 64 > 4096):
                raise CaptureError("unsupported burst size/length/alignment/boundary")
            self.aw.append(Burst(address, ident, beats, time))
            self.current.bursts += 1
        if wv and not wr:
            self.current.stalled_w_cycles += 1
        if wv and wr:
            if not self.aw:
                raise CaptureError("accepted W has no AW")
            burst = self.aw[0]
            strobe, last, data = signatures["w"]
            if last != (burst.accepted + 1 == burst.beats):
                raise CaptureError("WLAST does not match AWLEN")
            address = burst.address + burst.accepted * 64
            it = iter(data)
            for lane in range(64):
                if not (strobe >> lane & 1):
                    continue
                value = next(it)
                if re.fullmatch("[01]{8}", value) is None:
                    raise CaptureError("unknown accepted valid byte")
                expected_address = self.current.destination + len(self.current.payload)
                if address + lane != expected_address or len(self.current.payload) >= self.current.length:
                    raise CaptureError("missing/duplicate/reordered/out-of-range accepted byte")
                self.current.payload.append(int(value, 2))
            burst.accepted += 1
            self.current.beats += 1
            if last:
                burst.last_time = time
                self.aw.popleft()
                self.responses.setdefault(burst.id, deque()).append(burst)
        if bv and br:
            ident, response = signatures["b"]
            queue = self.responses.get(ident)
            if response != 0 or not queue:
                raise CaptureError("missing/mismatched/error B response")
            burst = queue.popleft()
            if burst.last_time >= time:
                raise CaptureError("B response precedes completed write")
            self.current.responses += 1
        if done and not self.previous_done:
            if (self.current is None or self.aw or any(self.responses.values())
                    or self.pending_stall or active
                    or len(self.current.payload) != self.current.length
                    or self.current.bursts != self.current.responses):
                raise CaptureError("DMA done with incomplete accepted transfer")
            self.current.end_time_ps = time
            self.transfers.append(self.current.record())
            self.current = None
        self.previous_done = done

    def finish(self):
        if (self.current or self.aw or any(self.responses.values())
                or self.pending_stall or not self.transfers):
            raise CaptureError("empty or truncated accepted-beat capture")
        return self.transfers


def reconstruct(path: Path, contract: dict) -> tuple[list[dict], int]:
    decoder = AcceptedWriteDecoder(contract)
    with path.open(encoding="ascii") as stream:
        for time, values in rising_samples(stream, contract):
            decoder.sample(time, values)
    return decoder.finish(), decoder.clock_samples
