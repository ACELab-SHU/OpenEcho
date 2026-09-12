#!/usr/bin/env python3
"""Decode the runtime Venus DAG embedded in an L1 ELF.

The decoder follows ``l2_scheduler_pkg.sv``.  It deliberately does not know
task names or application-specific graph topology: both come from ELF symbols
and the packed task containers consumed by the RTL L2 scheduler.
"""

import argparse
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path

from venus_dag import find_tool


class Elf32:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if self.data[:4] != b"\x7fELF" or self.data[4:6] != b"\x01\x01":
            raise ValueError(f"{path} is not a little-endian ELF32 file")
        header = struct.unpack_from("<16sHHIIIIIHHHHHH", self.data, 0)
        shoff, shentsize, shnum, shstrndx = header[6], header[11], header[12], header[13]
        if shentsize != 40:
            raise ValueError(f"unsupported ELF32 section header size {shentsize}")
        self.sections = [
            struct.unpack_from("<IIIIIIIIII", self.data, shoff + i * shentsize)
            for i in range(shnum)
        ]
        self.symbols = {}
        for section in self.sections:
            sh_type, sh_offset, sh_size, sh_link, sh_entsize = (
                section[1], section[4], section[5], section[6], section[9]
            )
            if sh_type not in (2, 11) or not sh_entsize:
                continue
            strings = self._section_data(sh_link)
            for offset in range(sh_offset, sh_offset + sh_size, sh_entsize):
                name_off, value, size, info, other, shndx = struct.unpack_from(
                    "<IIIBBH", self.data, offset
                )
                name = self._cstring(strings, name_off)
                if name:
                    self.symbols[name] = (value, size, shndx, info, other)

    def _section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]

    @staticmethod
    def _cstring(data, offset):
        end = data.find(b"\0", offset)
        return data[offset:end if end >= 0 else len(data)].decode(
            "utf-8", errors="replace"
        )

    def symbol_data(self, name):
        value, size, shndx, _, _ = self.symbols[name]
        if shndx == 0 or shndx >= len(self.sections):
            raise ValueError(f"symbol {name} has no file-backed section")
        section = self.sections[shndx]
        offset = section[4] + value - section[3]
        return self.data[offset:offset + size]


def bits(value, offset, width):
    return (value >> offset) & ((1 << width) - 1)


def decode_monitor_beat(text, context):
    """Decode one monitor beat printed MSB first.

    RTL monitor dumps often fill the unused high bytes of a short transfer
    with ``x``.  A known low byte remains useful input data, while an unknown
    byte must remain unspecified rather than being turned into a made-up zero.
    Return bytes in increasing-address order with ``None`` for unknown bytes.
    """
    if len(text) % 2 or not re.fullmatch(r"[0-9a-fA-FxX]+", text):
        raise ValueError(f"invalid RTL monitor beat at {context}")
    msb_first = []
    for offset in range(0, len(text), 2):
        token = text[offset:offset + 2]
        msb_first.append(None if "x" in token.lower() else int(token, 16))
    return list(reversed(msb_first))


def overlay_known_bytes(base, observed, length):
    """Overlay observed monitor bytes onto an existing L1/L2 image slice.

    ``base`` is the ELF-derived image before this capture.  Unknown monitor
    bytes deliberately retain that image instead of becoming artificial data.
    """
    payload = bytearray(base[:length])
    if len(payload) < length:
        payload.extend(b"\0" * (length - len(payload)))
    known = 0
    for offset, value in enumerate(observed[:length]):
        if value is None:
            continue
        payload[offset] = value
        known += 1
    return bytes(payload), known


def parse_integer(value, context):
    """Accept JSON integers and conventional decimal/hexadecimal strings."""
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError as error:
            raise ValueError(f"invalid integer {value!r} for {context}") from error
    raise ValueError(f"missing or invalid integer for {context}")


def discover_prefix(elf, requested=None):
    suffix = "_task_container"
    prefixes = sorted(name[:-len(suffix)] for name in elf.symbols if name.endswith(suffix))
    if requested:
        if requested not in prefixes:
            raise ValueError(f"no runtime DAG symbols with prefix {requested!r}")
        return requested
    if len(prefixes) != 1:
        raise ValueError(f"expected one runtime DAG symbol prefix, found {prefixes}")
    return prefixes[0]


def align_up(value, alignment):
    if alignment <= 0 or alignment & (alignment - 1):
        raise ValueError(f"alignment must be a positive power of two, got {alignment}")
    return (value + alignment - 1) & -alignment


def decode_dmt_layout(elf, dag_path, prefix, tasks):
    """Decode the static DMT allocation map shipped with a runtime DAG.

    The RTL does not invent a new storage address when a tile returns data.
    ``task_manager`` reads a DMT entry whose relative offset was generated
    with the DAG, then adds the L1-programmed ``L2_malloc`` base.  The human
    readable payload JSON is the producer-side source for those offsets;
    ``<prefix>_binandjsonsize`` in the L1 ELF is the value programmed by the
    overall testbench.  Keeping both values in the manifest avoids using an
    observed return-DMA address as model behaviour.
    """
    size_symbol = f"{prefix}_binandjsonsize"
    if size_symbol not in elf.symbols:
        raise ValueError(f"missing runtime DAG symbol {size_symbol}")
    size_data = elf.symbol_data(size_symbol)
    if len(size_data) < 4:
        raise ValueError(f"runtime DAG symbol {size_symbol} is shorter than 4 bytes")
    malloc_base = align_up(int.from_bytes(size_data[:4], "little"), 64)

    json_path = dag_path.parent / "payload" / f"{prefix}.json"
    if not json_path.is_file():
        raise ValueError(
            "cannot derive semantic DMT offsets: expected runtime payload JSON "
            f"{json_path}")
    try:
        raw_tasks = json.loads(json_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot parse runtime payload JSON {json_path}") from error
    if not isinstance(raw_tasks, list):
        raise ValueError(f"runtime payload JSON {json_path} is not a task array")

    by_id = {}
    for index, raw_task in enumerate(raw_tasks):
        if not isinstance(raw_task, dict):
            raise ValueError(f"{json_path}: task {index} is not an object")
        # Scheduler JSON appends one aggregate return-output object after the
        # per-task records.  It has no current_taskId and is not a task
        # container/DMT producer.
        if "current_taskId" not in raw_task:
            if set(raw_task) == {"return_output"}:
                continue
            raise ValueError(
                f"{json_path}: object {index} is neither a task nor the "
                "recognized return_output aggregate")
        task_id = parse_integer(raw_task.get("current_taskId"),
                                f"{json_path}: task {index}.current_taskId")
        if task_id in by_id:
            raise ValueError(f"{json_path}: duplicate current_taskId {task_id}")
        by_id[task_id] = raw_task

    slots = []
    for task in tasks:
        task_id = task["id"]
        raw_task = by_id.get(task_id)
        if raw_task is None:
            raise ValueError(f"{json_path}: missing task {task_id}")
        raw_outputs = raw_task.get("all_output")
        if not isinstance(raw_outputs, list):
            raise ValueError(f"{json_path}: task {task_id} has no all_output array")
        if len(raw_outputs) != task["output_count"]:
            raise ValueError(
                f"{json_path}: task {task_id} has {len(raw_outputs)} outputs, "
                f"but ELF declares {task['output_count']}")
        for port, raw_output in enumerate(raw_outputs):
            if not isinstance(raw_output, dict):
                raise ValueError(
                    f"{json_path}: task {task_id} output {port} is not an object")
            relative_offset = parse_integer(
                raw_output.get("temp_offset"),
                f"{json_path}: task {task_id} output {port}.temp_offset")
            capacity = parse_integer(
                raw_output.get("length"),
                f"{json_path}: task {task_id} output {port}.length")
            if relative_offset < 0 or capacity < 0:
                raise ValueError(
                    f"{json_path}: task {task_id} output {port} has negative DMT fields")
            slots.append({
                "task": task_id,
                "port": port,
                "relative_offset": relative_offset,
                "capacity": capacity,
            })
    return {
        "malloc_base": malloc_base,
        "slots": slots,
        "json": str(json_path.resolve()),
        "json_sha256": hashlib.sha256(json_path.read_bytes()).hexdigest(),
    }


def decode_task(raw, task_id, task_input_type_bits=3,
                task_container_spmd_fields=True):
    if len(raw) != 384:
        raise ValueError(f"task {task_id} container is {len(raw)} bytes, expected 384")
    if task_input_type_bits not in (2, 3):
        raise ValueError(
            "task input descriptor type width must be 2 or 3 bits, got "
            f"{task_input_type_bits}"
        )
    value = int.from_bytes(raw, "little")
    descriptors = []
    descriptor_width = 42 + task_input_type_bits
    for index in range(64):
        desc = bits(value, index * descriptor_width, descriptor_width)
        descriptors.append({
            "index": index,
            "destination": bits(desc, 0, 32),
            "output_port": bits(desc, 32, 4),
            "parent_task": bits(desc, 36, 6),
            "type": bits(desc, 42, task_input_type_bits),
        })
    offset = 64 * descriptor_width
    input_count = bits(value, offset, 7); offset += 7
    if task_container_spmd_fields:
        minimum_spmd = bits(value, offset, 6); offset += 6
        need_spmd = bits(value, offset, 1); offset += 1
    else:
        minimum_spmd = 1
        need_spmd = 0
    hardware = bits(value, offset, 11); offset += 11
    data_length = bits(value, offset, 25); offset += 25
    data_address = bits(value, offset, 25); offset += 25
    code_length = bits(value, offset, 22); offset += 22
    code_address = bits(value, offset, 22); offset += 22
    crc = bits(value, offset, 16)
    return {
        "id": task_id,
        "name": f"runtime_task_{task_id}",
        "crc": crc,
        "code_address": code_address,
        "code_length": code_length,
        "data_address": data_address,
        "data_length": data_length,
        "hardware_requirement": hardware,
        "need_spmd": need_spmd,
        "minimum_spmd_tasks": minimum_spmd,
        "inputs": descriptors[:input_count],
    }


def decode_runtime_dag(path, prefix=None, task_input_type_bits=3,
                       task_container_spmd_fields=True):
    elf_path = Path(path).resolve()
    elf = Elf32(elf_path)
    prefix = discover_prefix(elf, prefix)
    required = ["bin", "task_num", "output_num", "global_para", "task_container"]
    symbols = {kind: f"{prefix}_{kind}" for kind in required}
    missing = [name for name in symbols.values() if name not in elf.symbols]
    if missing:
        raise ValueError(f"missing runtime DAG symbols: {missing}")
    blob = elf.symbol_data(symbols["bin"])
    task_count = int.from_bytes(elf.symbol_data(symbols["task_num"])[:4], "little")
    output_raw = elf.symbol_data(symbols["output_num"])
    output_counts = [bits(int.from_bytes(output_raw, "little"), i * 4, 4)
                     for i in range(task_count)]
    global_raw = elf.symbol_data(symbols["global_para"])
    globals_ = []
    for index in range(len(global_raw) // 64):
        entry = int.from_bytes(global_raw[index * 64:(index + 1) * 64], "little")
        globals_.append({"source": bits(entry, 16, 28), "length": bits(entry, 0, 16)})
    containers = elf.symbol_data(symbols["task_container"])
    if len(containers) < task_count * 384:
        raise ValueError("task container symbol is shorter than task_num")
    tasks = [decode_task(containers[i * 384:(i + 1) * 384], i,
                         task_input_type_bits, task_container_spmd_fields)
             for i in range(task_count)]
    for task, output_count in zip(tasks, output_counts):
        task["output_count"] = output_count
    dmt_layout = decode_dmt_layout(elf, Path(path), prefix, tasks)
    return {
        "prefix": prefix,
        # Firmware execution must be tied to exactly the ELF whose packed
        # runtime DAG was decoded.  This is provenance, not a timing knob.
        "scheduler_firmware": {
            "elf": str(elf_path),
            "elf_sha256": hashlib.sha256(elf_path.read_bytes()).hexdigest(),
        },
        "blob": blob,
        "globals": globals_,
        "tasks": tasks,
        "dmt_layout": dmt_layout,
        # Explicit non-DAG backing is empty by default.  It is populated only
        # by --l2-backing-spec, never inferred from arbitrary LSU reads.
        "l2_backing_inputs": [],
    }


def parse_csv_dma_records(lines, blob, tasks):
    """Decode the comma-separated post-synthesis L2 DMA monitor.

    This format omits task_id.  Identify each dispatch by matching writes in
    the ISPM window against the reconstructed task code image, then tag later
    transfers on the same physical tile.  The RTL DMA can elide an unchanged
    prefix when reusing a tile, so a dispatch is not required to start with a
    write at ISPM offset zero.
    """
    raw_records = []
    for line in lines:
        fields = [field.strip() for field in line.split(",")]
        if len(fields) < 11 or fields[5] != "W":
            continue
        try:
            address = int(fields[6], 16)
        except ValueError:
            continue
        beats = []
        for field in fields[10:]:
            match = re.fullmatch(
                r"([0-9a-fA-FxX]+)\(([0-9a-fA-FxX]+)\)", field)
            if not match:
                continue
            data_text, strobe_text = match.groups()
            if "x" in data_text.lower() or "x" in strobe_text.lower():
                continue
            beat = bytes.fromhex(data_text)[::-1]
            strobe = int(strobe_text, 16)
            beats.append(bytes(value for byte, value in enumerate(beat)
                               if (strobe >> byte) & 1))
        # L2_DMA_trx.log can place residual W data from an earlier burst
        # before the beats belonging to the AW record on the same CSV row.
        # AXI AWLEN (field 7) is authoritative and encodes beats minus one;
        # retain the trailing AWLEN+1 beats associated with this address.
        # Without this, a one-beat VSPM table write may incorrectly absorb
        # several hundred bytes from its predecessor.
        try:
            expected_beats = int(fields[7], 16) + 1
        except ValueError:
            expected_beats = len(beats)
        if len(beats) > expected_beats:
            beats = beats[-expected_beats:]
        payload = bytearray().join(beats)
        if payload:
            raw_records.append({"address": address, "payload": bytes(payload)})

    code_images = {
        task["id"]: blob[task["code_address"]:
                         task["code_address"] + task["code_length"]]
        for task in tasks
    }
    current_task = {}
    for record in raw_records:
        address = record["address"]
        local = address & 0x1fffff
        tile_access = (address & 0xff000000) == 0x82000000
        tile = (address >> 21) & 0x7
        if tile_access and local < 0x20000:
            payload = record["payload"]
            scores = {}
            # A CSV row can contain W data from several contiguous AXI
            # bursts.  Consequently the code payload can start at a later
            # beat even though the recorded AW address is the ISPM base.
            # Score aligned occurrences of each code prefix and select the
            # unique longest match.
            for task_id, image in code_images.items():
                best = 0
                for start in range(0, len(payload), 64):
                    limit = min(len(image), len(payload) - start)
                    matched = 0
                    while (matched < limit and
                           payload[start + matched] == image[matched]):
                        matched += 1
                    best = max(best, matched)
                scores[task_id] = best
            best = max(scores.values(), default=0)
            matches = [task_id for task_id, score in scores.items()
                       if score == best and score >= 256]
            if len(matches) == 1:
                current_task[tile] = matches[0]
        record["task"] = current_task.get(tile)

    records = []
    for record in raw_records:
        if record["task"] is None:
            continue
        address = record["address"]
        records.append({
            "source": 0,
            "destination": address & 0x1fffff,
            "length": len(record["payload"]),
            "task": record["task"],
            "fid": 0,
            "payload": record["payload"],
            "tile_access": (address & 0xff000000) == 0x82000000,
            "tile": (address >> 21) & 0x7,
        })
    return records


def parse_rtl_dma_transactions(path, blob=b""):
    """Parse the structured L2-DMA dump emitted by ``dma_func_wrapper``.

    The wrapper records a descriptor when ``dma_go_i`` is sampled, which is
    the architectural transaction-issue boundary used by the RTL scheduler;
    it is deliberately *not* treated as a DMA-completion timestamp here.
    Both fire and return transactions are retained.  The payload is present
    only so callers that explicitly materialize a post-L1 fixture can use the
    observed bytes; timing models must not consume ``time_ns`` as an oracle.
    """
    lines = Path(path).read_text(encoding="utf-8", errors="replace").splitlines()
    header = re.compile(
        r"^src:\s+([0-9a-fA-F]+)\s+\|\s+dst:\s+([0-9a-fA-F]+)"
        r"\s+\|\s+len:\s+([0-9a-fA-F]+)\s+\|\s+"
        r"(fire_target_tile|ret_source_tile):\s*(\d+)\s+\|\s+"
        r"task_id:\s*(\d+)\s+\|\s+(fid|retid):\s*(\d+)"
        r"\s+\|\s+time:\s*([0-9]+(?:\.[0-9]+)?)\s*ns$")
    records = []
    index = 0
    while index < len(lines):
        match = header.search(lines[index])
        if not match:
            index += 1
            continue
        source = int(match.group(1), 16)
        destination = int(match.group(2), 16)
        length = int(match.group(3), 16)
        direction = "fire" if match.group(4) == "fire_target_tile" else "return"
        tile = int(match.group(5))
        task = int(match.group(6))
        transfer_key = match.group(7)
        transfer_id = int(match.group(8))
        time_ns = float(match.group(9))
        index += 1
        if index < len(lines) and lines[index].strip() == "data:":
            index += 1
        observed = []
        while (index < len(lines) and
               re.fullmatch(r"[0-9a-fA-FxX]+", lines[index].strip())):
            observed.extend(decode_monitor_beat(
                lines[index].strip(), f"{path}:{index + 1}"))
            index += 1
        payload, known_bytes = overlay_known_bytes(blob[:0], observed, length)
        record = {
            "direction": direction,
            "source": source,
            "destination": destination,
            "destination_local": destination & 0x1fffff,
            "length": length,
            "task": task,
            "tile": tile,
            "time_ns": time_ns,
            # Keep this internal representation for post-L1 materialization:
            # unknown monitor bytes remain ``None`` rather than becoming
            # invented zeroes.  Manifest writers intentionally omit it.
            "observed": observed,
            "payload": payload,
            "known_bytes": known_bytes,
        }
        record[transfer_key] = transfer_id
        records.append(record)
    return records


def overlay_rtl_dma_inputs(blob, path, tasks):
    """Overlay L1-initialized bytes from a generic RTL L2 DMA capture.

    This is an explicit validation fixture, not a hardware-model default.  It
    represents the memory image after L1 has materialized runtime parameters.
    ``parse_rtl_dma_transactions`` remains the single parser so return-DMA
    evidence cannot silently disappear from the L1 comparison path.
    """
    result = bytearray(blob)
    seen = set()
    transactions = parse_rtl_dma_transactions(path)
    records = []
    applied = 0
    for transaction in transactions:
        if transaction["direction"] != "fire":
            continue
        source_full = transaction["source"]
        source = source_full & 0x7fffffff
        destination = transaction["destination_local"]
        length = transaction["length"]
        observed = transaction["observed"]
        image_slice = result[source:min(source + length, len(result))]
        payload, known_bytes = overlay_known_bytes(image_slice, observed, length)
        records.append({
            "source": source_full,
            "destination": destination,
            "destination_full": transaction["destination"],
            "length": length,
            "task": transaction["task"],
            "fid": transaction["fid"],
            "payload": payload,
            "known_bytes": known_bytes,
            "tile_access": True,
            "tile": transaction["tile"],
            "time_ns": transaction["time_ns"],
        })
        if source in seen or source >= len(result):
            continue
        seen.add(source)
        end = min(source + length, len(result))
        for offset, value in enumerate(observed[:end - source]):
            if value is not None:
                result[source + offset] = value
                applied += 1
    if not records:
        lines = Path(path).read_text(encoding="utf-8", errors="replace").splitlines()
        records = parse_csv_dma_records(lines, blob, tasks)
        for record in records:
            if record["tile_access"]:
                continue
            source = record["destination"] & 0x0fffffff
            end = min(source + len(record["payload"]), len(result))
            if source < end:
                result[source:end] = record["payload"][:end - source]
                applied += end - source
    return bytes(result), applied, records


def collect_rtl_lsu_bytes(paths, allowed_ranges=None):
    """Return first-observed, physical-byte RTL LSU read responses.

    ``araddr`` on this RTL's 512-bit AXI interface can be unaligned.  Each
    returned data beat is nevertheless aligned to the 64-byte bus word.  Map
    response byte zero to ``araddr & ~0x3f`` rather than to ``araddr``; the
    latter silently shifts every byte after an unaligned read and is not an
    architectural LDU image.

    ``allowed_ranges`` is deliberately caller supplied.  This parser is used
    only for declared immutable inputs/backing fixtures; it never treats every
    later RTL read as a time-zero input.
    """
    if isinstance(paths, (str, Path)):
        paths = [paths]
    paths = [Path(path) for path in paths]
    if allowed_ranges is not None:
        allowed_ranges = list(allowed_ranges)

    def in_allowed_range(address):
        return (allowed_ranges is None or any(
            start <= address < end for start, end in allowed_ranges))

    transfer = re.compile(
        r"^transfer:\s+read\b.*\baraddr:\s*([0-9a-fA-F]+)"
        r".*\bbeats:\s*(\d+)"
    )
    response = re.compile(r"^read_data\b.*\|\s*([0-9a-fA-FxX]+)\s*$")
    timestamp = re.compile(r"\btime:\s*([0-9]+(?:\.[0-9]+)?)\s*ns")
    reads = []
    for file_index, path in enumerate(paths):
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        index = 0
        sequence = 0
        while index < len(lines):
            match = transfer.search(lines[index])
            if not match:
                index += 1
                continue
            time_match = timestamp.search(lines[index])
            time_ns = float(time_match.group(1)) if time_match else None
            address = int(match.group(1), 16) & 0x0fffffff
            beats = int(match.group(2))
            index += 1
            while index < len(lines) and lines[index].strip() == "data:":
                index += 1
            observed_bytes = []
            beat_index = 0
            while index < len(lines) and beat_index < beats:
                data_match = response.search(lines[index])
                if not data_match:
                    break
                values = decode_monitor_beat(
                    data_match.group(1), f"{path}:{index + 1}")
                # AXI returns a full, aligned data word even if ARADDR is
                # unaligned.  The LDU selects its first useful lane using
                # ARADDR's byte offset within this beat.
                beat_base = address & ~(len(values) - 1)
                for byte_offset, value in enumerate(values):
                    if value is None:
                        continue
                    byte_address = (beat_base + beat_index * len(values) +
                                    byte_offset)
                    if in_allowed_range(byte_address):
                        observed_bytes.append((byte_address, value))
                beat_index += 1
                index += 1
            reads.append({
                "time_ns": time_ns, "file_index": file_index,
                "sequence": sequence, "bytes": observed_bytes,
            })
            sequence += 1

    # When a monitor has no time field, retain caller order deterministically
    # rather than guessing an inter-file ordering.
    reads.sort(key=lambda item: (
        item["time_ns"] is None,
        item["time_ns"] if item["time_ns"] is not None else 0.0,
        item["file_index"], item["sequence"],
    ))
    observed = {}
    conflicts = 0
    for read in reads:
        for byte_address, value in read["bytes"]:
            if byte_address in observed:
                if observed[byte_address] != value:
                    conflicts += 1
                continue
            observed[byte_address] = value
    return observed, len(reads), conflicts


def overlay_rtl_lsu_globals(blob, paths, globals_):
    """Recover declared immutable globals from an RTL VLSU read capture.

    The global-parameter restriction is intentional: it prevents an RTL
    capture from preloading an address which an earlier DAG task is supposed
    to publish dynamically.  Keep bytes beyond the canonical ``*_bin`` image
    sparse until final materialization.  Dropping them here makes a valid
    type-5 global pointer resolve to an invented zero-filled hole merely
    because the linker placed that immutable payload after ``*_bin``.
    """
    result = bytearray(blob)
    global_ranges = [
        (entry["source"], entry["source"] + entry["length"])
        for entry in globals_
    ]
    observed, records, conflicts = collect_rtl_lsu_bytes(paths, global_ranges)
    applied = 0
    sparse = {}
    for byte_address, value in observed.items():
        if byte_address < len(result):
            result[byte_address] = value
        else:
            sparse[byte_address] = value
        applied += 1
    return bytes(result), sparse, applied, records, conflicts


def load_l2_backing_spec(path):
    """Load explicit sparse-L2 backing inputs for a conformance fixture.

    A backing spec is intentionally opt-in and self-documenting.  Its shape
    is::

        {"version": 1, "l2_backing_inputs": [
          {"address": "0x...", "length": "0x...",
           "provenance": "why this is an immutable input",
           "source": {"kind": "rtl-lsu", "logs": ["tile0.txt"]}}
        ]}

    ``kind: file`` is also supported with ``file`` and optional ``offset``.
    RTL-LSU data is accepted only for the declared address range and must be
    complete unless the caller explicitly sets ``allow_unknown``.  This keeps
    the mechanism distinct from a blanket replay of all LSU reads.
    """
    path = Path(path).resolve()
    raw = json.loads(path.read_text(encoding="utf-8"))
    entries = raw.get("l2_backing_inputs") if isinstance(raw, dict) else None
    if not isinstance(entries, list) or not entries:
        raise ValueError(
            f"{path} must contain a non-empty l2_backing_inputs array")

    def resolve_source_path(value, context):
        source_path = Path(value).expanduser()
        if not source_path.is_absolute():
            source_path = path.parent / source_path
        source_path = source_path.resolve()
        if not source_path.is_file():
            raise ValueError(f"{context} does not exist or is not a file: {source_path}")
        return source_path

    result = []
    for index, entry in enumerate(entries):
        context = f"{path}: l2_backing_inputs[{index}]"
        if not isinstance(entry, dict):
            raise ValueError(f"{context} is not an object")
        address = parse_integer(entry.get("address"), f"{context}.address")
        length = parse_integer(entry.get("length"), f"{context}.length")
        if address < 0 or length <= 0:
            raise ValueError(f"{context} has invalid address/length")
        provenance = entry.get("provenance")
        if not isinstance(provenance, str) or not provenance.strip():
            raise ValueError(f"{context} requires a non-empty provenance string")
        source = entry.get("source")
        if not isinstance(source, dict):
            raise ValueError(f"{context} requires a source object")
        kind = source.get("kind")
        if kind == "file":
            source_path = resolve_source_path(source.get("file"),
                                              f"{context}.source.file")
            offset = parse_integer(source.get("offset", 0),
                                   f"{context}.source.offset")
            if offset < 0:
                raise ValueError(f"{context}.source.offset is negative")
            data = source_path.read_bytes()
            payload = data[offset:offset + length]
            if len(payload) != length:
                raise ValueError(
                    f"{context} asks for {length} bytes at source offset "
                    f"0x{offset:x}, but {source_path} is too short")
            known_bytes, read_count, conflicts = length, 0, 0
            source_metadata = {
                "kind": "file", "file": str(source_path), "offset": offset,
            }
        elif kind == "rtl-lsu":
            logs = source.get("logs")
            if not isinstance(logs, list) or not logs:
                raise ValueError(f"{context}.source.logs must be a non-empty array")
            log_paths = [resolve_source_path(log, f"{context}.source.logs")
                         for log in logs]
            observed, read_count, conflicts = collect_rtl_lsu_bytes(
                log_paths, [(address, address + length)])
            payload_image = bytearray(length)
            for byte_address, value in observed.items():
                payload_image[byte_address - address] = value
            payload = bytes(payload_image)
            known_bytes = len(observed)
            if known_bytes != length and not entry.get("allow_unknown", False):
                raise ValueError(
                    f"{context} has only {known_bytes}/{length} known RTL-LSU "
                    "bytes; declare a complete range or set allow_unknown explicitly")
            source_metadata = {
                "kind": "rtl-lsu", "logs": [str(log) for log in log_paths],
                "read_records": read_count, "conflicting_repeats": conflicts,
            }
        else:
            raise ValueError(f"{context}.source.kind must be 'file' or 'rtl-lsu'")
        result.append({
            "address": address,
            "length": length,
            "payload": payload,
            "known_bytes": known_bytes,
            "provenance": provenance,
            "source": source_metadata,
        })
    return result


def build_transfer_plan(tasks, globals_, dmt_slots):
    """Build the symbolic L1 transfer contract directly from a runtime DAG.

    The task container deliberately names dynamic values by ``(task, port)``
    rather than baking a DMT address into every consumer descriptor.  Keep
    that distinction in the manifest: a type-0/4 transfer must be resolved
    only after the producer's return DMA has committed its DMT entry.  This
    function therefore never consults an RTL DMA dump and never assigns a
    tile, a physical address, or a runtime ``fid``.
    """
    slot_by_key = {
        (entry["task"], entry["port"]): entry for entry in dmt_slots
    }
    if len(slot_by_key) != len(dmt_slots):
        raise ValueError("runtime DAG DMT layout has duplicate task/port slots")

    def dmt_slot(producer, context):
        key = (producer["task"], producer["retid"])
        try:
            return slot_by_key[key]
        except KeyError as error:
            raise ValueError(
                f"{context} references missing DMT slot {key[0]}:{key[1]}") from error

    fire_plan = []
    for task in tasks:
        task_id = task["id"]
        ordinal = 0
        if task["code_length"]:
            fire_plan.append({
                "task": task_id,
                "issue_ordinal": ordinal,
                "kind": "code",
                "descriptor_index": None,
                "input_type": None,
                "source": {"space": "shared_l2",
                           "offset": task["code_address"]},
                "destination": {"space": "tile_local", "offset": 0},
                "length": {"mode": "fixed", "bytes": task["code_length"]},
                "payload": {"mode": "shared_l2"},
                "requires": [],
                "fid_policy": "runtime_dma_issue_counter",
            })
            ordinal += 1
        if task["data_length"]:
            fire_plan.append({
                "task": task_id,
                "issue_ordinal": ordinal,
                "kind": "data",
                "descriptor_index": None,
                "input_type": None,
                "source": {"space": "shared_l2",
                           "offset": task["data_address"]},
                "destination": {"space": "tile_local", "offset": 0x20000},
                "length": {"mode": "fixed", "bytes": task["data_length"]},
                "payload": {"mode": "shared_l2"},
                "requires": [],
                "fid_policy": "runtime_dma_issue_counter",
            })
            ordinal += 1

        for desc in task["inputs"]:
            input_type = desc["type"]
            entry = {
                "task": task_id,
                "issue_ordinal": ordinal,
                "kind": "input",
                "descriptor_index": desc["index"],
                "input_type": input_type,
                "destination": {
                    "space": "tile_local",
                    "offset": desc["destination"],
                },
                "fid_policy": "runtime_dma_issue_counter",
            }
            producer = {
                "task": desc["parent_task"],
                "retid": desc["output_port"],
            }
            if input_type == 0:
                slot = dmt_slot(
                    producer,
                    f"task {task_id} descriptor {desc['index']}")
                entry.update({
                    "source": {
                        "space": "dmt_return", "producer": producer,
                        "relative_offset": slot["relative_offset"],
                        "bytes": slot["capacity"],
                    },
                    # ``input_len`` in RTL's DMT is generated from this
                    # static allocation capacity.  A return may update only
                    # a prefix, but the consumer fire retains this width.
                    "length": {"mode": "fixed", "bytes": slot["capacity"]},
                    "payload": {"mode": "dmt_return", "producer": producer},
                    "requires": [{"kind": "dmt_ready", **producer}],
                })
            elif input_type == 4:
                slot = dmt_slot(
                    producer,
                    f"task {task_id} descriptor {desc['index']}")
                entry.update({
                    "source": {"space": "ptr_temp", "producer": producer},
                    "length": {"mode": "fixed", "bytes": 64},
                    "payload": {"mode": "packed_dmt_pointer",
                                "producer": producer,
                                "target": {
                                    "space": "dmt_return",
                                    "relative_offset": slot["relative_offset"],
                                    "bytes": slot["capacity"],
                                }},
                    "requires": [{"kind": "dmt_ready", **producer}],
                })
            elif input_type in (1, 2, 5, 6):
                global_index = (
                    (desc["parent_task"] << 4 | desc["output_port"]) - 1)
                if not 0 <= global_index < len(globals_):
                    raise ValueError(
                        f"task {task_id} descriptor {desc['index']} references "
                        f"global {global_index}")
                global_entry = globals_[global_index]
                global_source = {
                    "space": "shared_l2",
                    "offset": global_entry["source"],
                    # Pointer inputs carry this exact allocation capacity in
                    # their 64-byte record.  Preserve it in the symbolic
                    # plan so a timing model can construct the record from
                    # the runtime DAG rather than replaying captured bytes.
                    "bytes": global_entry["length"],
                    "global_index": global_index,
                }
                if input_type in (1, 2):
                    entry.update({
                        "source": global_source,
                        "length": {"mode": "fixed",
                                   "bytes": global_entry["length"]},
                        "payload": {"mode": "shared_l2"},
                        "requires": [],
                    })
                else:
                    entry.update({
                        "source": {"space": "ptr_global"
                                   if input_type == 5 else "ptr_dfe"},
                        "length": {"mode": "fixed", "bytes": 64},
                        "payload": {"mode": "packed_global_pointer",
                                    "target": global_source},
                        "requires": [],
                    })
            else:
                # Type 3 and type 7 take a state-machine path that does not
                # construct a DMA request in the current RTL.  A plan which
                # invented one would hide a real RTL/gem5 behavior gap.
                raise ValueError(
                    f"task {task_id} descriptor {desc['index']} has unsupported "
                    f"non-DMA input type {input_type}")
            fire_plan.append(entry)
            ordinal += 1

    return fire_plan


def build_return_plan(tasks):
    """Declare runtime return slots without fabricating their dynamic fields."""
    return [
        {
            "task": task["id"],
            "retid": retid,
            "kind": "runtime_return_slot",
            "source": {
                "space": "tile_runtime_output",
                "tile": "dispatch_assigned",
                "local_addr": "runtime",
            },
            "destination": {
                "space": "dmt_return",
                "task": task["id"],
                "retid": retid,
            },
            "length": {"mode": "runtime_return_size"},
            "presence": "runtime_return",
        }
        for task in tasks
        for retid in range(task["output_count"])
    ]


def serialize_rtl_dma_evidence(transactions):
    """Make a compact, non-behavioral evidence record for an RTL capture."""
    evidence = []
    for transaction in transactions:
        entry = {
            "direction": transaction["direction"],
            "task": transaction["task"],
            "tile": transaction["tile"],
            "source": transaction["source"],
            "destination": transaction["destination"],
            "length": transaction["length"],
            "time_ns": transaction["time_ns"],
            "known_bytes": transaction["known_bytes"],
        }
        if transaction["direction"] == "fire":
            entry["fid"] = transaction["fid"]
        else:
            entry["retid"] = transaction["retid"]
        if transaction["known_bytes"] == transaction["length"]:
            entry["payload_sha256"] = hashlib.sha256(
                transaction["payload"]).hexdigest()
        evidence.append(entry)
    return evidence


def build_task_image(task, blob, globals_, local_size=0x200000,
                     *, live_firmware_inputs=False):
    image = bytearray(local_size)
    initial_inputs = []
    materialized_globals = set()
    for source, length, destination, label in (
        (task["code_address"], task["code_length"], 0, "code"),
        (task["data_address"], task["data_length"], 0x20000, "data"),
    ):
        if source + length > len(blob) or destination + length > len(image):
            raise ValueError(f"task {task['id']} {label} range is outside its image")
        image[destination:destination + length] = blob[source:source + length]

    # Read-only tile-manager registers populated by RTL when a task is fired.
    # ``task_hardware_requirement_task_container_t`` is packed with the three
    # feature bits at the LSB, followed by num_lane[3:0] and spm_size[3:0].
    tile_manager = 0x1ff000
    struct.pack_into("<I", image, tile_manager + 0x18,
                     (task["hardware_requirement"] >> 3) & 0xf)
    struct.pack_into("<I", image, tile_manager + 0x20, 0)  # DAG slot 0
    struct.pack_into("<I", image, tile_manager + 0x24, task["id"])
    struct.pack_into("<I", image, tile_manager + 0x28, task["crc"])

    dependencies = []
    for desc in task["inputs"]:
        input_type = desc["type"]
        if input_type in (0, 4):
            dependencies.append({
                "task": task["id"], "parent": desc["parent_task"],
                "port": desc["output_port"], "destination": desc["destination"],
                "type": input_type, "descriptor_index": desc["index"],
            })
            continue
        if input_type not in (1, 2, 5, 6):
            continue
        global_index = (desc["parent_task"] << 4 | desc["output_port"]) - 1
        if not 0 <= global_index < len(globals_):
            raise ValueError(f"task {task['id']} references global {global_index}")
        entry = globals_[global_index]
        if live_firmware_inputs:
            # The CPU executes the exact L1 ELF and DMA supplies these bytes.
            # Registry ELFs are task code scaffolding, not an input oracle.
            continue
        if input_type in (1, 2):
            payload = blob[entry["source"]:entry["source"] + entry["length"]]
        else:
            # Type-5/type-6 descriptors carry a pointer to a payload which
            # remains resident in the L1-visible shared L2 image.  The RTL
            # only DMA-writes the packed pointer into the tile; a later LDU
            # follows that pointer and reads the original global payload.
            # Each SE task has a private address space, so materialize the
            # referenced shared bytes there as the functional equivalent of
            # the common RTL L2 backing store.
            source = entry["source"]
            end = source + entry["length"]
            if end > len(blob) or end > len(image):
                raise ValueError(
                    f"task {task['id']} global pointer payload is outside "
                    "its replay image")
            if (source, end) not in materialized_globals:
                image[source:end] = blob[source:end]
                materialized_globals.add((source, end))
            packed = entry["length"] | (entry["source"] << 16)
            payload = packed.to_bytes(64, "little")
        destination = desc["destination"]
        # Every non-dependency descriptor is materialized by an RTL L1 DMA,
        # regardless of whether its destination is in DSPM or VSPM.  Keep
        # the low-address ELF overlay for legacy replay mode, but also expose
        # the transfer explicitly so a physical, persistent tile DSPM sees it.
        initial_inputs.append({
            "destination": destination,
            "payload": payload,
            "type": input_type,
            "descriptor_index": desc["index"],
            "source": ({"space": "shared_l2", "offset": entry["source"]}
                       if input_type in (1, 2)
                       else {"space": "ptr_global"
                             if input_type == 5 else "ptr_dfe"}),
        })
        if destination & 0x100000:
            continue
        if destination + len(payload) > len(image):
            raise ValueError(f"task {task['id']} input destination is outside local memory")
        image[destination:destination + len(payload)] = payload
    return image, dependencies, initial_inputs


def write_replay_elf(image, output_dir, stem, entry=4):
    output_dir.mkdir(parents=True, exist_ok=True)
    low = output_dir / f"{stem}.bin"
    high = output_dir / f"{stem}.high.bin"
    low.write_bytes(image)
    high.write_bytes(image)
    low_obj = output_dir / f"{stem}.o"
    high_obj = output_dir / f"{stem}.high.o"
    script = output_dir / f"{stem}.ld"
    elf = output_dir / f"{stem}.elf"
    script.write_text(
        "SECTIONS\n{\n"
        f"  . = 0x0; .local : {{ {low_obj.name}(.data) }}\n"
        f"  . = 0x80000000; .venus : {{ {high_obj.name}(.data) }}\n"
        "}\n", encoding="utf-8")
    objcopy = find_tool("riscv64-linux-gnu-objcopy")
    linker = find_tool("riscv64-linux-gnu-ld")
    for source, target in ((low, low_obj), (high, high_obj)):
        subprocess.run([objcopy, "-I", "binary", "-O", "elf32-littleriscv",
                        "-B", "riscv:rv32", str(source), str(target)], check=True)
    subprocess.run([linker, "-m", "elf32lriscv", "-T", script.name, "-e", hex(entry),
                    "-o", elf.name, low_obj.name, high_obj.name],
                   cwd=output_dir, check=True)
    return elf.resolve()


def materialize_shared_l2(runtime):
    """Build the dense GEM5 image from DAG bytes, globals, and fixtures.

    Value and pointer fire descriptors refer to globals in shared L2. A
    pointer DMA carries a record, not the bytes of its backing allocation.
    Some scheduler ELFs keep those payloads outside
    the ``*_bin`` symbol, so an RTL DMA capture can be the only byte-complete
    source available to the materializer.  Put those declared globals at
    their architectural offsets before adding optional fixtures.  Never let
    an absent payload silently become zero-filled DMA data.

    GEM5's sequencer consumes one binary image today.  Keep the input model
    sparse in the manifest, but extend the generated image only as far as the
    highest declared byte.  A global or backing may not silently overwrite
    canonical DAG bytes or another input with different bytes.
    """
    base = runtime["blob"]
    image = bytearray(base)
    previous = []

    for task in runtime["tasks"]:
        for desc in task["inputs"]:
            if desc["type"] not in (1, 2, 5, 6):
                continue
            global_index = (
                (desc["parent_task"] << 4 | desc["output_port"]) - 1
            )
            if not 0 <= global_index < len(runtime["globals"]):
                raise ValueError(
                    f"task {task['id']} references global {global_index}")
            global_entry = runtime["globals"][global_index]
            start = global_entry["source"]
            length = global_entry["length"]
            end = start + length
            if start < 0 or not 0 < length <= 0xffff:
                raise ValueError(f"task {task['id']} global {global_index} has invalid range")

            matches = [
                record for record in runtime.get("dma_records", [])
                if desc["type"] in (1, 2)
                and record["task"] == task["id"]
                and record["destination"] == desc["destination"]
                and len(record["payload"]) == length
            ]
            unique_payloads = {bytes(record["payload"]) for record in matches}
            if len(unique_payloads) > 1:
                raise ValueError(
                    f"task {task['id']} global {global_index} has conflicting "
                    "RTL DMA payloads")
            backing_payloads = {
                bytes(backing["payload"][
                    start - backing["address"]:
                    end - backing["address"]
                ])
                for backing in runtime.get("l2_backing_inputs", [])
                if backing["address"] <= start
                and end <= (
                    backing["address"] + len(backing["payload"])
                )
            }
            if len(backing_payloads) > 1:
                raise ValueError(
                    f"task {task['id']} global {global_index} has "
                    "conflicting byte-complete L2 backing payloads")
            payload_from_backing = False
            if unique_payloads:
                payload = unique_payloads.pop()
            elif backing_payloads:
                payload = backing_payloads.pop()
                payload_from_backing = True
            elif end <= len(base):
                payload = base[start:end]
            else:
                raise ValueError(
                    f"task {task['id']} global {global_index} "
                    f"[0x{start:x}, 0x{end:x}) is outside the canonical "
                    "DAG blob and has neither a byte-complete RTL DMA "
                    "payload nor an explicit byte-complete L2 backing")

            if end > 32 * 1024 * 1024:
                raise ValueError(
                    f"global {global_index} [0x{start:x}, 0x{end:x}) exceeds "
                    "the 32 MiB GEM5 shared-L2 model")
            base_overlap_end = min(end, len(base))
            if start < base_overlap_end:
                base_slice = base[start:base_overlap_end]
                payload_slice = payload[:base_overlap_end - start]
                if payload_slice != base_slice:
                    if desc["type"] == 5:
                        raise ValueError("static pointer backing overwrites canonical DAG bytes")
                    for owner in runtime["tasks"]:
                        for region in ("code", "data"):
                            low = owner.get(f"{region}_address", 0)
                            high = low + owner.get(f"{region}_length", 0)
                            if max(low, start) < min(high, end):
                                raise ValueError(
                                    f"global {global_index} overwrites task {owner['id']} {region}")
                if payload_slice != base_slice and not payload_from_backing:
                    raise ValueError(
                        f"global {global_index} overwrites canonical DAG bytes "
                        f"[0x{start:x}, 0x{base_overlap_end:x})")
            for previous_index, previous_start, previous_payload in previous:
                previous_end = previous_start + len(previous_payload)
                overlap_start = max(start, previous_start)
                overlap_end = min(end, previous_end)
                if overlap_start >= overlap_end:
                    continue
                lhs = payload[overlap_start - start:overlap_end - start]
                rhs = previous_payload[
                    overlap_start - previous_start:
                    overlap_end - previous_start
                ]
                if lhs != rhs:
                    raise ValueError(
                        f"global {global_index} conflicts with global/backing "
                        f"{previous_index} at "
                        f"[0x{overlap_start:x}, 0x{overlap_end:x})")
            if len(image) < end:
                image.extend(b"\0" * (end - len(image)))
            image[start:end] = payload
            previous.append((f"global-{global_index}", start, payload))

    # RTL-LSU recovery is restricted by ``overlay_rtl_lsu_globals`` to the
    # architectural ranges of declared immutable globals.  Coalesce its
    # sparse post-``*_bin`` bytes into auditable runs, preserve gaps as
    # unknown/zero, and reject conflicts with a byte-complete type-1/type-2
    # payload recovered above.  This is input materialization only; it never
    # changes a request, response, or completion time.
    sparse_items = sorted(runtime.get("rtl_lsu_sparse_bytes", {}).items())
    sparse_runs = []
    for address, value in sparse_items:
        if not sparse_runs or address != sparse_runs[-1][0] + len(
                sparse_runs[-1][1]):
            sparse_runs.append((address, bytearray()))
        sparse_runs[-1][1].append(value)
    for run_index, (start, run_payload) in enumerate(sparse_runs):
        payload = bytes(run_payload)
        end = start + len(payload)
        if end > 32 * 1024 * 1024:
            raise ValueError(
                f"RTL-LSU global run {run_index} "
                f"[0x{start:x}, 0x{end:x}) exceeds the 32 MiB GEM5 "
                "shared-L2 model")
        for previous_index, previous_start, previous_payload in previous:
            previous_end = previous_start + len(previous_payload)
            overlap_start = max(start, previous_start)
            overlap_end = min(end, previous_end)
            if overlap_start >= overlap_end:
                continue
            lhs = payload[overlap_start - start:overlap_end - start]
            rhs = previous_payload[
                overlap_start - previous_start:
                overlap_end - previous_start
            ]
            if lhs != rhs:
                raise ValueError(
                    f"RTL-LSU global run {run_index} conflicts with "
                    f"{previous_index} at "
                    f"[0x{overlap_start:x}, 0x{overlap_end:x})")
        if len(image) < end:
            image.extend(b"\0" * (end - len(image)))
        image[start:end] = payload
        previous.append((f"rtl-lsu-global-{run_index}", start, payload))

    for index, backing in enumerate(runtime.get("l2_backing_inputs", [])):
        start = backing["address"]
        payload = backing["payload"]
        end = start + len(payload)
        if end > 32 * 1024 * 1024:
            raise ValueError(
                f"L2 backing {index} [0x{start:x}, 0x{end:x}) exceeds the "
                "32 MiB GEM5 shared-L2 model")
        base_overlap_end = min(end, len(base))
        if start < base_overlap_end:
            base_slice = base[start:base_overlap_end]
            fixture_slice = payload[:base_overlap_end - start]
            if fixture_slice != base_slice:
                declared_global_match = any(
                    str(previous_index).startswith("global-")
                    and previous_start <= start
                    and end <= previous_start + len(previous_payload)
                    and payload == previous_payload[
                        start - previous_start:end - previous_start
                    ]
                    for (previous_index, previous_start,
                         previous_payload) in previous
                )
                if not declared_global_match:
                    raise ValueError(
                        f"L2 backing {index} overwrites canonical DAG bytes "
                        f"[0x{start:x}, 0x{base_overlap_end:x}) without an "
                        "identical declared immutable global")
        for previous_index, previous_start, previous_payload in previous:
            previous_end = previous_start + len(previous_payload)
            overlap_start = max(start, previous_start)
            overlap_end = min(end, previous_end)
            if overlap_start >= overlap_end:
                continue
            lhs = payload[overlap_start - start:overlap_end - start]
            rhs = previous_payload[
                overlap_start - previous_start:overlap_end - previous_start]
            if lhs != rhs:
                raise ValueError(
                    f"L2 backing {index} conflicts with input {previous_index} "
                    f"at [0x{overlap_start:x}, 0x{overlap_end:x})")
        if len(image) < end:
            image.extend(b"\0" * (end - len(image)))
        image[start:end] = payload
        previous.append((index, start, payload))
    return bytes(image)


def materialize(runtime, output_dir, *, live_firmware_inputs=False):
    if live_firmware_inputs and any(runtime.get(key) for key in (
            "dma_records", "dma_transactions", "l2_backing_inputs", "rtl_lsu_sparse_bytes")):
        raise ValueError("live firmware inputs cannot use capture/backing overlays")
    output_dir.mkdir(parents=True, exist_ok=True)
    manifest_tasks, dependencies, initial_inputs = [], [], []
    # v5 describes the scheduler's logical transaction contract entirely
    # from the runtime DAG.  It is intentionally constructed before any
    # optional RTL capture is consulted: observed DMA addresses/times remain
    # evidence, not a source of behavior for the GEM5 scheduler.
    dmt_layout = runtime["dmt_layout"]
    dmt_slot_by_key = {
        (entry["task"], entry["port"]): entry
        for entry in dmt_layout["slots"]
    }
    fire_plan = build_transfer_plan(
        runtime["tasks"], runtime["globals"], dmt_layout["slots"])
    return_plan = build_return_plan(runtime["tasks"])
    shared_l2 = output_dir / "shared_l2.bin"
    l2_backing_manifest = []
    for index, backing in enumerate(runtime.get("l2_backing_inputs", [])):
        backing_path = output_dir / f"l2_backing_{index:02d}.bin"
        backing_path.write_bytes(backing["payload"])
        l2_backing_manifest.append({
            "address": backing["address"],
            "length": backing["length"],
            "file": str(backing_path.resolve()),
            "sha256": hashlib.sha256(backing["payload"]).hexdigest(),
            "known_bytes": backing["known_bytes"],
            "provenance": backing["provenance"],
            "source": backing["source"],
        })
    dense_shared_l2 = (runtime["blob"] if live_firmware_inputs
                       else materialize_shared_l2(runtime))
    shared_l2.write_bytes(dense_shared_l2)
    for task in runtime["tasks"]:
        image, task_dependencies, task_initial = build_task_image(
            task, dense_shared_l2, runtime["globals"],
            live_firmware_inputs=live_firmware_inputs)
        task_dir = output_dir / "replay" / f"task_{task['id']:02d}"
        elf = write_replay_elf(image, task_dir, task["name"])
        code_path = task_dir / "code.bin"
        data_path = task_dir / "data.bin"
        code_path.write_bytes(runtime["blob"][
            task["code_address"]:task["code_address"] + task["code_length"]])
        data_path.write_bytes(runtime["blob"][
            task["data_address"]:task["data_address"] + task["data_length"]])
        # A task's hardware requirements are immutable DAG metadata.  Its
        # physical tile is intentionally absent: the runtime scheduler picks
        # that assignment, and an optional DMA capture must not backfill it.
        manifest_tasks.append({
            "id": task["id"], "name": task["name"], "elf": str(elf),
            "code_file": str(code_path.resolve()),
            "data_file": str(data_path.resolve()),
            "code_source": task["code_address"],
            "data_source": task["data_address"],
            "crc": task["crc"], "output_count": task["output_count"],
            "hardware_requirement": task["hardware_requirement"],
            "need_spmd": task["need_spmd"],
            "minimum_spmd_tasks": task["minimum_spmd_tasks"],
        })
        dependencies.extend(task_dependencies)
        for dependency in task_dependencies:
            # The DMT address and width come from the static runtime DAG
            # layout, not from an observed DMA transfer.  The type-0 fire
            # width is exactly the DMT's fixed ``input_len``; type-4 itself
            # transfers a 64-byte pointer which names this same slot.
            key = (dependency["parent"], dependency["port"])
            try:
                slot = dmt_slot_by_key[key]
            except KeyError as error:
                raise ValueError(
                    f"task {task['id']} input references missing DMT slot "
                    f"{key[0]}:{key[1]}") from error
            dependency["slot_relative_offset"] = slot["relative_offset"]
            dependency["slot_address"] = (
                dmt_layout["malloc_base"] + slot["relative_offset"])
            dependency["slot_address_valid"] = True
            dependency["slot_consumer_bytes"] = slot["capacity"]
            dependency["length"] = (
                slot["capacity"] if dependency["type"] == 0 else 64)
        for index, entry in enumerate(task_initial):
            matches = [
                record for record in runtime.get("dma_records", [])
                if record["task"] == task["id"]
                and record["destination"] == entry["destination"]
            ]
            payload = (matches[0]["payload"] if len(matches) == 1
                       else entry["payload"])
            payload_path = task_dir / f"initial_input_{index:02d}.bin"
            payload_path.write_bytes(payload)
            initial_inputs.append({
                "task": task["id"], "destination": entry["destination"],
                "file": str(payload_path.resolve()),
                "type": entry["type"],
                "descriptor_index": entry["descriptor_index"],
                "source": entry["source"],
            })
    manifest = {
        "version": 5,
        "input_materialization": ("live-firmware-dma" if live_firmware_inputs
                                  else "contract-image"),
        "mode": "rtl-l1-runtime-dag",
        "address_profile": "venus_l2_tile_v1",
        "rtl_symbol_prefix": runtime["prefix"],
        "tasks": manifest_tasks,
        "dependency_inputs": dependencies,
        "initial_inputs": initial_inputs,
        # This is static DAG metadata, not a decoded return-DMA observation:
        # the overall RTL testbench writes ``binandjsonsize`` to L2_malloc,
        # and task JSON's ``temp_offset`` fields seed the DMT entries.
        "dmt_layout": {
            "malloc_base": dmt_layout["malloc_base"],
            "slots": dmt_layout["slots"],
            "source_json": dmt_layout["json"],
            "source_json_sha256": dmt_layout["json_sha256"],
        },
        # These plans are semantic.  In particular, a DMT address/length,
        # assigned tile, physical address, and runtime fid/retid are *not*
        # static DAG fields and must be resolved by the scheduler state
        # machine at issue/complete time.
        "fire_plan": fire_plan,
        "return_plan": return_plan,
        "scheduler_firmware": runtime["scheduler_firmware"],
        # These are immutable, opt-in time-zero fixtures.  VenusSequencer
        # consumes their already-materialized bytes via shared_l2_image; the
        # entries remain in the manifest so a result can be audited back to
        # its capture or source file rather than looking like a normal DAG
        # producer publication.
        "l2_backing_inputs": l2_backing_manifest,
        "shared_l2_image": str(shared_l2.resolve()),
        "trace_file": str((output_dir / "venus_dag_trace.jsonl").resolve()),
        "output_dump_dir": str(output_dir.resolve()),
    }
    if runtime.get("dma_transactions"):
        # An explicit RTL dump is useful for a first-divergence comparator,
        # but is never consumed as the GEM5 timing model's schedule.
        manifest["rtl_observed_dma"] = serialize_rtl_dma_evidence(
            runtime["dma_transactions"])
    if runtime.get("rtl_lsu_evidence"):
        manifest["rtl_observed_lsu"] = runtime["rtl_lsu_evidence"]
    path = (output_dir / "venus_dag_manifest.json").resolve()
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("l1_elf", type=Path)
    parser.add_argument("--prefix")
    parser.add_argument('--all-dags', action='store_true',
                        help='Materialize an ELF-qualified registry for firmware execution')
    parser.add_argument(
        "--task-input-type-bits", type=int, choices=(2, 3), default=3,
        help="physical task-input descriptor type width used by Scheduler",
    )
    parser.add_argument(
        "--task-container-without-spmd-fields",
        action="store_true",
        help=("decode the pre-SPMD task-container layout used by Venus1.0; "
              "input_num immediately follows hardware_requirement"),
    )
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument(
        "--rtl-dma-log", type=Path,
        help="explicit post-L1 RTL DMA capture used to materialize validation inputs")
    parser.add_argument(
        "--rtl-lsu-log", type=Path, action="append",
        help=("RTL VLSU read capture used to recover L1-generated DAG globals; "
              "repeat once per tile capture"))
    parser.add_argument(
        "--l2-backing-spec", type=Path,
        help=("explicit JSON whitelist of immutable sparse-L2 backing inputs; "
              "never inferred from all RTL LSU reads"))
    args = parser.parse_args()
    if args.all_dags:
        if not args.output_dir or args.rtl_dma_log or args.rtl_lsu_log or args.l2_backing_spec:
            parser.error('--all-dags requires --output-dir and no capture/backing overlays')
        from venus_firmware_registry import materialize_registry
        path = materialize_registry(args.l1_elf, args.prefix, args.output_dir,
                                    args.task_input_type_bits,
                                    not args.task_container_without_spmd_fields)
        print(f'manifest: {path}')
        return
    runtime = decode_runtime_dag(
        args.l1_elf, args.prefix, args.task_input_type_bits,
        not args.task_container_without_spmd_fields,
    )
    if args.rtl_dma_log:
        runtime["dma_transactions"] = parse_rtl_dma_transactions(
            args.rtl_dma_log)
        runtime["blob"], applied, runtime["dma_records"] = overlay_rtl_dma_inputs(
            runtime["blob"], args.rtl_dma_log, runtime["tasks"])
        print(f"overlaid {applied} post-L1 bytes from {args.rtl_dma_log}")
    if args.rtl_lsu_log:
        (runtime["blob"], runtime["rtl_lsu_sparse_bytes"], applied, records,
         conflicts) = overlay_rtl_lsu_globals(
            runtime["blob"], args.rtl_lsu_log, runtime["globals"])
        runtime["rtl_lsu_evidence"] = {
            "logs": [str(path.resolve()) for path in args.rtl_lsu_log],
            "read_records": records,
            "known_global_bytes": applied,
            "sparse_post_blob_bytes": len(runtime["rtl_lsu_sparse_bytes"]),
            "conflicting_repeats": conflicts,
        }
        print(
            f"overlaid {applied} global bytes from {records} RTL LSU reads "
            f"({conflicts} conflicting repeated bytes) across "
            f"{len(args.rtl_lsu_log)} RTL LSU log(s)")
    if args.l2_backing_spec:
        runtime["l2_backing_inputs"] = load_l2_backing_spec(
            args.l2_backing_spec)
        total = sum(entry["length"] for entry in runtime["l2_backing_inputs"])
        print(
            f"loaded {len(runtime['l2_backing_inputs'])} explicit sparse-L2 "
            f"backing input(s), {total} bytes, from {args.l2_backing_spec}")
    summary = {key: value for key, value in runtime.items()
               if key not in ("blob", "dma_records", "dma_transactions",
                              "l2_backing_inputs", "rtl_lsu_sparse_bytes")}
    print(json.dumps(summary, indent=2))
    if args.output_dir:
        print(f"manifest: {materialize(runtime, args.output_dir)}")


if __name__ == "__main__":
    main()
