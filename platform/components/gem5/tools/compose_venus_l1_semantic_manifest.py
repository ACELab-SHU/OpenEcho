#!/usr/bin/env python3
"""Compose an RTL-L1 timing manifest with a qualified functional replay.

``venus_l1_dag.py`` reconstructs the Scheduler/DMT transfer contract from an
L1 ELF.  Some applications also have launcher-populated shared-L2 bytes or a
backend ABI adapter which are deliberately absent from the compact L1 DAG
blob.  In that case the L1-derived manifest is the timing authority while an
already-qualified replay manifest remains the functional input authority.

This composer is intentionally topology- and task-name-agnostic.  It accepts
the pairing only when every task has byte-identical code and data and when the
L1 shared-L2 image is an exact prefix of the functional image.  It does not
copy observed timestamps, add delays, or alter the semantic fire/return plan.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_manifest(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} is not a JSON object")
    return value


def resolved_file(manifest_path: Path, value: str) -> Path:
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = manifest_path.parent / path
    path = path.resolve()
    if not path.is_file():
        raise ValueError(f"referenced file does not exist: {path}")
    return path


def tasks_by_id(manifest: dict, path: Path) -> dict[int, dict]:
    tasks = manifest.get("tasks")
    if not isinstance(tasks, list) or not tasks:
        raise ValueError(f"{path} has no non-empty tasks array")
    result = {}
    for task in tasks:
        if not isinstance(task, dict) or not isinstance(task.get("id"), int):
            raise ValueError(f"{path} has a task without an integer id")
        if task["id"] in result:
            raise ValueError(f"{path} has duplicate task id {task['id']}")
        result[task["id"]] = task
    return result


def file_identity(task: dict, key: str, manifest_path: Path) -> dict:
    path = resolved_file(manifest_path, task[key])
    return {"path": str(path), "bytes": path.stat().st_size,
            "sha256": sha256(path)}


def compose(semantic_path: Path, functional_path: Path,
            output_path: Path) -> dict:
    semantic_path = semantic_path.resolve()
    functional_path = functional_path.resolve()
    semantic = load_manifest(semantic_path)
    functional = load_manifest(functional_path)
    if semantic.get("version") != 5 or not semantic.get("fire_plan"):
        raise ValueError("semantic manifest must be version 5 with a fire_plan")

    semantic_tasks = tasks_by_id(semantic, semantic_path)
    functional_tasks = tasks_by_id(functional, functional_path)
    if set(semantic_tasks) != set(functional_tasks):
        raise ValueError("semantic and functional task-id sets differ")

    task_checks = []
    composed_tasks = []
    for task_id in sorted(semantic_tasks):
        timing_task = semantic_tasks[task_id]
        replay_task = functional_tasks[task_id]
        checks = {}
        for key in ("code_file", "data_file"):
            timing_identity = file_identity(timing_task, key, semantic_path)
            replay_identity = file_identity(replay_task, key, functional_path)
            if (timing_identity["bytes"], timing_identity["sha256"]) != (
                    replay_identity["bytes"], replay_identity["sha256"]):
                raise ValueError(
                    f"task {task_id} {key} differs between manifests")
            checks[key] = {
                "bytes": timing_identity["bytes"],
                "sha256": timing_identity["sha256"],
            }
        for key in ("code_source", "data_source", "output_count",
                    "hardware_requirement", "minimum_spmd_tasks"):
            if timing_task.get(key) != replay_task.get(key):
                raise ValueError(f"task {task_id} metadata differs at {key}")

        # Preserve all semantic Scheduler metadata.  Only the executable
        # replay image and readable task name come from the qualified
        # functional adapter whose code/data identity was proven above.
        composed_task = dict(timing_task)
        composed_task["name"] = replay_task.get("name", timing_task["name"])
        for key in ("elf", "code_file", "data_file"):
            composed_task[key] = str(resolved_file(
                functional_path, replay_task[key]))
        composed_tasks.append(composed_task)
        task_checks.append({"task_id": task_id, **checks})

    semantic_l2 = resolved_file(
        semantic_path, semantic["shared_l2_image"])
    functional_l2 = resolved_file(
        functional_path, functional["shared_l2_image"])
    semantic_bytes = semantic_l2.read_bytes()
    functional_bytes = functional_l2.read_bytes()
    if len(functional_bytes) < len(semantic_bytes):
        raise ValueError("functional shared-L2 image is shorter than L1 image")
    if functional_bytes[:len(semantic_bytes)] != semantic_bytes:
        raise ValueError(
            "functional shared-L2 image does not preserve the L1 image prefix")

    initial_inputs = functional.get("initial_inputs")
    if not isinstance(initial_inputs, list):
        raise ValueError("functional manifest has no initial_inputs array")
    for entry in initial_inputs:
        if not isinstance(entry, dict) or "file" not in entry:
            raise ValueError("functional initial_inputs contains an invalid entry")
        entry["file"] = str(resolved_file(functional_path, entry["file"]))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    result = dict(semantic)
    result["mode"] = "rtl-l1-runtime-dag-with-qualified-functional-adapter"
    result["tasks"] = composed_tasks
    result["initial_inputs"] = initial_inputs
    result["shared_l2_image"] = str(functional_l2)
    result["output_dump_dir"] = str(output_path.parent.resolve())
    result["trace_file"] = str(
        (output_path.parent / "venus_dag_trace.jsonl").resolve())
    result["functional_adapter"] = {
        "schema": "ace-echo-venus-l1-functional-composition/v1",
        "semantic_manifest": str(semantic_path),
        "semantic_manifest_sha256": sha256(semantic_path),
        "functional_manifest": str(functional_path),
        "functional_manifest_sha256": sha256(functional_path),
        "task_code_data_identity": task_checks,
        "semantic_shared_l2_bytes": len(semantic_bytes),
        "semantic_shared_l2_sha256": sha256(semantic_l2),
        "functional_shared_l2_bytes": len(functional_bytes),
        "functional_shared_l2_sha256": sha256(functional_l2),
        "functional_image_preserves_semantic_prefix": True,
        "timing_authority": "semantic fire_plan/return_plan from L1 ELF",
        "functional_authority": (
            "qualified replay ELF, initial_inputs, and launcher-populated "
            "shared-L2 image"),
        "observed_timestamps_consumed": False,
        "fixed_delay_added": False,
    }
    output_path.write_text(json.dumps(result, indent=2) + "\n",
                           encoding="utf-8")
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--semantic-manifest", type=Path, required=True)
    parser.add_argument("--functional-manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = compose(args.semantic_manifest, args.functional_manifest,
                     args.output)
    print(json.dumps(result["functional_adapter"], indent=2))


if __name__ == "__main__":
    main()
