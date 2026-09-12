#!/usr/bin/env python3
"""Inspect Venus RTL DAG descriptors and materialize replayable task ELFs.

The post-synthesis descriptor encodes a dependency as a ten-bit value.  The
upper bits select the parent task and the low nibble selects one of its output
ports.  A zero value is ambiguous with task-0/output-0, so the input ``type``
field is used to distinguish external inputs from dependency inputs.
"""

import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys
import time
import warnings
from pathlib import Path


def load_tasks(path):
    with path.open("r", encoding="utf-8") as stream:
        raw = json.load(stream)
    tasks = [entry for entry in raw if isinstance(entry, dict) and "current_taskId" in entry]
    tasks.sort(key=lambda entry: entry["current_taskId"])
    ids = [entry["current_taskId"] for entry in tasks]
    if ids != list(range(len(tasks))):
        raise ValueError(f"task IDs are not contiguous: {ids}")
    return tasks


def parse_parent(input_desc):
    """Return (task_id, output_port), or None for an external/static input."""
    encoded = int(input_desc.get("parentTasksPort", "0"), 0)
    # The DSL encodes ownership in the low two bits and pass mode in bit 2:
    # 0/4 are producer dependencies, 1/5 are static external inputs, and 2/6
    # are dynamic DAG inputs.  Do not use bit 0 alone: it is clear for both
    # producer dependencies and dynamic DAG inputs.
    input_type = int(input_desc.get("type", "0"), 0)
    if input_type in (1, 2, 5, 6):
        return None
    if input_type not in (0, 4):
        raise ValueError(f"unsupported DAG input type {input_type}")
    return encoded >> 4, encoded & 0xF


def dependency_graph(tasks):
    deps = {task["current_taskId"]: set() for task in tasks}
    for task in tasks:
        task_id = task["current_taskId"]
        for input_desc in task.get("all_input", []):
            parent = parse_parent(input_desc)
            if parent is None:
                continue
            parent_id, output_port = parent
            if parent_id >= task_id:
                raise ValueError(
                    f"task {task_id} input {input_desc.get('name')} has invalid "
                    f"parent task {parent_id}"
                )
            outputs = tasks[parent_id].get("all_output", [])
            if output_port >= len(outputs):
                raise ValueError(
                    f"task {task_id} input {input_desc.get('name')} references "
                    f"missing task {parent_id} output {output_port}"
                )
            deps[task_id].add(parent_id)
    return deps


def dependency_input_length(tasks, input_desc, parent):
    """Resolve a dependency DMA length from the consumer or producer ABI."""
    if input_desc.get("consumer_capacity_bytes") is not None:
        capacity = input_desc["consumer_capacity_bytes"]
        if type(capacity) is int and capacity > 0:
            hint = input_desc.get("length", capacity)
            if type(hint) is int and 0 < hint <= capacity:
                return hint
            warnings.warn(f"ignoring invalid advisory consumer length: {input_desc.get('name')}")
            return capacity
        warnings.warn(f"ignoring invalid advisory consumer capacity: {input_desc.get('name')}")
    if input_desc.get("length") is not None:
        return int(input_desc["length"])
    parent_id, output_port = parent
    output_desc = tasks[parent_id].get("all_output", [])[output_port]
    if output_desc.get("length") is None:
        raise ValueError(
            f"dependency input {input_desc.get('name')} and parent task "
            f"{parent_id} port {output_port} both omit length")
    return int(output_desc["length"])


def build_legacy_dmt_slots(tasks, base=0x01000000, alignment=64):
    """Allocate deterministic shared-L2 slots for legacy in-process DAGs."""
    capacities = {}
    for task in tasks:
        for input_desc in task.get("all_input", []):
            parent = parse_parent(input_desc)
            if parent is None:
                continue
            parent_output = tasks[parent[0]].get("all_output", [])[parent[1]]
            capacity = max(
                dependency_input_length(tasks, input_desc, parent),
                int(parent_output.get("length", 0)),
            )
            capacities[parent] = max(capacities.get(parent, 0), capacity)

    slots = {}
    cursor = base
    for key in sorted(capacities):
        capacity = capacities[key]
        cursor = (cursor + alignment - 1) & -alignment
        slots[key] = {"address": cursor, "capacity": capacity}
        cursor += (capacity + alignment - 1) & -alignment
    return slots


def packed_dmt_pointer(length, address):
    """Pack the declared DMT length, not the backing allocation capacity."""
    if not 0 < length <= 0xffff:
        raise ValueError(f"DMT pointer length {length} does not fit uint16")
    if not 0 <= address <= 0xffffffff:
        raise ValueError(f"DMT pointer address {address:#x} does not fit uint32")
    payload = bytearray(64)
    payload[0:2] = length.to_bytes(2, "little")
    payload[2:6] = address.to_bytes(4, "little")
    return bytes(payload)


def static_allocation_capacities(tasks, combined_size):
    """Infer backing spans for bounds checks, never DMT record lengths.

    These spans end at the next declared input or image end. They are not a
    physical SRAM capacity certificate or a proof of task access bounds.
    """
    offsets = sorted({
        int(input_desc["offset"])
        for candidate in tasks
        for input_desc in candidate.get("all_input", [])
        if parse_parent(input_desc) is None and input_desc.get("offset") is not None
    })
    for offset in offsets:
        if not 0 <= offset < combined_size:
            raise ValueError(
                f"static input offset {offset} outside combined image "
                f"of {combined_size} bytes")
    capacities = {}
    for index, offset in enumerate(offsets):
        end = offsets[index + 1] if index + 1 < len(offsets) else combined_size
        if end > offset:
            capacities[offset] = end - offset
    return capacities


def static_pointer_payload(input_desc, capacities):
    """Match Scheduler's global/DFE DMT entry: offset plus JSON length.

    A type-5/6 pointer transports one 64-byte record. The length in that
    record is explicitly declared by the compiler; neither the transport
    size nor the distance to the next allocation may replace it.
    """
    name = input_desc.get("name", "<unnamed>")
    fields = {}
    for key in ("offset", "length"):
        value = input_desc.get(key)
        if (isinstance(value, bool) or not isinstance(value, (int, str))):
            raise ValueError(f"static pointer {name}: explicit integer {key} required")
        try:
            fields[key] = int(value)
        except ValueError as error:
            raise ValueError(
                f"static pointer {name}: invalid integer {key} {value!r}") from error
    offset, length = fields["offset"], fields["length"]
    payload = packed_dmt_pointer(length, offset)
    capacity = capacities.get(offset, 0)
    if capacity <= 0 or length > capacity:
        raise ValueError(
            f"static pointer {name}: declared length {length} exceeds "
            f"backing span {capacity} at offset {offset}")
    return payload


def topological_levels(tasks, deps):
    remaining = {task_id: set(parents) for task_id, parents in deps.items()}
    levels = []
    completed = set()
    while remaining:
        ready = sorted(task_id for task_id, parents in remaining.items() if parents <= completed)
        if not ready:
            raise ValueError("dependency cycle detected")
        levels.append(ready)
        completed.update(ready)
        for task_id in ready:
            del remaining[task_id]
    return levels


def print_summary(tasks):
    deps = dependency_graph(tasks)
    levels = topological_levels(tasks, deps)
    print(f"tasks: {len(tasks)}")
    print("topological levels: " + " | ".join(
        ",".join(str(task_id) for task_id in level) for level in levels
    ))
    for task in tasks:
        task_id = task["current_taskId"]
        parents = ",".join(str(parent) for parent in sorted(deps[task_id])) or "-"
        print(
            f"{task_id:2d} {task['debug_task_name']:<30} "
            f"parents={parents:<12} text={task['text_length']:6d} "
            f"data={task['data_length']:6d}"
        )


def decode_input_data(input_desc):
    encoded = input_desc.get("data")
    if not encoded:
        raise ValueError(f"input {input_desc.get('name')} has no replay data")
    if not encoded.startswith("0x"):
        raise ValueError(f"input {input_desc.get('name')} data is not hexadecimal")
    payload = bytes.fromhex(encoded[2:])
    slice_length = int(input_desc.get("slice_length", 0))
    expected = slice_length if slice_length > 0 else int(input_desc["length"])
    if len(payload) != expected:
        raise ValueError(
            f"input {input_desc.get('name')} has {len(payload)} bytes, expected {expected}"
        )
    # The descriptor serializes a DMA payload as one big-endian hexadecimal
    # integer: the lowest-address byte is at the far right.  Convert it back
    # to byte-address order before placing it in tile-local memory.
    return payload[::-1]


def find_tool(name):
    path = shutil.which(name)
    if not path:
        raise RuntimeError(f"required tool not found: {name}")
    return path


def task_binary_path(case_dir, task_name):
    """Locate a descriptor task image, accepting replicated-instance names."""
    task_bin = case_dir / f"{task_name}.bin"
    if task_bin.is_file():
        return task_bin
    # Post-synthesis DAG descriptors disambiguate replicated instances with
    # names such as ``Task_foo*7``.  The case directory retains one compiled
    # image per implementation (``Task_foo.bin``), because the
    # instance-specific state is supplied by the descriptor inputs.  Only use
    # this as a fallback so descriptors that do ship per-instance binaries
    # keep their exact-name behavior.
    canonical_name = task_name.split("*", 1)[0]
    canonical_bin = case_dir / f"{canonical_name}.bin"
    if canonical_name != task_name and canonical_bin.is_file():
        return canonical_bin
    raise FileNotFoundError(task_bin)


def materialize_task(
    tasks, case_dir, task_id, output_dir, external_only=False,
    entry_base=0x4, local_memory_size=0x200000,
    dependency_dump_dir=None,
):
    task = tasks[task_id]
    task_name = task["debug_task_name"]
    task_bin = task_binary_path(case_dir, task_name)
    raw_image = task_bin.read_bytes()
    image = bytearray(raw_image)
    applied = []
    for input_desc in task.get("all_input", []):
        parent = parse_parent(input_desc)
        if external_only and parent is not None:
            continue
        if parent is not None and dependency_dump_dir is not None:
            parent_id, parent_port = parent
            dump_path = dependency_dump_dir / (
                f"task_{parent_id}_port_{parent_port}.bin")
            if not dump_path.is_file():
                raise FileNotFoundError(dump_path)
            expected = dependency_input_length(tasks, input_desc, parent)
            payload = dump_path.read_bytes()
            # Dynamic producer returns may be shorter than the consumer's
            # statically allocated DMT slot.  The in-process scheduler copies
            # the valid prefix into a zero-initialized slot; mirror that ABI
            # here instead of reading beyond the captured producer payload.
            payload = payload[:expected].ljust(expected, b"\0")
        else:
            payload = decode_input_data(input_desc)
        address = int(input_desc["dest_address"], 0)
        end = address + len(payload)
        if end > len(image):
            image.extend(bytes(end - len(image)))
        image[address:end] = payload
        applied.append((input_desc["name"], address, len(payload)))

    # gem5 SE cannot begin execution with an entry value of exactly zero. The
    # first RTL instruction only clears x1, whose SE initial value is already
    # zero, so entry 0x4 preserves the original task address space, including
    # absolute function pointers and jump tables. A larger entry value remains
    # available as a diagnostic relocation mode.
    linked_size = int(task["total_length"])
    if entry_base >= linked_size:
        relocated_end = entry_base + linked_size
        if relocated_end > len(image):
            image.extend(bytes(relocated_end - len(image)))
        image[entry_base:relocated_end] = raw_image[:linked_size]
    if len(image) < local_memory_size:
        image.extend(bytes(local_memory_size - len(image)))

    output_dir.mkdir(parents=True, exist_ok=True)
    flat_path = output_dir / f"{task_name}.replay.bin"
    high_flat_path = output_dir / f"{task_name}.replay.high.bin"
    object_path = output_dir / f"{task_name}.replay.o"
    high_object_path = output_dir / f"{task_name}.replay.high.o"
    linker_script_path = output_dir / f"{task_name}.replay.ld"
    elf_path = output_dir / f"{task_name}.replay.elf"
    flat_path.write_bytes(image)
    high_flat_path.write_bytes(image)
    linker_script_path.write_text(
        "SECTIONS\n"
        "{\n"
        f"  . = 0x0; .local : {{ {object_path.name}(.data) }}\n"
        f"  . = 0x80000000; .venus : {{ {high_object_path.name}(.data) }}\n"
        "}\n",
        encoding="utf-8",
    )

    objcopy = find_tool("riscv64-linux-gnu-objcopy")
    linker = find_tool("riscv64-linux-gnu-ld")
    subprocess.run([
        objcopy, "-I", "binary", "-O", "elf32-littleriscv", "-B", "riscv:rv32",
        str(flat_path), str(object_path),
    ], check=True)
    subprocess.run([
        objcopy, "-I", "binary", "-O", "elf32-littleriscv", "-B", "riscv:rv32",
        str(high_flat_path), str(high_object_path),
    ], check=True)
    subprocess.run([
        linker, "-m", "elf32lriscv", "-T", linker_script_path.name,
        "-e", hex(entry_base), "-o", elf_path.name,
        object_path.name, high_object_path.name,
    ], check=True, cwd=output_dir)

    print(f"task: {task_id} {task_name}")
    print(f"entry: 0x{entry_base:08x}")
    print(f"image bytes: {len(image)}")
    print(f"inputs applied: {len(applied)}")
    for name, address, length in applied:
        print(f"  {name:<32} 0x{address:08x} {length} bytes")
    print(f"ELF: {elf_path}")
    return elf_path


def materialize_scheduler_images(task, case_dir, task_dir):
    """Write the L1 code/data DMA payloads for a static descriptor task."""
    task_dir.mkdir(parents=True, exist_ok=True)
    raw_image = task_binary_path(case_dir, task["debug_task_name"]).read_bytes()
    text_length = int(task["text_length"])
    data_length = int(task["data_length"])
    # Static case .bin files are sparse flat local-memory images: code begins
    # at zero and the initialized DSPM section begins at 0x20000.  Some old
    # descriptors declare zero-filled tail bytes after their compact code
    # image, hence explicit padding instead of requiring an exact file size.
    code = raw_image[:text_length].ljust(text_length, b"\0")
    data_offset = 0x20000
    data = raw_image[data_offset:data_offset + data_length].ljust(
        data_length, b"\0")
    code_path = task_dir / "code.bin"
    data_path = task_dir / "data.bin"
    code_path.write_bytes(code)
    data_path.write_bytes(data)
    return code_path.resolve(), data_path.resolve()


def read_sim_stat(stats_path, name):
    if not stats_path.is_file():
        return None
    for line in stats_path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0] == name:
            try:
                return int(fields[1])
            except ValueError:
                return fields[1]
    return None


def run_replay_task(task, tasks, case_dir, run_dir, gem5, config,
                    venus_config, sim_mode, timeout_seconds,
                    dependency_dump_dir):
    task_id = task["current_taskId"]
    task_name = task["debug_task_name"]
    task_dir = (run_dir / f"task_{task_id:02d}_{task_name}").resolve()
    replay_dir = task_dir / "replay"
    m5out_dir = task_dir / "m5out"
    task_dir.mkdir(parents=True, exist_ok=True)
    elf_path = materialize_task(
        tasks, case_dir, task_id, replay_dir, external_only=False,
        dependency_dump_dir=dependency_dump_dir,
    ).resolve()

    command = [
        str(gem5), f"--outdir={m5out_dir}", str(config),
        f"--binary={elf_path}", f"--venus-config={venus_config}",
        f"--venus-sim-mode={sim_mode}",
    ]
    env = os.environ.copy()
    env["VENUS_GEM5_TASK_EBREAK_EXIT"] = "1"
    log_path = task_dir / "sim.log"
    start = time.monotonic()
    status = "passed"
    return_code = 0
    with log_path.open("w", encoding="utf-8") as log:
        try:
            result = subprocess.run(
                command, cwd=task_dir, env=env, stdout=log,
                stderr=subprocess.STDOUT, timeout=timeout_seconds,
                check=False,
            )
            return_code = result.returncode
            if return_code != 0:
                status = "failed"
        except subprocess.TimeoutExpired:
            status = "timeout"
            return_code = 124

    monitor_path = task_dir / "Debug" / "venusgem5_sequencer_monitor.json"
    # Fast mode intentionally has no full sequencer monitor.  Report that as
    # an observation policy, not as zero retired vector instructions.
    venus_retired = None if sim_mode == "fast" else 0
    if monitor_path.is_file():
        try:
            monitor = json.loads(monitor_path.read_text(encoding="utf-8"))
            venus_retired = sum(
                len(entry.get("Venus_instr", [])) for entry in monitor
                if isinstance(entry, dict)
            )
        except (ValueError, TypeError):
            status = "invalid-monitor"

    return {
        "task_id": task_id,
        "task_name": task_name,
        "status": status,
        "return_code": return_code,
        "wall_seconds": round(time.monotonic() - start, 3),
        "sim_ticks": read_sim_stat(m5out_dir / "stats.txt", "simTicks"),
        "scalar_retired": read_sim_stat(m5out_dir / "stats.txt", "simInsts"),
        "venus_retired": venus_retired,
        "sim_mode": sim_mode,
        "directory": str(task_dir),
    }


def run_dag_replay(tasks, case_dir, run_dir, gem5, config, venus_config,
                   sim_mode, jobs, timeout_seconds, selected_tasks,
                   dependency_dump_dir):
    deps = dependency_graph(tasks)
    levels = topological_levels(tasks, deps)
    selected = set(range(len(tasks))) if not selected_tasks else set(selected_tasks)
    invalid = sorted(selected - set(range(len(tasks))))
    if invalid:
        raise ValueError(f"task index out of range: {invalid}")

    run_dir.mkdir(parents=True, exist_ok=True)
    results = []
    for level_index, level in enumerate(levels):
        runnable = [task_id for task_id in level if task_id in selected]
        if not runnable:
            continue
        print(f"L1 level {level_index}: {','.join(map(str, runnable))}")
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=min(jobs, len(runnable))
        ) as executor:
            futures = {
                executor.submit(
                    run_replay_task, tasks[task_id], tasks, case_dir,
                    run_dir, gem5, config, venus_config, sim_mode,
                    timeout_seconds, dependency_dump_dir
                ): task_id
                for task_id in runnable
            }
            for future in concurrent.futures.as_completed(futures):
                result = future.result()
                results.append(result)
                venus_display = (
                    "disabled" if result["venus_retired"] is None
                    else result["venus_retired"]
                )
                print(
                    f"  task {result['task_id']:2d} {result['status']:<8} "
                    f"scalar={result['scalar_retired']} "
                    f"venus={venus_display}"
                )

        failed = [result for result in results if result["status"] != "passed"]
        if failed:
            print("stopping at first failed L1 level")
            break

    results.sort(key=lambda result: result["task_id"])
    summary = {
        "mode": "golden-input DAG replay",
        "venus_config": venus_config,
        "topological_levels": levels,
        "tasks": results,
    }
    summary_path = run_dir / "dag_replay_summary.json"
    summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(f"summary: {summary_path}")
    return 0 if all(result["status"] == "passed" for result in results) else 1


def build_inprocess_manifest(tasks, combined_bin, case_dir, run_dir):
    replay_dir = run_dir / "replay"
    replay_dir.mkdir(parents=True, exist_ok=True)
    manifest_tasks = []
    dependency_inputs = []
    initial_inputs = []
    outputs = []
    dmt_slots = build_legacy_dmt_slots(tasks)
    combined = combined_bin.read_bytes()
    static_capacities = static_allocation_capacities(tasks, len(combined))
    static_pointer_records = []
    shared_l2 = replay_dir / "shared_l2.bin"
    shared_l2.write_bytes(combined)
    pointer_dir = replay_dir / "dmt_pointers"
    for task in tasks:
        task_id = task["current_taskId"]
        task_dir = replay_dir / f"task_{task_id:02d}"
        elf = materialize_task(
            tasks, case_dir, task_id, task_dir,
            external_only=True,
        ).resolve()
        code_path, data_path = materialize_scheduler_images(
            task, case_dir, task_dir)
        manifest_tasks.append({
            "id": task_id,
            "name": task["debug_task_name"],
            "elf": str(elf),
            "code_file": str(code_path),
            "data_file": str(data_path),
            "code_source": 0,
            "data_source": int(task.get("data_offset", 0)),
            # Legacy post-synthesis JSON has no packaged L1 CRC field.  Zero
            # is the neutral cold-tile tag; it must still be explicit because
            # the runtime allocator consumes a dense task_crcs vector.
            "crc": int(task.get("crc", 0)),
            "hardware_requirement": int(
                task.get("hardwareinfo", "0b0"), 0),
            "need_spmd": bool(task.get("is_spmd", 0)),
            "minimum_spmd_tasks": int(task.get("min_core_num", 0)),
            "output_count": len(task.get("all_output", [])),
        })
        initial_dir = task_dir / "initial_inputs"
        for input_index, input_desc in enumerate(task.get("all_input", [])):
            parent = parse_parent(input_desc)
            if parent is None:
                input_type = int(input_desc.get("type", "0"), 0)
                if input_type in (5, 6):
                    payload = static_pointer_payload(input_desc, static_capacities)
                    offset = int(input_desc["offset"])
                    static_pointer_records.append({
                        "task": task_id, "input_index": input_index,
                        "name": input_desc.get("name"), "type": input_type,
                        "shared_l2_offset": offset,
                        "record_length_bytes": int(input_desc["length"]),
                        "backing_span_bytes": static_capacities[offset],
                        "transport_bytes": len(payload),
                    })
                else:
                    payload = decode_input_data(input_desc)
                initial_dir.mkdir(parents=True, exist_ok=True)
                input_path = initial_dir / f"input_{input_index:02d}.bin"
                input_path.write_bytes(payload)
                initial_inputs.append({
                    "task": task_id,
                    "destination": int(input_desc["dest_address"], 0),
                    "file": str(input_path.resolve()),
                })
                continue
            input_type = int(input_desc.get("type", "0"), 0)
            consumer_capacity = dependency_input_length(tasks, input_desc, parent)
            slot = dmt_slots[parent]
            dependency = {
                "task": task_id,
                "parent": parent[0],
                "port": parent[1],
                "destination": int(input_desc["dest_address"], 0),
                "length": 64 if input_type == 4 else consumer_capacity,
                "type": input_type,
                "descriptor_index": int(input_desc.get("index", input_index)),
                "slot_address": slot["address"],
                "slot_address_valid": True,
                "slot_consumer_bytes": consumer_capacity,
            }
            if input_type == 4:
                pointer_dir.mkdir(parents=True, exist_ok=True)
                pointer_path = pointer_dir / (
                    f"task_{task_id:02d}_input_{input_index:02d}.bin")
                pointer_path.write_bytes(packed_dmt_pointer(
                    consumer_capacity, slot["address"]))
                dependency["pointer_file"] = str(pointer_path.resolve())
            dependency_inputs.append(dependency)
        for port, output_desc in enumerate(task.get("all_output", [])):
            # Version-2 manifests obtain source and valid length from the
            # task's architectural vreturn descriptor. Preserve the legacy
            # fixed-source path only when an old descriptor carries it.
            if "src_address" in output_desc:
                outputs.append({
                    "task": task_id,
                    "port": port,
                    "source": int(output_desc["src_address"], 0),
                    "length": int(output_desc["length"]),
                })
    manifest = {
        "version": 2,
        "mode": "rtl-aligned-in-process",
        "tasks": manifest_tasks,
        "dependency_inputs": dependency_inputs,
        "initial_inputs": initial_inputs,
        "static_pointer_records": static_pointer_records,
        "outputs": outputs,
        "shared_l2_image": str(shared_l2.resolve()),
        "trace_file": str((run_dir / "venus_dag_trace.jsonl").resolve()),
        "output_dump_dir": str(run_dir.resolve()),
    }
    manifest_path = (run_dir / "venus_dag_manifest.json").resolve()
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest_path


def run_dag_inprocess(tasks, combined_bin, case_dir, run_dir, gem5, config,
                      venus_config, sim_mode, timeout_seconds):
    run_dir.mkdir(parents=True, exist_ok=True)
    manifest = build_inprocess_manifest(tasks, combined_bin, case_dir, run_dir)
    m5out_dir = (run_dir / "m5out").resolve()
    command = [
        str(gem5), f"--outdir={m5out_dir}", str(config),
        f"--dag-manifest={manifest}", f"--venus-config={venus_config}",
        f"--venus-sim-mode={sim_mode}",
    ]
    env = os.environ.copy()
    env["VENUS_GEM5_DAG"] = "1"
    log_path = run_dir / "sim.log"
    print("running one-process RTL-aligned DAG")
    print("command: " + " ".join(command))
    with log_path.open("w", encoding="utf-8") as log:
        try:
            result = subprocess.run(
                command, cwd=run_dir, env=env, stdout=log,
                stderr=subprocess.STDOUT, timeout=timeout_seconds,
                check=False,
            )
            return result.returncode
        except subprocess.TimeoutExpired:
            print(f"timeout after {timeout_seconds}s; log: {log_path}")
            return 124


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("descriptor", type=Path, help="post-synthesis dag1.json")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("inspect", help="validate and print task dependencies")
    materialize = subparsers.add_parser("materialize", help="create a task replay ELF")
    materialize.add_argument("--case-dir", type=Path, required=True)
    materialize.add_argument("--task", type=int, required=True)
    materialize.add_argument("--output-dir", type=Path, required=True)
    materialize.add_argument(
        "--entry-base", type=lambda value: int(value, 0), default=0x4,
        help="SE-mode entry address (default: 0x4, skipping the x1 clear at PC 0)",
    )
    materialize.add_argument(
        "--local-memory-size", type=lambda value: int(value, 0), default=0x200000,
        help="mapped scalar/local memory size (default: 0x200000)",
    )
    materialize.add_argument(
        "--external-only", action="store_true",
        help="initialize only static inputs; leave dependency inputs for the scheduler",
    )
    run_dag = subparsers.add_parser(
        "run-dag", help="run tasks by dependency level using descriptor golden inputs"
    )
    run_dag.add_argument("--case-dir", type=Path, required=True)
    run_dag.add_argument("--run-dir", type=Path, required=True)
    run_dag.add_argument(
        "--gem5", type=Path,
        help=(
            "gem5 executable (default: gem5.opt for --sim-mode=fast, "
            "otherwise gem5.debug)"
        ),
    )
    run_dag.add_argument(
        "--config", type=Path,
        default=Path("configs/tutorial/part1/packet_gen.py"),
    )
    run_dag.add_argument(
        "--venus-config",
        choices=["venus1p0-64x512", "venus1p0-64x512-legacy-bpll",
                 "venus1p0-64x512-300mhz", "venus2p0-16x128",
                 "venus-rtl-16x128", "venus-rtl-16x512"],
        default="venus-rtl-16x128",
    )
    run_dag.add_argument("--jobs", type=int, default=1)
    run_dag.add_argument("--timeout", type=int, default=600)
    run_dag.add_argument(
        "--sim-mode", choices=["verification", "fast"],
        default="verification",
        help="select the packet_gen observation profile",
    )
    run_dag.add_argument(
        "--tasks", type=int, nargs="*",
        help="run only these task IDs (useful for staged bring-up)",
    )
    run_dag.add_argument(
        "--dependency-dump-dir", type=Path,
        help=(
            "directory containing task_<id>_port_<port>.bin dumps from a "
            "completed in-process DAG; use them to replay dependency inputs"
        ),
    )
    inprocess = subparsers.add_parser(
        "run-dag-inprocess",
        help="run the full DAG in one gem5 process with RTL-shaped L1/DMT/DMA state",
    )
    inprocess.add_argument("--case-dir", type=Path, required=True)
    inprocess.add_argument("--combined-bin", type=Path, required=True)
    inprocess.add_argument("--run-dir", type=Path, required=True)
    inprocess.add_argument(
        "--gem5", type=Path,
        help=(
            "gem5 executable (default: gem5.opt for --sim-mode=fast, "
            "otherwise gem5.debug)"
        ),
    )
    inprocess.add_argument(
        "--config", type=Path,
        default=Path("configs/tutorial/part1/packet_gen.py"),
    )
    inprocess.add_argument(
        "--venus-config",
        choices=["venus1p0-64x512", "venus1p0-64x512-legacy-bpll",
                 "venus1p0-64x512-300mhz", "venus2p0-16x128",
                 "venus-rtl-16x128", "venus-rtl-16x512"],
        default="venus-rtl-16x128",
    )
    inprocess.add_argument("--timeout", type=int, default=1800)
    inprocess.add_argument(
        "--sim-mode", choices=["verification", "fast"],
        default="verification",
        help="select the packet_gen observation profile",
    )
    return parser


def main():
    args = build_parser().parse_args()
    try:
        tasks = load_tasks(args.descriptor)
        if args.command == "inspect":
            print_summary(tasks)
        elif args.command == "materialize":
            if args.task < 0 or args.task >= len(tasks):
                raise ValueError(f"task index out of range: {args.task}")
            dependency_graph(tasks)
            materialize_task(
                tasks, args.case_dir, args.task, args.output_dir,
                args.external_only, args.entry_base, args.local_memory_size
            )
        elif args.command == "run-dag":
            if args.jobs < 1:
                raise ValueError("--jobs must be at least 1")
            repo_dir = Path(__file__).resolve().parent.parent
            selected_gem5 = args.gem5 or Path(
                "build/RISCV/gem5.opt" if args.sim_mode == "fast"
                else "build/RISCV/gem5.debug"
            )
            gem5 = (selected_gem5 if selected_gem5.is_absolute()
                    else repo_dir / selected_gem5)
            config = args.config if args.config.is_absolute() else repo_dir / args.config
            if not gem5.is_file():
                raise FileNotFoundError(gem5)
            if not config.is_file():
                raise FileNotFoundError(config)
            return run_dag_replay(
                tasks, args.case_dir.resolve(), args.run_dir.resolve(),
                gem5.resolve(), config.resolve(), args.venus_config,
                args.sim_mode, args.jobs, args.timeout, args.tasks,
                args.dependency_dump_dir.resolve()
                if args.dependency_dump_dir else None,
            )
        elif args.command == "run-dag-inprocess":
            repo_dir = Path(__file__).resolve().parent.parent
            selected_gem5 = args.gem5 or Path(
                "build/RISCV/gem5.opt" if args.sim_mode == "fast"
                else "build/RISCV/gem5.debug"
            )
            gem5 = (selected_gem5 if selected_gem5.is_absolute()
                    else repo_dir / selected_gem5)
            config = args.config if args.config.is_absolute() else repo_dir / args.config
            if not gem5.is_file():
                raise FileNotFoundError(gem5)
            if not config.is_file():
                raise FileNotFoundError(config)
            dependency_graph(tasks)
            return run_dag_inprocess(
                tasks, args.combined_bin.resolve(), args.case_dir.resolve(),
                args.run_dir.resolve(),
                gem5.resolve(), config.resolve(), args.venus_config,
                args.sim_mode, args.timeout,
            )
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
