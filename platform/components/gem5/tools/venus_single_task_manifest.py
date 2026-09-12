#!/usr/bin/env python3
"""Build a replayable, one-task Venus DAG manifest from a VEMU DAG image.

VEMU final_output JSON records static input offsets into the combined DAG BIN,
whereas gem5's in-process manifest needs concrete byte files.  This utility
materializes one task without changing the original JSON/BIN or its input
classification.  It is intended for task-level RTL/GEM5 convergence work.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path

from venus_dag import (decode_input_data, load_tasks, materialize_scheduler_images,
                       materialize_task, parse_parent,
                       static_allocation_capacities)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dag-json", type=Path, required=True)
    parser.add_argument("--combined-bin", type=Path, required=True)
    parser.add_argument("--case-dir", type=Path, required=True,
                        help="directory containing Task_<name>.bin images")
    parser.add_argument("--task", type=int, required=True,
                        help="current_taskId from the VEMU DAG JSON")
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def hydrate_static_inputs(task, combined, capacities):
    provenance = []
    for input_desc in task.get("all_input", []):
        if parse_parent(input_desc) is not None:
            raise ValueError(
                f"task {task['current_taskId']} input {input_desc.get('name')} "
                "is a dependency; use a multi-task manifest instead")
        length = int(input_desc["length"])
        offset = int(input_desc["offset"])
        end = offset + length
        if offset < 0 or end > len(combined):
            raise ValueError(
                f"input {input_desc.get('name')} range [{offset}, {end}) "
                f"is outside combined image ({len(combined)} bytes)")
        input_type = int(input_desc.get("type", "0"), 0)
        if input_type in (5, 6):
            capacity = capacities.get(offset, 0)
            if not 0 < capacity <= 0xffff:
                raise ValueError(
                    f"pointer input {input_desc.get('name')} allocation "
                    f"capacity {capacity} does not fit its packed descriptor")
            payload = capacity | (offset << 16)
            payload = payload.to_bytes(64, "little")
        else:
            payload = combined[offset:end]
        input_desc["data"] = "0x" + payload[::-1].hex()
        provenance.append({
            "name": input_desc.get("name"),
            "offset": offset,
            "length": length,
            "input_type": input_type,
            "pointer_capacity": capacities.get(offset) if input_type in (5, 6) else None,
            "sha256": hashlib.sha256(payload).hexdigest(),
        })
    return provenance


def main():
    args = parse_args()
    tasks = load_tasks(args.dag_json)
    if args.task < 0 or args.task >= len(tasks):
        raise SystemExit(f"task {args.task} is outside [0, {len(tasks)})")

    task = copy.deepcopy(tasks[args.task])
    combined = args.combined_bin.read_bytes()
    capacities = static_allocation_capacities(tasks, len(combined))
    provenance = hydrate_static_inputs(task, combined, capacities)

    run_dir = args.output_dir.resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    shared_l2 = run_dir / "shared_l2.bin"
    shared_l2.write_bytes(combined)
    task_dir = run_dir / "task_00"
    replay_dir = task_dir / "replay"
    task["current_taskId"] = 0
    elf_path = materialize_task([task], args.case_dir, 0, replay_dir).resolve()
    code_path, data_path = materialize_scheduler_images(
        task, args.case_dir, task_dir)

    initial_inputs = []
    input_dir = task_dir / "initial_inputs"
    input_dir.mkdir(parents=True, exist_ok=True)
    for index, input_desc in enumerate(task.get("all_input", [])):
        payload = decode_input_data(input_desc)
        input_path = input_dir / f"input_{index:02d}.bin"
        input_path.write_bytes(payload)
        initial_inputs.append({
            "task": 0,
            "destination": int(input_desc["dest_address"], 0),
            "file": str(input_path.resolve()),
        })

    manifest = {
        "version": 2,
        "mode": "vemu-static-single-task",
        "tasks": [{
            "id": 0,
            "name": task["debug_task_name"],
            "elf": str(elf_path),
            "code_file": str(code_path),
            "data_file": str(data_path),
            "code_source": 0,
            "data_source": int(task.get("data_offset", 0)),
            "crc": int(task.get("crc", 0)),
            "hardware_requirement": int(
                task.get("hardwareinfo", "0b0"), 0),
            "need_spmd": bool(task.get("is_spmd", 0)),
            "minimum_spmd_tasks": int(task.get("min_core_num", 0)),
            "output_count": int(task["Output_Num"]),
        }],
        "dependency_inputs": [],
        "initial_inputs": initial_inputs,
        # Version-2 capture obtains source and valid length from the actual
        # vreturn descriptor.  No synthetic output source is permitted here.
        "outputs": [],
        "shared_l2_image": str(shared_l2.resolve()),
        "trace_file": str((run_dir / "venus_dag_trace.jsonl").resolve()),
        "output_dump_dir": str(run_dir),
    }
    manifest_path = run_dir / "venus_dag_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n",
                             encoding="utf-8")
    provenance_path = run_dir / "input_provenance.json"
    provenance_path.write_text(json.dumps({
        "dag_json": str(args.dag_json.resolve()),
        "combined_bin": str(args.combined_bin.resolve()),
        "task": args.task,
        "inputs": provenance,
    }, indent=2) + "\n", encoding="utf-8")
    print(f"manifest: {manifest_path}")
    print(f"input provenance: {provenance_path}")


if __name__ == "__main__":
    main()
