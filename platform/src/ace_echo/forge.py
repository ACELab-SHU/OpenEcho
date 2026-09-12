from __future__ import annotations

from dataclasses import asdict, dataclass
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
from typing import Any

from .artifacts import safe_name


class ForgeContractError(ValueError):
    pass


REQUEST_SCHEMA = "ace-echo-forge-request/v1"
BACKEND_SCHEMA = "ace-echo-backend-manifest/v1"
REFERENCE_SCHEMA = "ace-echo-reference-adapter/v1"
RUN_SCHEMA = "ace-echo-forge-run/v1"
FINAL_SCHEMA = "ace-echo-forge-final-report/v1"

REFERENCE_STATES = {
    "MISSING",
    "SOURCE_IMPLEMENTATION",
    "PROVISIONAL_GOLDEN",
    "AUTHORITATIVE_GOLDEN",
}
WORKLOAD_KINDS = {"task", "dag", "application"}
SOURCE_KINDS = {
    "intent_only",
    "spec_only",
    "executable_source",
    "venus_source",
}
HEAVY_CANDIDATE_ENTRIES = {
    "artifacts",
    "build",
    "dumps",
    "generated-source",
    "logs",
    "m5out",
    "source-copy",
    "traces",
}


def _load_object(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ForgeContractError(f"cannot read {path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ForgeContractError(f"invalid JSON in {path}: {error}") from error
    if not isinstance(value, dict):
        raise ForgeContractError(f"top-level JSON value must be an object: {path}")
    return value


def _canonical_bytes(value: object) -> bytes:
    return json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=False,
    ).encode("utf-8")


def canonical_digest(value: object) -> str:
    return "sha256:" + hashlib.sha256(_canonical_bytes(value)).hexdigest()


def _all_strings(value: object):
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for key, item in value.items():
            yield str(key)
            yield from _all_strings(item)
    elif isinstance(value, list):
        for item in value:
            yield from _all_strings(item)


def _require_string(value: dict[str, Any], key: str, where: str) -> str:
    result = value.get(key)
    if not isinstance(result, str) or not result.strip():
        raise ForgeContractError(f"{where}.{key} must be a non-empty string")
    return result


def _require_mapping(value: dict[str, Any], key: str, where: str) -> dict[str, Any]:
    result = value.get(key)
    if not isinstance(result, dict):
        raise ForgeContractError(f"{where}.{key} must be an object")
    return result


def _positive_int(value: dict[str, Any], key: str, where: str) -> int:
    result = value.get(key)
    if not isinstance(result, int) or isinstance(result, bool) or result <= 0:
        raise ForgeContractError(f"{where}.{key} must be a positive integer")
    return result


def _resolve_declared_path(text: str, manifest_path: Path) -> Path:
    path = Path(text).expanduser()
    return (path if path.is_absolute() else manifest_path.parent / path).resolve()


def software_validation_mode(manifest: dict[str, Any]) -> str:
    """Select the full-matrix executor, not the correctness coverage.

    Older/custom manifests retain their conservative verification default.
    Explicit diagnostic requests never fall back to fast.
    """
    engine = manifest.get("execution_policy", {}).get(
        "full_software_engine", "gem5.verification"
    )
    if engine not in ("gem5.fast", "gem5.verification"):
        raise ForgeContractError("full_software_engine must be gem5.fast or gem5.verification")
    return engine.split(".")[1]


def validate_backend_manifest(path: Path) -> dict[str, Any]:
    path = path.expanduser().resolve()
    manifest = _load_object(path)
    if manifest.get("schema") != BACKEND_SCHEMA:
        raise ForgeContractError(f"backend schema must be {BACKEND_SCHEMA}")
    _require_string(manifest, "backend_id", "backend")

    architecture = _require_mapping(manifest, "architecture", "backend")
    for key in (
        "rows", "lanes", "banks_per_lane", "bank_width_bits",
        "workspace_alignment_bytes", "workspace_limit_bytes",
    ):
        _positive_int(architecture, key, "backend.architecture")

    abi = _require_mapping(manifest, "abi", "backend")
    descriptor = _require_mapping(
        abi, "task_input_descriptor", "backend.abi"
    )
    physical_type_bits = _positive_int(
        descriptor, "physical_type_bits", "backend.abi.task_input_descriptor"
    )
    logical_type_map = _require_mapping(
        descriptor,
        "logical_to_physical_type",
        "backend.abi.task_input_descriptor",
    )
    required_logical_types = {"0", "1", "2", "4", "5", "6"}
    if not required_logical_types.issubset(logical_type_map):
        missing = sorted(required_logical_types - set(logical_type_map))
        raise ForgeContractError(
            "backend.abi.task_input_descriptor.logical_to_physical_type "
            f"is missing logical types: {', '.join(missing)}"
        )
    for logical, physical in logical_type_map.items():
        try:
            logical_value = int(logical, 0)
        except (TypeError, ValueError) as error:
            raise ForgeContractError(
                "backend task input logical type keys must be integers"
            ) from error
        if logical_value < 0 or logical_value > 7:
            raise ForgeContractError(
                f"backend logical task input type {logical} is outside [0, 7]"
            )
        if (not isinstance(physical, int) or isinstance(physical, bool)
                or physical < 0 or physical >= (1 << physical_type_bits)):
            raise ForgeContractError(
                f"backend physical task input type {physical!r} for logical "
                f"type {logical} does not fit in {physical_type_bits} bits"
            )
    unsupported_logical_types = descriptor.get(
        "rtl_unsupported_logical_types"
    )
    if not isinstance(unsupported_logical_types, list):
        raise ForgeContractError(
            "backend.abi.task_input_descriptor."
            "rtl_unsupported_logical_types must be a list"
        )
    for logical in unsupported_logical_types:
        if (not isinstance(logical, int) or isinstance(logical, bool)
                or logical < 0 or logical > 7):
            raise ForgeContractError(
                "backend RTL-unsupported task input logical types must be "
                "integers in [0, 7]"
            )

    task_container = abi.get("task_container")
    if task_container is not None:
        if not isinstance(task_container, dict):
            raise ForgeContractError(
                "backend.abi.task_container must be an object"
            )
        includes_spmd_fields = task_container.get("includes_spmd_fields")
        if not isinstance(includes_spmd_fields, bool):
            raise ForgeContractError(
                "backend.abi.task_container.includes_spmd_fields must be "
                "a boolean"
            )
        rtl_evidence = task_container.get("rtl_evidence")
        if not isinstance(rtl_evidence, str) or not rtl_evidence:
            raise ForgeContractError(
                "backend.abi.task_container.rtl_evidence must be a "
                "non-empty string"
            )

    scheduler_output = abi.get("scheduler_output")
    if scheduler_output is not None:
        if not isinstance(scheduler_output, dict):
            raise ForgeContractError(
                "backend.abi.scheduler_output must be an object"
            )
        max_task_outputs = scheduler_output.get("max_task_outputs", 16)
        if (not isinstance(max_task_outputs, int)
                or isinstance(max_task_outputs, bool)
                or max_task_outputs <= 0):
            raise ForgeContractError(
                "backend.abi.scheduler_output.max_task_outputs must be a "
                "positive integer"
            )
        max_dag_outputs = scheduler_output.get("max_dag_outputs")
        if (not isinstance(max_dag_outputs, int)
                or isinstance(max_dag_outputs, bool)
                or max_dag_outputs <= 0):
            raise ForgeContractError(
                "backend.abi.scheduler_output.max_dag_outputs must be a "
                "positive integer"
            )
        return_length_bits = scheduler_output.get(
            "task_return_length_bits", 32
        )
        if (not isinstance(return_length_bits, int)
                or isinstance(return_length_bits, bool)
                or return_length_bits <= 0):
            raise ForgeContractError(
                "backend.abi.scheduler_output.task_return_length_bits must "
                "be a positive integer"
            )
        if scheduler_output.get("vreturn_length_unit", "bytes") != "bytes":
            raise ForgeContractError(
                "backend.abi.scheduler_output.vreturn_length_unit must be "
                "'bytes'"
            )
        variable_length = scheduler_output.get(
            "variable_length_outputs", True
        )
        if not isinstance(variable_length, bool):
            raise ForgeContractError(
                "backend.abi.scheduler_output.variable_length_outputs must "
                "be a boolean"
            )
        dma_beat_bytes = scheduler_output.get("dma_beat_bytes", 64)
        if (not isinstance(dma_beat_bytes, int)
                or isinstance(dma_beat_bytes, bool)
                or dma_beat_bytes <= 0):
            raise ForgeContractError(
                "backend.abi.scheduler_output.dma_beat_bytes must be a "
                "positive integer"
            )
        descriptor_lag = scheduler_output.get(
            "descriptor_response_lag_ports", 0
        )
        if (not isinstance(descriptor_lag, int)
                or isinstance(descriptor_lag, bool)
                or descriptor_lag < 0
                or descriptor_lag >= max_task_outputs):
            raise ForgeContractError(
                "backend.abi.scheduler_output."
                "descriptor_response_lag_ports must be an integer in "
                f"[0, {max_task_outputs - 1}]"
            )
        transfer_chunk = scheduler_output.get(
            "descriptor_transfer_chunk_bytes", 64
        )
        if (not isinstance(transfer_chunk, int)
                or isinstance(transfer_chunk, bool)
                or transfer_chunk < 0
                or (transfer_chunk != 0
                    and transfer_chunk % dma_beat_bytes != 0)):
            raise ForgeContractError(
                "backend.abi.scheduler_output."
                "descriptor_transfer_chunk_bytes must be zero for one "
                "continuous DMA or a positive multiple of dma_beat_bytes"
            )
        completion_fallback = scheduler_output.get(
            "completion_poll_fallback", False
        )
        if not isinstance(completion_fallback, bool):
            raise ForgeContractError(
                "backend.abi.scheduler_output."
                "completion_poll_fallback must be a boolean"
            )
        completion_interval = scheduler_output.get(
            "completion_poll_interval", 4096
        )
        if (not isinstance(completion_interval, int)
                or isinstance(completion_interval, bool)
                or completion_interval <= 0):
            raise ForgeContractError(
                "backend.abi.scheduler_output."
                "completion_poll_interval must be a positive integer"
            )

    execution = _require_mapping(manifest, "execution_policy", "backend")
    software_validation_mode(manifest)
    diagnostic = execution.get("diagnostic_engine", "gem5.verification")
    if diagnostic != "gem5.verification":
        raise ForgeContractError("diagnostic_engine must be gem5.verification")
    forbidden = execution.get("forbidden_engines")
    if not isinstance(forbidden, list) or "vemu" not in {
        str(item).lower() for item in forbidden
    }:
        raise ForgeContractError(
            "backend.execution_policy.forbidden_engines must include 'vemu'"
        )

    engines = _require_mapping(manifest, "engines", "backend")
    if any(str(name).lower() == "vemu" for name in engines):
        raise ForgeContractError("VEMU must not be declared as a backend engine")
    engine_text = "\n".join(_all_strings(engines)).lower()
    if "vemu" in engine_text or "debug/emulator" in engine_text or "make emulator" in engine_text:
        raise ForgeContractError("backend engines must not reference VEMU or Emulator")
    gem5 = _require_mapping(engines, "gem5", "backend.engines")
    for profile in ("fast", "verification"):
        _require_mapping(gem5, profile, "backend.engines.gem5")
    rtl = _require_mapping(engines, "rtl", "backend.engines")
    if rtl.get("source_immutable") is not True:
        raise ForgeContractError("backend.engines.rtl.source_immutable must be true")
    scheduler = _require_mapping(engines, "scheduler", "backend.engines")
    _require_string(scheduler, "build_target", "backend.engines.scheduler")
    linker_script = scheduler.get("linker_script")
    if linker_script is not None and (
            not isinstance(linker_script, str) or not linker_script):
        raise ForgeContractError(
            "backend.engines.scheduler.linker_script must be a non-empty "
            "string when present"
        )
    configure_dcache_end = scheduler.get("configure_dcache_end", True)
    if not isinstance(configure_dcache_end, bool):
        raise ForgeContractError(
            "backend.engines.scheduler.configure_dcache_end must be a "
            "boolean"
        )

    clock_contracts = manifest.get("clock_contracts")
    if clock_contracts is not None:
        if not isinstance(clock_contracts, dict):
            raise ForgeContractError("backend.clock_contracts must be an object")
        active_clock = clock_contracts.get("active")
        if active_clock is not None:
            if not isinstance(active_clock, str) or not active_clock:
                raise ForgeContractError(
                    "backend.clock_contracts.active must be a non-empty string"
                )
            active_contract = _require_mapping(
                clock_contracts, active_clock, "backend.clock_contracts"
            )
            _positive_int(
                active_contract, "system_axi_hz",
                f"backend.clock_contracts.{active_clock}",
            )
            _positive_int(
                active_contract, "tile_hz",
                f"backend.clock_contracts.{active_clock}",
            )
            gem5_clock_profile = _require_string(
                active_contract, "gem5_venus_config",
                f"backend.clock_contracts.{active_clock}",
            )
            for profile in ("fast", "verification"):
                engine_profile = _require_mapping(
                    gem5, profile, "backend.engines.gem5"
                )
                if engine_profile.get("venus_config") != gem5_clock_profile:
                    raise ForgeContractError(
                        f"backend.engines.gem5.{profile}.venus_config must "
                        "match the active clock contract"
                    )
            clock_firmware = _require_string(
                active_contract, "firmware",
                f"backend.clock_contracts.{active_clock}",
            )
            scheduler_firmware = scheduler.get("devctrl_init_body")
            if (not isinstance(scheduler_firmware, str)
                    or Path(scheduler_firmware).name
                    != Path(clock_firmware).name):
                raise ForgeContractError(
                    "backend.engines.scheduler.devctrl_init_body must match "
                    "the active clock-contract firmware"
                )
            resolved_clock_firmware = _resolve_declared_path(
                clock_firmware, path
            )
            if not resolved_clock_firmware.is_file():
                raise ForgeContractError(
                    "active clock-contract firmware does not exist: "
                    f"{resolved_clock_firmware}"
                )
    rtl_flow = _require_mapping(rtl, "flow", "backend.engines.rtl")
    for key in (
        "fresh_build_target", "replay_target", "runtime_case_variable",
        "unique_build_variable",
    ):
        _require_string(rtl_flow, key, "backend.engines.rtl.flow")
    for key in (
        "firmware_destinations", "snapshot_excludes", "evidence_paths",
    ):
        items = rtl_flow.get(key)
        if not isinstance(items, list) or not items or not all(
            isinstance(item, str) and item for item in items
        ):
            raise ForgeContractError(
                f"backend.engines.rtl.flow.{key} must be a non-empty string list"
            )
    make_variables = _require_mapping(
        rtl_flow, "make_variables", "backend.engines.rtl.flow"
    )
    if not make_variables or not all(
        isinstance(key, str) and key
        and isinstance(value, str) and value
        for key, value in make_variables.items()
    ):
        raise ForgeContractError(
            "backend.engines.rtl.flow.make_variables must contain string pairs"
        )
    required_environment = _require_mapping(
        rtl_flow, "required_environment", "backend.engines.rtl.flow"
    )
    if not required_environment:
        raise ForgeContractError(
            "backend.engines.rtl.flow.required_environment cannot be empty"
        )

    paths = _require_mapping(manifest, "paths", "backend")
    for key in ("dsl_root", "workload_root", "gem5_root", "rtl_root"):
        _require_string(paths, key, "backend.paths")

    return manifest


def validate_reference_adapter(path: Path) -> dict[str, Any]:
    path = path.expanduser().resolve()
    adapter = _load_object(path)
    if adapter.get("schema") != REFERENCE_SCHEMA:
        raise ForgeContractError(f"reference schema must be {REFERENCE_SCHEMA}")
    _require_string(adapter, "adapter_id", "reference")
    commands = _require_mapping(adapter, "commands", "reference")
    for stage in ("prepare", "build", "run", "extract"):
        argv = commands.get(stage)
        if not isinstance(argv, list) or not argv or not all(
            isinstance(item, str) and item for item in argv
        ):
            raise ForgeContractError(
                f"reference.commands.{stage} must be a non-empty argv array"
            )
    _require_mapping(adapter, "input_schema", "reference")
    _require_mapping(adapter, "output_schema", "reference")
    _require_mapping(adapter, "comparison", "reference")
    return adapter


def validate_forge_request(path: Path, backend_override: Path | None = None) -> tuple[dict[str, Any], dict[str, Any]]:
    path = path.expanduser().resolve()
    request = _load_object(path)
    if request.get("schema") != REQUEST_SCHEMA:
        raise ForgeContractError(f"request schema must be {REQUEST_SCHEMA}")
    _require_string(request, "intent", "request")

    source = _require_mapping(request, "source", "request")
    source_kind = _require_string(source, "kind", "request.source")
    if source_kind not in SOURCE_KINDS:
        raise ForgeContractError(f"unsupported request.source.kind: {source_kind}")

    workload = _require_mapping(request, "workload", "request")
    workload_kind = _require_string(workload, "kind", "request.workload")
    if workload_kind not in WORKLOAD_KINDS:
        raise ForgeContractError(f"unsupported request.workload.kind: {workload_kind}")
    _require_string(workload, "name", "request.workload")
    scopes = workload.get("required_correctness_scopes")
    if not isinstance(scopes, list) or not scopes or not all(
        isinstance(item, str) and item for item in scopes
    ):
        raise ForgeContractError(
            "request.workload.required_correctness_scopes must be non-empty"
        )

    backend_text = _require_string(request, "backend_manifest", "request")
    if backend_override is not None:
        if backend_text != "@project":
            raise ForgeContractError("Project selection is active: use backend_manifest=@project, or explicit --config for legacy requests")
        backend_path = backend_override.resolve()
    elif backend_text == "@project":
        raise ForgeContractError("@project requires an active project hardware selection")
    else:
        backend_path = _resolve_declared_path(backend_text, path)
    backend = validate_backend_manifest(backend_path)

    reference = _require_mapping(request, "reference", "request")
    state = _require_string(reference, "state", "request.reference")
    if state not in REFERENCE_STATES:
        raise ForgeContractError(f"unsupported reference state: {state}")
    adapter_text = reference.get("adapter")
    if adapter_text is not None:
        if not isinstance(adapter_text, str) or not adapter_text:
            raise ForgeContractError("request.reference.adapter must be a path string")
        validate_reference_adapter(_resolve_declared_path(adapter_text, path))
    if state == "AUTHORITATIVE_GOLDEN":
        approval = _require_mapping(reference, "approval", "request.reference")
        if approval.get("reviewer") != "human" or approval.get("decision") != "accept":
            raise ForgeContractError(
                "authoritative golden requires human accept approval"
            )
        _require_string(approval, "golden_digest", "request.reference.approval")

    optimization = _require_mapping(request, "optimization", "request")
    max_candidates = _positive_int(
        optimization, "max_candidates", "request.optimization"
    )
    plateau = _positive_int(
        optimization, "plateau_candidates", "request.optimization"
    )
    if plateau > max_candidates:
        raise ForgeContractError(
            "optimization.plateau_candidates cannot exceed max_candidates"
        )
    _require_string(optimization, "primary_metric", "request.optimization")

    cleanup = _require_mapping(request, "cleanup", "request")
    if cleanup.get("after_application_complete") is not True:
        raise ForgeContractError(
            "request.cleanup.after_application_complete must be true"
        )
    retain_top = _positive_int(cleanup, "retain_top_correct", "request.cleanup")
    if retain_top < 3:
        raise ForgeContractError("cleanup.retain_top_correct must be at least 3")

    validation = _require_mapping(request, "validation", "request")
    if validation.get("vemu_forbidden") is not True:
        raise ForgeContractError("request.validation.vemu_forbidden must be true")
    if validation.get("rtl_source_immutable") is not True:
        raise ForgeContractError("request.validation.rtl_source_immutable must be true")
    if validation.get("compare_all_visible_outputs") is not True:
        raise ForgeContractError(
            "request.validation.compare_all_visible_outputs must be true"
        )

    request["_resolved_backend_manifest"] = str(backend_path)
    return request, backend


@dataclass(frozen=True)
class ForgeRun:
    schema: str
    run_id: str
    created_at: str
    status: str
    request_path: str
    request_digest: str
    backend_path: str
    backend_digest: str
    backend_id: str
    golden_gate: str
    vemu_forbidden: bool
    full_software_engine: str
    diagnostic_engine: str


def prepare_forge_run(
    request_path: Path,
    run_root: Path,
    explicit: Path | None = None,
    backend_override: Path | None = None,
) -> Path:
    request_path = request_path.expanduser().resolve()
    request, backend = validate_forge_request(request_path, backend_override=backend_override)
    now = datetime.now()
    run_id = (
        f"{now.strftime('%Y%m%d-%H%M%S')}-forge-"
        f"{safe_name(request['workload']['name'])}"
    )
    run_dir = (explicit or run_root / run_id).expanduser().resolve()
    if run_dir.exists() and any(run_dir.iterdir()):
        raise ForgeContractError(f"run directory is not empty: {run_dir}")
    run_dir.mkdir(parents=True, exist_ok=True)
    for name in ("candidates", "stages", "knowledge-proposals"):
        (run_dir / name).mkdir()

    state = request["reference"]["state"]
    if state == "AUTHORITATIVE_GOLDEN":
        golden_gate = "PASS"
    elif state == "SOURCE_IMPLEMENTATION":
        golden_gate = "PENDING_REFERENCE_EXECUTION"
    elif state == "PROVISIONAL_GOLDEN":
        golden_gate = "REVIEW_REQUIRED"
    else:
        golden_gate = "PENDING_REFERENCE_CREATION"
    record = ForgeRun(
        schema=RUN_SCHEMA,
        run_id=run_id,
        created_at=datetime.now(timezone.utc).isoformat(),
        status="READY",
        request_path=str(request_path),
        request_digest=canonical_digest({
            key: value for key, value in request.items() if not key.startswith("_")
        }),
        backend_path=request["_resolved_backend_manifest"],
        backend_digest=canonical_digest(backend),
        backend_id=backend["backend_id"],
        golden_gate=golden_gate,
        vemu_forbidden=True,
        full_software_engine="gem5." + software_validation_mode(backend),
        diagnostic_engine="gem5.verification",
    )
    (run_dir / "forge-run.json").write_text(
        json.dumps(asdict(record), indent=2) + "\n", encoding="utf-8"
    )
    return run_dir


def _contained(child: Path, parent: Path) -> bool:
    try:
        child.relative_to(parent)
        return True
    except ValueError:
        return False


def prune_completed_run(run_dir: Path, keep_all: bool = False) -> dict[str, Any]:
    run_dir = run_dir.expanduser().resolve()
    manifest_path = run_dir / "forge-run.json"
    final_path = run_dir / "final-report.json"
    manifest = _load_object(manifest_path)
    final = _load_object(final_path)
    if manifest.get("schema") != RUN_SCHEMA:
        raise ForgeContractError(f"run schema must be {RUN_SCHEMA}")
    if final.get("schema") != FINAL_SCHEMA:
        raise ForgeContractError(f"final report schema must be {FINAL_SCHEMA}")
    if final.get("status") != "PASS" or final.get("application_complete") is not True:
        raise ForgeContractError(
            "candidate cleanup requires a PASS, application-complete final report"
        )

    candidates_root = (run_dir / "candidates").resolve()
    if not _contained(candidates_root, run_dir) or candidates_root.is_symlink():
        raise ForgeContractError("unsafe candidates directory")
    records: list[tuple[Path, dict[str, Any]]] = []
    if candidates_root.exists():
        for candidate_dir in sorted(candidates_root.iterdir()):
            if not candidate_dir.is_dir() or candidate_dir.is_symlink():
                continue
            metadata_path = candidate_dir / "candidate.json"
            if not metadata_path.is_file():
                raise ForgeContractError(
                    f"candidate is missing candidate.json: {candidate_dir}"
                )
            metadata = _load_object(metadata_path)
            if metadata.get("candidate_id") != candidate_dir.name:
                raise ForgeContractError(
                    f"candidate id/path mismatch: {candidate_dir.name}"
                )
            records.append((candidate_dir, metadata))

    keep: set[str] = set()
    for _, metadata in records:
        if metadata.get("role") in {"baseline", "winner"}:
            keep.add(metadata["candidate_id"])
        if metadata.get("rtl_qualified") is True:
            keep.add(metadata["candidate_id"])

    retain_top = int(final.get("retain_top_correct", 3))
    ranked = [
        metadata for _, metadata in records
        if metadata.get("correct") is True
        and metadata.get("fitness") == "RANKED"
        and isinstance(metadata.get("score"), (int, float))
    ]
    direction = final.get("metric_direction", "minimize")
    ranked.sort(
        key=lambda item: (item["score"], item["candidate_id"]),
        reverse=direction == "maximize",
    )
    keep.update(item["candidate_id"] for item in ranked[:retain_top])

    if keep_all:
        keep.update(metadata["candidate_id"] for _, metadata in records)

    removed: list[str] = []
    if not keep_all:
        for candidate_dir, metadata in records:
            if metadata["candidate_id"] in keep:
                continue
            for entry_name in sorted(HEAVY_CANDIDATE_ENTRIES):
                raw_entry = candidate_dir / entry_name
                if raw_entry.is_symlink():
                    continue
                entry = raw_entry.resolve()
                if not _contained(entry, candidate_dir.resolve()):
                    continue
                if entry.is_dir():
                    shutil.rmtree(entry)
                    removed.append(str(entry.relative_to(run_dir)))
                elif entry.is_file():
                    entry.unlink()
                    removed.append(str(entry.relative_to(run_dir)))

    report = {
        "schema": "ace-echo-retention-report/v1",
        "run_id": manifest["run_id"],
        "keep_all": keep_all,
        "kept_candidate_ids": sorted(keep),
        "removed_entries": removed,
        "retained_metadata_for_all_candidates": True,
    }
    (run_dir / "retention-report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    return report
