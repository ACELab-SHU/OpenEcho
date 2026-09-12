#!/usr/bin/env python3
"""Prepare a Venus DAG manifest for an isolated GEM5 run.

Besides relocating generated outputs, materialize the dense shared-L2 image
promised by the manifest.  Older manifests can carry byte-complete
``initial_inputs`` whose architectural source offsets extend past the
canonical DAG blob.  Timing DMA reads the shared-L2 image, not the replay ELF,
so leaving those bytes only in per-task files silently turns valid inputs
into zeroes.
"""

import argparse
import json
from pathlib import Path


SHARED_L2_LIMIT = 32 * 1024 * 1024


def resolve_artifact(path, manifest_dir):
    path = Path(path)
    return path if path.is_absolute() else manifest_dir / path


def materialize_shared_l2(manifest, manifest_dir, output):
    """Merge declared shared-L2 inputs into a run-local dense image.

    This is driven solely by the manifest's architectural source mapping.
    Task names, PCs, destination addresses, and payload values do not affect
    admission.  Conflicting aliases fail rather than choosing an order.
    """
    source_path = resolve_artifact(
        manifest["shared_l2_image"], manifest_dir).resolve()
    image = bytearray(source_path.read_bytes())
    canonical_size = len(image)
    written = {}
    input_count = 0
    input_bytes = 0

    for entry in manifest.get("initial_inputs", []):
        source = entry.get("source", {})
        if source.get("space") != "shared_l2":
            continue
        start = int(source["offset"])
        payload_path = resolve_artifact(
            entry["file"], manifest_dir).resolve()
        payload = payload_path.read_bytes()
        end = start + len(payload)
        if start < 0 or end > SHARED_L2_LIMIT:
            raise ValueError(
                f"task {entry.get('task')} shared-L2 input "
                f"[0x{start:x}, 0x{end:x}) is outside the 32 MiB model")

        for offset, value in enumerate(payload):
            address = start + offset
            previous = written.get(address)
            if previous is not None and previous != value:
                raise ValueError(
                    f"conflicting shared-L2 inputs at 0x{address:x}")
            if address < canonical_size and image[address] != value:
                raise ValueError(
                    f"shared-L2 input conflicts with canonical image at "
                    f"0x{address:x}")
            written[address] = value

        if len(image) < end:
            image.extend(b"\0" * (end - len(image)))
        image[start:end] = payload
        input_count += 1
        input_bytes += len(payload)

    output.write_bytes(image)
    return {
        "source": str(source_path),
        "bytes": len(image),
        "declared_inputs": input_count,
        "declared_input_bytes": input_bytes,
        "unique_input_bytes": len(written),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--run-dir", type=Path, required=True)
    args = parser.parse_args()

    run_dir = args.run_dir.resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    input_manifest = args.input.resolve()
    manifest = json.loads(input_manifest.read_text(encoding="utf-8"))
    shared_l2 = run_dir / "shared_l2.bin"
    shared_l2_summary = materialize_shared_l2(
        manifest, input_manifest.parent, shared_l2)
    manifest["shared_l2_image"] = str(shared_l2)
    manifest["shared_l2_materialization"] = shared_l2_summary
    manifest["trace_file"] = str(run_dir / "venus_dag_trace.jsonl")
    manifest["output_dump_dir"] = str(run_dir)
    output = run_dir / "venus_dag_manifest.json"
    output.write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(output)


if __name__ == "__main__":
    main()
