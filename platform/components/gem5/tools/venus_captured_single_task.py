#!/usr/bin/env python3
"""Build a self-contained one-task replay from captured parent returns.

The normal runtime manifest keeps dependency inputs in DMT/shared L2 and only
dispatches a consumer after its parents complete.  For cycle-level debugging
we often already have byte-exact parent-return captures.  This tool seeds
those bytes into a private shared-L2 image, materializes the corresponding
direct or ``ptr_temp`` input record, and emits a legacy in-process manifest
containing only the selected consumer task.

This is a replay aid, not a timing oracle: no observed start/end cycle is
copied into the result, and the task still executes through the normal CPU,
sequencer, vector units, VRF arbitration, and LSU model.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


POINTER_RECORD_BYTES = 64


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--task", required=True, type=int)
    parser.add_argument(
        "--capture-dir", required=True, type=Path,
        help="directory containing task_<parent>_port_<port>.bin",
    )
    parser.add_argument("--output-dir", required=True, type=Path)
    return parser.parse_args()


def capture_path(directory: Path, parent: int, port: int) -> Path:
    candidates = (
        directory / f"task_{parent}_port_{port}.bin",
        directory / f"task_{parent:02d}_port_{port}.bin",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError(
        f"no capture for dependency {parent}:{port} in {directory}"
    )


def pointer_record(capacity: int, address: int) -> bytes:
    if not 0 < capacity <= 0xFFFF:
        raise ValueError(f"ptr_temp capacity {capacity} does not fit uint16")
    if not 0 <= address <= 0xFFFFFFFF:
        raise ValueError(f"ptr_temp address {address:#x} does not fit uint32")
    result = bytearray(POINTER_RECORD_BYTES)
    result[0:2] = capacity.to_bytes(2, "little")
    result[2:6] = address.to_bytes(4, "little")
    return bytes(result)


def main():
    args = parse_args()
    source = json.loads(args.manifest.read_text(encoding="utf-8"))
    tasks = {int(task["id"]): task for task in source["tasks"]}
    if args.task not in tasks:
        raise SystemExit(f"task {args.task} is absent from {args.manifest}")

    run_dir = args.output_dir.resolve()
    input_dir = run_dir / "inputs"
    input_dir.mkdir(parents=True, exist_ok=True)

    shared_source = source.get("shared_l2_image")
    shared = bytearray(Path(shared_source).read_bytes()) if shared_source else bytearray()

    initial_inputs = []
    for entry in source.get("initial_inputs", []):
        if int(entry["task"]) != args.task:
            continue
        replay = dict(entry)
        replay["task"] = 0
        replay["file"] = str(Path(entry["file"]).resolve())
        initial_inputs.append(replay)

    dependency_provenance = []
    dependencies = [
        entry for entry in source.get("dependency_inputs", [])
        if int(entry["task"]) == args.task
    ]
    for index, dependency in enumerate(dependencies):
        parent = int(dependency["parent"])
        port = int(dependency["port"])
        input_type = int(dependency.get("type", 0))
        destination = int(dependency["destination"])
        captured_path = capture_path(args.capture_dir, parent, port)
        captured = captured_path.read_bytes()

        if input_type == 0:
            length = int(dependency.get("length", 0)) or len(captured)
            if len(captured) < length:
                if not dependency.get(
                        "slot_address_valid", "slot_address" in dependency):
                    raise ValueError(
                        f"capture {captured_path} has {len(captured)} bytes; "
                        f"direct dependency needs {length} and has no DMT "
                        "slot backing"
                    )
                # A runtime return may write fewer bytes than the fixed DMT
                # consumer span.  RTL leaves the remainder of that shared-L2
                # slot unchanged; it does not synthesize a larger parent
                # return.  Reconstruct the consumer DMA payload from the
                # initial slot image and overlay only the bytes actually
                # returned by the parent.
                address = int(dependency["slot_address"])
                end = address + length
                if len(shared) < end:
                    shared.extend(bytes(end - len(shared)))
                payload_buffer = bytearray(shared[address:end])
                payload_buffer[:len(captured)] = captured
                payload = bytes(payload_buffer)
            else:
                payload = captured[:length]
            replay_path = input_dir / f"dependency_{index:02d}_direct.bin"
        elif input_type == 4:
            if not dependency.get(
                    "slot_address_valid", "slot_address" in dependency):
                raise ValueError(
                    f"ptr_temp dependency {parent}:{port} has no DMT address"
                )
            address = int(dependency["slot_address"])
            capacity = int(dependency.get(
                "slot_consumer_bytes", dependency.get("slot_capacity", 0)
            ))
            if capacity == 0:
                raise ValueError(
                    f"ptr_temp dependency {parent}:{port} has no capacity"
                )
            if len(captured) > capacity:
                raise ValueError(
                    f"capture {captured_path} has {len(captured)} bytes; "
                    f"DMT slot capacity is {capacity}"
                )
            end = address + capacity
            if len(shared) < end:
                shared.extend(bytes(end - len(shared)))
            shared[address:address + len(captured)] = captured
            payload = pointer_record(capacity, address)
            replay_path = input_dir / f"dependency_{index:02d}_ptr_temp.bin"
        else:
            raise ValueError(
                f"unsupported dependency type {input_type} for {parent}:{port}"
            )

        replay_path.write_bytes(payload)
        initial_inputs.append({
            "task": 0,
            "destination": destination,
            "file": str(replay_path.resolve()),
            "source_dependency": {"task": parent, "port": port},
        })
        dependency_provenance.append({
            "parent": parent,
            "port": port,
            "type": input_type,
            "destination": destination,
            "capture": str(captured_path),
            "captured_bytes": len(captured),
            "replay_file": str(replay_path.resolve()),
        })

    shared_path = run_dir / "shared_l2.bin"
    shared_path.write_bytes(shared)

    task = dict(tasks[args.task])
    task["id"] = 0
    task["name"] = f"captured_replay_{task.get('name', args.task)}"
    for key in ("elf", "code_file", "data_file"):
        if key in task:
            task[key] = str(Path(task[key]).resolve())

    manifest = {
        "version": 2,
        "mode": "rtl-captured-single-task-replay",
        "source_manifest": str(args.manifest.resolve()),
        "source_task": args.task,
        "tasks": [task],
        "dependency_inputs": [],
        "initial_inputs": initial_inputs,
        "outputs": [],
        "shared_l2_image": str(shared_path.resolve()),
        "trace_file": str((run_dir / "venus_dag_trace.jsonl").resolve()),
        "output_dump_dir": str(run_dir),
    }
    manifest_path = run_dir / "venus_dag_manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    provenance_path = run_dir / "capture_provenance.json"
    provenance_path.write_text(
        json.dumps({
            "source_manifest": str(args.manifest.resolve()),
            "source_task": args.task,
            "dependencies": dependency_provenance,
        }, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"manifest: {manifest_path}")
    print(f"capture provenance: {provenance_path}")


if __name__ == "__main__":
    main()
