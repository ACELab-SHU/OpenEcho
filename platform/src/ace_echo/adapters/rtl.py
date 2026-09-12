from __future__ import annotations

from dataclasses import dataclass
import fnmatch
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import time
import math
from typing import Any

from ..artifacts import git_identity
from ..backend import BackendProfile
from ..process import Runner
from ..rtl_startup_diagnostics import classify_startup_errors
from ..dag_output_compare import compare_dag_outputs


@dataclass(frozen=True)
class RtlCase:
    name: str
    l1_bin: Path


class RtlAdapter:
    """Run a backend-declared RTL flow without editing its source checkout.

    RTL is copied to a run-local snapshot, generated dependencies declared by
    the backend are materialized there, and only then is firmware staged. A
    multi-case run compiles once and uses the replay target for later cases.
    """

    def __init__(self, runner: Runner, backend: BackendProfile):
        self.runner = runner
        self.backend = backend
        self.rtl = backend.manifest["engines"]["rtl"]
        self.flow = self.rtl["flow"]

    @staticmethod
    def _sha256(path: Path) -> str:
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
        return digest.hexdigest()

    @staticmethod
    def _validate_case_name(name: str) -> None:
        candidate = Path(name)
        if (not name or candidate.is_absolute() or ".." in candidate.parts
                or any(part in ("", ".") for part in candidate.parts)):
            raise ValueError(f"unsafe RTL case name: {name!r}")

    def _validate_source_identity(self) -> dict[str, object]:
        actual = git_identity(self.backend.rtl_root, "rtl")
        required = self.backend.manifest.get("repositories", {}).get(
            "rtl", {}
        )
        expected_commit = required.get("required_commit")
        expected_branch = required.get("required_branch")
        if expected_commit and actual.get("head") != expected_commit:
            raise ValueError(
                "RTL commit mismatch: expected "
                f"{expected_commit}, got {actual.get('head')}"
            )
        if expected_branch and actual.get("branch") != expected_branch:
            raise ValueError(
                "RTL branch mismatch: expected "
                f"{expected_branch}, got {actual.get('branch')}"
            )
        if required.get("required_clean") and actual.get("dirty"):
            raise ValueError("RTL source checkout is dirty")
        return actual

    def _copy_snapshot(self, destination: Path) -> None:
        excludes = list(self.flow.get("snapshot_excludes", []))
        shutil.copytree(
            self.backend.rtl_root,
            destination,
            symlinks=True,
            ignore=self._snapshot_ignore(
                self.backend.rtl_root, excludes
            ),
        )
        dependencies = self.flow.get("snapshot_dependencies", {})
        for overlay in dependencies.get("copy_overlays", []):
            source = Path(overlay["source"]).expanduser().resolve()
            target = destination / overlay["destination"]
            if not source.is_dir():
                raise FileNotFoundError(
                    f"RTL snapshot dependency is missing: {source}"
                )
            leaf_subdirectories = overlay.get("leaf_subdirectories")
            if leaf_subdirectories:
                self._copy_overlay_leaf_subdirectories(
                    source, target, list(leaf_subdirectories)
                )
            else:
                shutil.copytree(
                    source, target, dirs_exist_ok=True, symlinks=True
                )
        self._adapt_snapshot_metadata(destination, dependencies)

    @staticmethod
    def _copy_overlay_leaf_subdirectories(
            source: Path, target: Path, leaf_names: list[str]) -> None:
        """Copy only declared leaves below each direct dependency directory.

        Generated IP caches often contain hundreds of megabytes of synthesis
        workspaces although RTL simulation consumes only a small ``src`` leaf.
        The selection is manifest-declared and fails closed when no leaf is
        found, so a typo cannot silently create an incomplete fresh snapshot.
        """
        invalid = [
            name for name in leaf_names
            if not name or Path(name).is_absolute() or ".." in Path(name).parts
        ]
        if invalid:
            raise ValueError(
                f"unsafe overlay leaf subdirectories: {invalid!r}"
            )
        copied = 0
        for child in sorted(source.iterdir()):
            if not child.is_dir():
                continue
            for leaf_name in leaf_names:
                leaf = child / leaf_name
                if not leaf.is_dir():
                    continue
                shutil.copytree(
                    leaf,
                    target / child.name / leaf_name,
                    dirs_exist_ok=True,
                    symlinks=True,
                )
                copied += 1
        if not copied:
            raise FileNotFoundError(
                f"overlay {source} contains none of the declared leaf "
                f"subdirectories: {leaf_names!r}"
            )

    @staticmethod
    def _snapshot_ignore(source_root: Path, patterns: list[str]):
        """Match snapshot exclusions against paths relative to the RTL root.

        ``shutil.ignore_patterns`` only sees basenames.  Consequently a
        manifest entry such as ``sim/build_*`` never matches and can leak a
        previous simulator work library into a supposedly fresh RTL build.
        Match both the relative path and basename so manifests may express
        scoped exclusions while retaining the conventional ``.git`` form.
        """
        root = source_root.resolve()

        def ignore(directory: str, names: list[str]) -> set[str]:
            parent = Path(directory).resolve().relative_to(root)
            ignored: set[str] = set()
            for name in names:
                relative = (parent / name).as_posix()
                if any(
                    fnmatch.fnmatchcase(relative, pattern)
                    or fnmatch.fnmatchcase(name, pattern)
                    for pattern in patterns
                ):
                    ignored.add(name)
            return ignored

        return ignore

    @staticmethod
    def _write_snapshot_metadata(snapshot: Path, path: Path, text: str) -> None:
        """Relocate copied, possibly read-only metadata without touching sources."""
        resolved = path.resolve()
        try:
            resolved.relative_to(snapshot.resolve())
        except ValueError as error:
            raise ValueError(f"metadata path escapes RTL snapshot: {path}") from error
        if path.is_symlink() or resolved.suffix.lower() in {".v", ".vh", ".sv", ".svh"}:
            raise ValueError(f"refusing RTL source or symlink metadata edit: {path}")
        info = path.stat()
        if info.st_nlink != 1:
            raise ValueError(f"refusing shared-inode metadata edit: {path}")
        mode = stat.S_IMODE(info.st_mode)
        try:
            path.chmod(mode | stat.S_IWUSR)
            path.write_text(text, encoding="utf-8")
        finally:
            path.chmod(mode)

    @staticmethod
    def _adapt_snapshot_metadata(snapshot: Path,
                                 dependencies: dict[str, Any]) -> None:
        """Apply only manifest-declared build-metadata compatibility edits.

        These edits are confined to the disposable snapshot. They may rebase
        generated file lists or remove a tool-version-incompatible automation
        directive, but cannot silently patch RTL source files.
        """
        for edit in dependencies.get("text_edits", []):
            path = snapshot / edit["path"]
            text = path.read_text(encoding="utf-8")
            for line in edit.get("remove_exact_lines", []):
                matches = sum(
                    candidate.strip() == line.strip()
                    for candidate in text.splitlines()
                )
                if matches != 1:
                    raise ValueError(
                        f"snapshot edit expected one match in {path}: {line!r}; "
                        f"found {matches}"
                    )
                text = "\n".join(
                    candidate for candidate in text.splitlines()
                    if candidate.strip() != line.strip()
                ) + "\n"
            RtlAdapter._write_snapshot_metadata(snapshot, path, text)

        for rewrite in dependencies.get("generated_path_rewrites", []):
            old = str(rewrite["old"])
            new = str(rewrite["new"]).format(snapshot=str(snapshot))
            matched_files = 0
            replacements = 0
            for path in sorted(snapshot.glob(rewrite["glob"])):
                if not path.is_file():
                    continue
                text = path.read_text(encoding="utf-8", errors="strict")
                count = text.count(old)
                if count:
                    RtlAdapter._write_snapshot_metadata(
                        snapshot, path, text.replace(old, new)
                    )
                    matched_files += 1
                    replacements += count
            if rewrite.get("required", True) and not replacements:
                raise ValueError(
                    f"generated path rewrite matched nothing: {old!r} in "
                    f"{rewrite['glob']!r}"
                )
            if replacements and not matched_files:
                raise AssertionError("path rewrite replacement accounting failed")

    def _environment(self) -> dict[str, str]:
        environment = {
            str(key): str(value)
            for key, value in self.flow.get("required_environment", {}).items()
        }
        prepend = [str(Path(item).expanduser())
                   for item in self.flow.get("path_prepend", [])]
        if prepend:
            environment["PATH"] = os.pathsep.join(
                prepend + [os.environ.get("PATH", "")]
            )
        return environment

    @staticmethod
    def _make_args(values: dict[str, Any]) -> list[str]:
        return [f"{key}={value}" for key, value in sorted(values.items())]

    def _stage_firmware(self, snapshot: Path, case: RtlCase,
                        *, stage_scheduler: bool) -> None:
        for template in self.flow["firmware_destinations"]:
            if not stage_scheduler and "{case_name}" not in template:
                continue
            target = snapshot / template.format(case_name=case.name)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(case.l1_bin, target)

    def _build_directory(self, snapshot: Path, copy_id: str) -> Path:
        variables = self.flow["make_variables"]
        return snapshot / "sim" / (
            f"build_{variables['SIM_NAME']}{variables['SIM_TYPE']}_{copy_id}"
        )

    @staticmethod
    def _set_sim_horizon(snapshot: Path, horizon_us: int) -> None:
        """Shorten or extend only the run-local UCLI diagnostic horizon."""
        if horizon_us <= 0:
            raise ValueError("RTL simulation horizon must be positive")
        path = snapshot / "sim/simv_ucli.tcl"
        text = path.read_text(encoding="utf-8")
        lines = text.splitlines()
        matches = [
            index for index, line in enumerate(lines)
            if line.strip().startswith("run ") and line.strip().endswith("us")
        ]
        if len(matches) != 1:
            raise ValueError(
                f"expected one run horizon in {path}; found {len(matches)}"
            )
        lines[matches[0]] = f"run {horizon_us}us"
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    def _configure_task_boundary_trace(self, snapshot: Path) -> str:
        """Enable a manifest-declared, run-local RTL task-boundary waveform.

        The observer changes only the disposable UCLI script.  It neither
        patches RTL nor adds design-specific timing.  Debug-capable build
        targets and the observed signals are backend contract data because
        their availability and hierarchy are RTL-generation specific.
        """
        observer = self.flow.get("task_boundary_trace")
        if not isinstance(observer, dict):
            raise ValueError(
                "backend does not declare an RTL task-boundary trace"
            )
        output_name = str(observer.get("output", ""))
        for label, value in (("trace output", output_name),):
            candidate = Path(value)
            if (not value or candidate.is_absolute()
                    or ".." in candidate.parts):
                raise ValueError(f"unsafe task-boundary {label}: {value!r}")
        mode = str(observer.get("mode", "ucli_vpd"))
        if mode == "testbench_edge_log":
            source_name = str(observer.get("testbench_source", ""))
            source_candidate = Path(source_name)
            if (not source_name or source_candidate.is_absolute()
                    or ".." in source_candidate.parts):
                raise ValueError(
                    f"unsafe task-boundary testbench source: {source_name!r}"
                )
            tiles = observer.get("tiles")
            if not isinstance(tiles, list) or not tiles:
                raise ValueError("task-boundary edge log requires tiles")
            signal_pattern = re.compile(r"^[A-Za-z_$][A-Za-z0-9_$.[\]:]*$")
            identifier_pattern = re.compile(
                r"^[A-Za-z_$][A-Za-z0-9_$]*$"
            )
            normalized: list[dict[str, Any]] = []
            seen_tiles: set[int] = set()
            for tile in tiles:
                if not isinstance(tile, dict):
                    raise ValueError("task-boundary tile entries must be maps")
                tile_id = tile.get("tile")
                reset = tile.get("reset_signal")
                task = tile.get("task_signal")
                sequencer = tile.get("sequencer_running_signal")
                sequencer_module = tile.get("sequencer_module")
                sequencer_port = tile.get("sequencer_running_port")
                running_ids = tile.get("sequencer_running_ids", 0)
                if (not isinstance(tile_id, int) or tile_id < 0
                        or tile_id in seen_tiles):
                    raise ValueError(
                        f"invalid task-boundary tile id: {tile_id!r}"
                    )
                if (not isinstance(reset, str)
                        or signal_pattern.fullmatch(reset) is None
                        or not isinstance(task, str)
                        or signal_pattern.fullmatch(task) is None):
                    raise ValueError(
                        f"unsafe task-boundary tile signals: {tile!r}"
                    )
                if sequencer is not None and (
                        not isinstance(sequencer, str)
                        or signal_pattern.fullmatch(sequencer) is None):
                    raise ValueError(
                        f"unsafe sequencer running signal: {sequencer!r}"
                    )
                for label, value in (
                        ("sequencer module", sequencer_module),
                        ("sequencer running port", sequencer_port)):
                    if value is not None and (
                            not isinstance(value, str)
                            or identifier_pattern.fullmatch(value) is None):
                        raise ValueError(f"unsafe {label}: {value!r}")
                bind_declared = (
                    sequencer_module is not None or sequencer_port is not None
                )
                if bind_declared and (
                        sequencer_module is None or sequencer_port is None):
                    raise ValueError(
                        "sequencer module and running port must be declared "
                        f"together: {tile!r}"
                    )
                if sequencer is not None and bind_declared:
                    raise ValueError(
                        "declare either a sequencer XMR or a module bind, "
                        f"not both: {tile!r}"
                    )
                if (not isinstance(running_ids, int) or running_ids < 0
                        or running_ids > 64
                        or ((sequencer is None and not bind_declared)
                            != (running_ids == 0))):
                    raise ValueError(
                        "sequencer running signal and a positive ID count "
                        f"must be declared together: {tile!r}"
                    )
                seen_tiles.add(tile_id)
                normalized.append({
                    "tile": tile_id,
                    "reset": reset,
                    "task": task,
                    "sequencer": sequencer,
                    "sequencer_module": sequencer_module,
                    "sequencer_port": sequencer_port,
                    "running_ids": running_ids,
                })

            shuffle_observers = observer.get(
                "shuffle_phase_observers", []
            )
            if not isinstance(shuffle_observers, list):
                raise ValueError(
                    "shuffle phase observers must be a list"
                )
            normalized_shuffle: list[dict[str, Any]] = []
            for entry in shuffle_observers:
                if not isinstance(entry, dict):
                    raise ValueError(
                        "shuffle phase observer entries must be maps"
                    )
                module = entry.get("module")
                lane_count = entry.get("lane_count")
                pe_count = entry.get("pe_count")
                id_width = entry.get("id_width")
                vl_width = entry.get("vl_width")
                state_width = entry.get("state_width", 4)
                vew_width = entry.get("vew_width", 2)
                if (not isinstance(module, str)
                        or identifier_pattern.fullmatch(module) is None):
                    raise ValueError(
                        f"unsafe shuffle observer module: {module!r}"
                    )
                for label, value, limit in (
                        ("lane count", lane_count, 256),
                        ("PE count", pe_count, 256),
                        ("ID width", id_width, 32),
                        ("VL width", vl_width, 32),
                        ("state width", state_width, 8),
                        ("VEW width", vew_width, 8)):
                    if (not isinstance(value, int) or value <= 0
                            or value > limit):
                        raise ValueError(
                            f"invalid shuffle observer {label}: {value!r}"
                        )
                signals: dict[str, str] = {}
                for field in (
                        "clock_signal", "reset_signal", "state_signal",
                        "done_signal", "grant_signal", "id_signal",
                        "vm_r_signal", "vew_signal", "vl_signal"):
                    value = entry.get(field)
                    if (not isinstance(value, str)
                            or signal_pattern.fullmatch(value) is None):
                        raise ValueError(
                            f"unsafe shuffle observer {field}: {value!r}"
                        )
                    signals[field] = value
                normalized_shuffle.append({
                    "module": module,
                    "lane_count": lane_count,
                    "pe_count": pe_count,
                    "id_width": id_width,
                    "vl_width": vl_width,
                    "state_width": state_width,
                    "vew_width": vew_width,
                    **signals,
                })

            lane_resource_observers = observer.get(
                "lane_resource_observers", []
            )
            if not isinstance(lane_resource_observers, list):
                raise ValueError(
                    "lane resource observers must be a list"
                )
            normalized_lane_resources: list[dict[str, Any]] = []
            lane_signal_fields = (
                "clock_signal", "reset_signal", "lane_id_signal",
                "pe_valid_signal", "pe_ready_signal",
                "operand_valid_signal", "operand_ready_signal",
                "operand_queue_valid_signal",
                "operand_queue_ready_signal",
                "mask_valid_signal", "mask_ready_signal",
                "vrf_req_signal", "vrf_wen_signal",
                "vrf_arb_req_signal", "vrf_arb_gnt_signal",
                "hazard_vs1_signal", "hazard_vs2_signal",
                "hazard_vd1_signal", "hazard_vd2_signal",
                "vfu_valid_signal", "vfu_id_signal", "vfu_op_signal",
                "vm_r_signal", "vm_w_signal",
                "bitalu_ready_signal", "cau_ready_signal",
                "serdiv_ready_signal", "bitalu_grant_signal",
                "cau_grant_signal", "serdiv_grant_signal",
            )
            for entry in lane_resource_observers:
                if not isinstance(entry, dict):
                    raise ValueError(
                        "lane resource observer entries must be maps"
                    )
                module = entry.get("module")
                if (not isinstance(module, str)
                        or identifier_pattern.fullmatch(module) is None):
                    raise ValueError(
                        f"unsafe lane resource observer module: {module!r}"
                    )
                dimensions: dict[str, int] = {}
                for field, label, limit in (
                        ("lane_id_width", "lane ID width", 32),
                        ("operand_width", "operand width", 64),
                        ("hazard_width", "hazard width", 64),
                        ("bank_width", "bank width", 64),
                        ("vrf_arb_width", "VRF arbiter width", 256),
                        ("id_width", "instruction ID width", 32),
                        ("op_width", "opcode width", 32)):
                    value = entry.get(field)
                    if (not isinstance(value, int) or value <= 0
                            or value > limit):
                        raise ValueError(
                            f"invalid lane resource observer {label}: "
                            f"{value!r}"
                        )
                    dimensions[field] = value
                observed_lane = entry.get("observed_lane")
                if (not isinstance(observed_lane, int)
                        or observed_lane < 0
                        or observed_lane >= (1 << dimensions["lane_id_width"])):
                    raise ValueError(
                        "invalid lane resource observer lane: "
                        f"{observed_lane!r}"
                    )
                signals: dict[str, str] = {}
                for field in lane_signal_fields:
                    value = entry.get(field)
                    if (not isinstance(value, str)
                            or signal_pattern.fullmatch(value) is None):
                        raise ValueError(
                            f"unsafe lane resource observer {field}: "
                            f"{value!r}"
                        )
                    signals[field] = value
                normalized_lane_resources.append({
                    "module": module,
                    "observed_lane": observed_lane,
                    **dimensions,
                    **signals,
                })

            sequencer_ready_observers = observer.get(
                "sequencer_ready_observers", []
            )
            if not isinstance(sequencer_ready_observers, list):
                raise ValueError(
                    "sequencer ready observers must be a list"
                )
            normalized_sequencer_ready: list[dict[str, Any]] = []
            sequencer_signal_fields = (
                "clock_signal", "reset_signal", "ready_vector_signal",
                "pe_valid_signal", "state_signal", "op_signal",
                "upstream_valid_signal", "upstream_ready_signal",
                "running_full_signal", "lane_desync_signal",
                "queue_ready_signal", "queue_done_signal",
                "queue_counts_signal", "target_vfus_signal",
                "accepted_signal",
            )
            for entry in sequencer_ready_observers:
                if not isinstance(entry, dict):
                    raise ValueError(
                        "sequencer ready observer entries must be maps"
                    )
                module = entry.get("module")
                if (not isinstance(module, str)
                        or identifier_pattern.fullmatch(module) is None):
                    raise ValueError(
                        f"unsafe sequencer ready observer module: {module!r}"
                    )
                dimensions: dict[str, int] = {}
                for field, label, limit in (
                        ("ready_width", "ready width", 256),
                        ("state_width", "state width", 32),
                        ("op_width", "opcode width", 32),
                        ("queue_width", "VFU queue width", 32),
                        ("queue_counts_width", "VFU queue counts width", 128)):
                    value = entry.get(field)
                    if (not isinstance(value, int) or value <= 0
                            or value > limit):
                        raise ValueError(
                            f"invalid sequencer ready observer {label}: "
                            f"{value!r}"
                        )
                    dimensions[field] = value
                signals: dict[str, str] = {}
                for field in sequencer_signal_fields:
                    value = entry.get(field)
                    if (not isinstance(value, str)
                            or signal_pattern.fullmatch(value) is None):
                        raise ValueError(
                            f"unsafe sequencer ready observer {field}: "
                            f"{value!r}"
                        )
                    signals[field] = value
                normalized_sequencer_ready.append({
                    "module": module,
                    **dimensions,
                    **signals,
                })

            scalar_retire_observers = observer.get(
                "scalar_retire_observers", []
            )
            if not isinstance(scalar_retire_observers, list):
                raise ValueError(
                    "scalar retire observers must be a list"
                )
            normalized_scalar_retires: list[dict[str, Any]] = []
            for entry in scalar_retire_observers:
                if not isinstance(entry, dict):
                    raise ValueError(
                        "scalar retire observer entries must be maps"
                    )
                module = entry.get("module")
                if (not isinstance(module, str)
                        or identifier_pattern.fullmatch(module) is None):
                    raise ValueError(
                        f"unsafe scalar retire observer module: {module!r}"
                    )
                signals: dict[str, str] = {}
                for field in (
                        "clock_signal", "reset_signal", "valid_signal",
                        "pc_signal", "instruction_signal"):
                    value = entry.get(field)
                    if (not isinstance(value, str)
                            or signal_pattern.fullmatch(value) is None):
                        raise ValueError(
                            f"unsafe scalar retire observer {field}: "
                            f"{value!r}"
                        )
                    signals[field] = value
                normalized_scalar_retires.append({
                    "module": module,
                    **signals,
                })

            scalar_dispatch_observers = observer.get(
                "scalar_dispatch_observers", []
            )
            if not isinstance(scalar_dispatch_observers, list):
                raise ValueError(
                    "scalar dispatch observers must be a list"
                )
            normalized_scalar_dispatches: list[dict[str, Any]] = []
            scalar_dispatch_signal_fields = (
                "clock_signal", "reset_signal", "valid_signal",
                "ready_signal", "msb_instruction_signal",
                "lsb_instruction_signal", "id_instruction_signal",
                "vec_msb_load_signal", "vec_lsb_load_wait_signal",
                "vec_lsb_load_signal", "instr_vector_signal",
                "uncomplete_signal", "stall_signal",
                "vector_wait_signal",
            )
            for entry in scalar_dispatch_observers:
                if not isinstance(entry, dict):
                    raise ValueError(
                        "scalar dispatch observer entries must be maps"
                    )
                module = entry.get("module")
                if (not isinstance(module, str)
                        or identifier_pattern.fullmatch(module) is None):
                    raise ValueError(
                        f"unsafe scalar dispatch observer module: {module!r}"
                    )
                signals: dict[str, str] = {}
                for field in scalar_dispatch_signal_fields:
                    value = entry.get(field)
                    if (not isinstance(value, str)
                            or signal_pattern.fullmatch(value) is None):
                        raise ValueError(
                            f"unsafe scalar dispatch observer {field}: "
                            f"{value!r}"
                        )
                    signals[field] = value
                normalized_scalar_dispatches.append({
                    "module": module,
                    **signals,
                })

            path = snapshot / source_name
            text = path.read_text(encoding="utf-8")
            marker = "// ACE-ECHO task-boundary observer"
            if marker in text:
                raise ValueError(
                    f"task-boundary observer already present in {path}"
                )
            endmodules = [
                match.start() for match in
                re.finditer(r"(?m)^endmodule\s*$", text)
            ]
            if len(endmodules) != 1:
                raise ValueError(
                    f"expected one endmodule in {path}; found {len(endmodules)}"
                )
            code = [
                marker,
                "integer ace_echo_task_boundary_fd;",
                "initial begin",
                "  ace_echo_task_boundary_fd = $fopen("
                f"\"{output_name}\", \"w\");",
                "end",
            ]
            bind_code: list[str] = []
            for tile in normalized:
                tile_id = tile["tile"]
                reset = tile["reset"]
                task = tile["task"]
                sequencer = tile["sequencer"]
                running_ids = tile["running_ids"]
                code.extend([
                    f"always @(posedge {reset}) begin",
                    "  $fwrite(ace_echo_task_boundary_fd,",
                    f"    \"task %0d start execute at tile {tile_id} at time "
                    "%0t\\n\",",
                    f"    {task}, $time);",
                    "  $fflush(ace_echo_task_boundary_fd);",
                    "end",
                    f"always @(negedge {reset}) begin",
                    "  $fwrite(ace_echo_task_boundary_fd,",
                    f"    \"task %0d execute complete at tile {tile_id} at "
                    "time %0t\\n\",",
                    f"    {task}, $time);",
                    "  $fflush(ace_echo_task_boundary_fd);",
                    "end",
                ])
                for running_id in range(
                        running_ids if sequencer is not None else 0):
                    code.extend([
                        f"always @(posedge {sequencer}[{running_id}]) begin",
                        "  $fwrite(ace_echo_task_boundary_fd,",
                        f"    \"vins {running_id} start at task %0d tile "
                        f"{tile_id} at time %0t\\n\",",
                        f"    {task}, $time);",
                        "  $fflush(ace_echo_task_boundary_fd);",
                        "end",
                        f"always @(negedge {sequencer}[{running_id}]) begin",
                        "  $fwrite(ace_echo_task_boundary_fd,",
                        f"    \"vins {running_id} complete at task %0d tile "
                        f"{tile_id} at time %0t\\n\",",
                        f"    {task}, $time);",
                        "  $fflush(ace_echo_task_boundary_fd);",
                        "end",
                    ])
                sequencer_module = tile["sequencer_module"]
                if sequencer_module is not None:
                    observer_module = (
                        f"ace_echo_vins_observer_tile{tile_id}"
                    )
                    bind_code.extend([
                        f"module {observer_module} (",
                        f"  input logic [{running_ids - 1}:0] running_i",
                        ");",
                    ])
                    for running_id in range(running_ids):
                        bind_code.extend([
                            f"always @(posedge running_i[{running_id}])",
                            f"  $display(\"ACE_ECHO_VINS {running_id} start "
                            f"tile {tile_id} time %0t\", $time);",
                            f"always @(negedge running_i[{running_id}])",
                            f"  $display(\"ACE_ECHO_VINS {running_id} "
                            f"complete tile {tile_id} time %0t\", $time);",
                        ])
                    bind_code.extend([
                        "endmodule",
                        f"bind {sequencer_module} {observer_module} "
                        f"ace_echo_vins_observer_i (",
                        f"  .running_i({tile['sequencer_port']})",
                        ");",
                    ])
            for observer_index, shuffle in enumerate(normalized_shuffle):
                observer_module = (
                    f"ace_echo_shuffle_phase_observer_{observer_index}"
                )
                lanes = shuffle["lane_count"]
                pes = shuffle["pe_count"]
                state_width = shuffle["state_width"]
                id_width = shuffle["id_width"]
                vew_width = shuffle["vew_width"]
                vl_width = shuffle["vl_width"]
                bind_code.extend([
                    f"module {observer_module} (",
                    "  input logic clk_i,",
                    "  input logic rst_ni,",
                    f"  input logic [{state_width - 1}:0] state_i,",
                    "  input logic done_i,",
                    f"  input logic [{lanes - 1}:0][{pes - 1}:0] "
                    "grant_i,",
                    f"  input logic [{id_width - 1}:0] id_i,",
                    "  input logic vm_r_i,",
                    f"  input logic [{vew_width - 1}:0] vew_i,",
                    f"  input logic [{vl_width - 1}:0] vl_i",
                    ");",
                    "  integer active;",
                    "  integer lane;",
                    "  integer pe;",
                    "  integer grants;",
                    "  integer c0, c1, c2, c3, c4, c5, c6, c7, c8, "
                    "c9, c10;",
                    "  integer g0, g1, g2, g3, g4, g5, g6, g7, g8, "
                    "g9, g10;",
                    "  integer captured_id, captured_vm_r, captured_vew;",
                    "  integer captured_vl;",
                    "  time start_time;",
                    "  task automatic clear_counters;",
                    "    begin",
                    "      c0=0; c1=0; c2=0; c3=0; c4=0; c5=0;",
                    "      c6=0; c7=0; c8=0; c9=0; c10=0;",
                    "      g0=0; g1=0; g2=0; g3=0; g4=0; g5=0;",
                    "      g6=0; g7=0; g8=0; g9=0; g10=0;",
                    "    end",
                    "  endtask",
                    "  always @(posedge clk_i or negedge rst_ni) begin",
                    "    if (!rst_ni) begin",
                    "      active=0; captured_id=0; captured_vm_r=0;",
                    "      captured_vew=0; captured_vl=0; start_time=0;",
                    "      clear_counters();",
                    "    end else begin",
                    "      if (!active && state_i == 7) begin",
                    "        clear_counters();",
                    "        active=1; captured_id=id_i;",
                    "        captured_vm_r=vm_r_i; captured_vew=vew_i;",
                    "        captured_vl=vl_i; start_time=$time;",
                    "      end",
                    "      if (active || state_i == 7) begin",
                    "        grants=0;",
                    f"        for (lane=0; lane<{lanes}; lane=lane+1)",
                    f"          for (pe=0; pe<{pes}; pe=pe+1)",
                    "            if (grant_i[lane][pe]) grants=grants+1;",
                    "        case (state_i)",
                    "          0: begin c0=c0+1; g0=g0+grants; end",
                    "          1: begin c1=c1+1; g1=g1+grants; end",
                    "          2: begin c2=c2+1; g2=g2+grants; end",
                    "          3: begin c3=c3+1; g3=g3+grants; end",
                    "          4: begin c4=c4+1; g4=g4+grants; end",
                    "          5: begin c5=c5+1; g5=g5+grants; end",
                    "          6: begin c6=c6+1; g6=g6+grants; end",
                    "          7: begin c7=c7+1; g7=g7+grants; end",
                    "          8: begin c8=c8+1; g8=g8+grants; end",
                    "          9: begin c9=c9+1; g9=g9+grants; end",
                    "          10: begin c10=c10+1; g10=g10+grants; end",
                    "        endcase",
                    "      end",
                    "      if (active && done_i) begin",
                    "        $display(\"ACE_ECHO_SHUFFLE_PHASE id %0d vm_r "
                    "%0d vew %0d vl %0d start %0t complete %0t "
                    "cycles %0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d "
                    "grants %0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d,%0d\",",
                    "          captured_id, captured_vm_r, captured_vew,",
                    "          captured_vl, start_time, $time,",
                    "          c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,",
                    "          g0,g1,g2,g3,g4,g5,g6,g7,g8,g9,g10);",
                    "        active=0;",
                    "      end",
                    "    end",
                    "  end",
                    "endmodule",
                    f"bind {shuffle['module']} {observer_module} "
                    f"{observer_module}_i (",
                    f"  .clk_i({shuffle['clock_signal']}),",
                    f"  .rst_ni({shuffle['reset_signal']}),",
                    f"  .state_i({shuffle['state_signal']}),",
                    f"  .done_i({shuffle['done_signal']}),",
                    f"  .grant_i({shuffle['grant_signal']}),",
                    f"  .id_i({shuffle['id_signal']}),",
                    f"  .vm_r_i({shuffle['vm_r_signal']}),",
                    f"  .vew_i({shuffle['vew_signal']}),",
                    f"  .vl_i({shuffle['vl_signal']})",
                    ");",
                ])
            for observer_index, lane in enumerate(
                    normalized_lane_resources):
                observer_module = (
                    f"ace_echo_lane_resource_observer_{observer_index}"
                )
                lane_width = lane["lane_id_width"]
                operand_width = lane["operand_width"]
                hazard_width = lane["hazard_width"]
                bank_width = lane["bank_width"]
                vrf_arb_width = lane["vrf_arb_width"]
                id_width = lane["id_width"]
                op_width = lane["op_width"]
                bind_code.extend([
                    f"module {observer_module} (",
                    "  input logic clk_i,",
                    "  input logic rst_ni,",
                    f"  input logic [{lane_width - 1}:0] lane_id_i,",
                    "  input logic pe_valid_i, input logic pe_ready_i,",
                    f"  input logic [{operand_width - 1}:0] "
                    "operand_valid_i,",
                    f"  input logic [{operand_width - 1}:0] "
                    "operand_ready_i,",
                    f"  input logic [{operand_width - 1}:0] "
                    "operand_queue_valid_i,",
                    f"  input logic [{operand_width - 1}:0] "
                    "operand_queue_ready_i,",
                    "  input logic mask_valid_i, input logic mask_ready_i,",
                    f"  input logic [{bank_width - 1}:0] vrf_req_i,",
                    f"  input logic [{bank_width - 1}:0] vrf_wen_i,",
                    f"  input logic [{vrf_arb_width - 1}:0] "
                    "vrf_arb_req_i,",
                    f"  input logic [{vrf_arb_width - 1}:0] "
                    "vrf_arb_gnt_i,",
                    f"  input logic [{hazard_width - 1}:0] hazard_vs1_i,",
                    f"  input logic [{hazard_width - 1}:0] hazard_vs2_i,",
                    f"  input logic [{hazard_width - 1}:0] hazard_vd1_i,",
                    f"  input logic [{hazard_width - 1}:0] hazard_vd2_i,",
                    "  input logic vfu_valid_i,",
                    f"  input logic [{id_width - 1}:0] vfu_id_i,",
                    f"  input logic [{op_width - 1}:0] vfu_op_i,",
                    "  input logic vm_r_i, input logic vm_w_i,",
                    "  input logic bitalu_ready_i, input logic cau_ready_i,",
                    "  input logic serdiv_ready_i,",
                    "  input logic bitalu_grant_i, input logic cau_grant_i,",
                    "  input logic serdiv_grant_i",
                    ");",
                    f"  logic [{operand_width - 1}:0] prev_operand_valid;",
                    f"  logic [{operand_width - 1}:0] prev_operand_ready;",
                    f"  logic [{operand_width - 1}:0] "
                    "prev_operand_queue_valid;",
                    f"  logic [{operand_width - 1}:0] "
                    "prev_operand_queue_ready;",
                    f"  logic [{bank_width - 1}:0] prev_vrf_req;",
                    f"  logic [{bank_width - 1}:0] prev_vrf_wen;",
                    f"  logic [{vrf_arb_width - 1}:0] prev_vrf_arb_req;",
                    f"  logic [{vrf_arb_width - 1}:0] prev_vrf_arb_gnt;",
                    f"  logic [{hazard_width - 1}:0] prev_hazard_vs1;",
                    f"  logic [{hazard_width - 1}:0] prev_hazard_vs2;",
                    f"  logic [{hazard_width - 1}:0] prev_hazard_vd1;",
                    f"  logic [{hazard_width - 1}:0] prev_hazard_vd2;",
                    "  logic [11:0] prev_scalar;",
                    "  wire [11:0] scalar_state = {pe_valid_i, pe_ready_i,",
                    "    mask_valid_i, mask_ready_i, vfu_valid_i, vm_r_i,",
                    "    vm_w_i, bitalu_ready_i, cau_ready_i,",
                    "    serdiv_ready_i, bitalu_grant_i, cau_grant_i};",
                    "  always @(posedge clk_i or negedge rst_ni) begin",
                    "    if (!rst_ni) begin",
                    "      prev_operand_valid <= '0;",
                    "      prev_operand_ready <= '0;",
                    "      prev_operand_queue_valid <= '0;",
                    "      prev_operand_queue_ready <= '0;",
                    "      prev_vrf_req <= '0; prev_vrf_wen <= '0;",
                    "      prev_vrf_arb_req <= '0; prev_vrf_arb_gnt <= '0;",
                    "      prev_hazard_vs1 <= '0; prev_hazard_vs2 <= '0;",
                    "      prev_hazard_vd1 <= '0; prev_hazard_vd2 <= '0;",
                    "      prev_scalar <= '0;",
                    "    end else if (lane_id_i == "
                    f"{lane_width}'d{lane['observed_lane']}) begin",
                    "      if (operand_valid_i != prev_operand_valid ||",
                    "          operand_ready_i != prev_operand_ready ||",
                    "          operand_queue_valid_i != "
                    "prev_operand_queue_valid ||",
                    "          operand_queue_ready_i != "
                    "prev_operand_queue_ready ||",
                    "          vrf_req_i != prev_vrf_req ||",
                    "          vrf_wen_i != prev_vrf_wen ||",
                    "          vrf_arb_req_i != prev_vrf_arb_req ||",
                    "          vrf_arb_gnt_i != prev_vrf_arb_gnt ||",
                    "          hazard_vs1_i != prev_hazard_vs1 ||",
                    "          hazard_vs2_i != prev_hazard_vs2 ||",
                    "          hazard_vd1_i != prev_hazard_vd1 ||",
                    "          hazard_vd2_i != prev_hazard_vd2 ||",
                    "          scalar_state != prev_scalar ||",
                    "          serdiv_grant_i) begin",
                    "        $display(\"ACE_ECHO_LANE_RESOURCE lane %0d "
                    "time %0t pe %0d/%0d opcmd %0h/%0h mask %0d/%0d "
                    "vrf %0h/%0h vfu %0d id %0d op %0d vm %0d/%0d "
                    "ready %0d/%0d/%0d grant %0d/%0d/%0d "
                    "arb %0h/%0h oq %0h/%0h haz %0h/%0h/%0h/%0h\",",
                    "          lane_id_i, $time, pe_valid_i, pe_ready_i,",
                    "          operand_valid_i, operand_ready_i,",
                    "          mask_valid_i, mask_ready_i, vrf_req_i,",
                    "          vrf_wen_i, vfu_valid_i, vfu_id_i, vfu_op_i,",
                    "          vm_r_i, vm_w_i, bitalu_ready_i, cau_ready_i,",
                    "          serdiv_ready_i, bitalu_grant_i, cau_grant_i,",
                    "          serdiv_grant_i, vrf_arb_req_i,",
                    "          vrf_arb_gnt_i, operand_queue_valid_i,",
                    "          operand_queue_ready_i, hazard_vs1_i,",
                    "          hazard_vs2_i, hazard_vd1_i, hazard_vd2_i);",
                    "      end",
                    "      prev_operand_valid <= operand_valid_i;",
                    "      prev_operand_ready <= operand_ready_i;",
                    "      prev_operand_queue_valid <= operand_queue_valid_i;",
                    "      prev_operand_queue_ready <= operand_queue_ready_i;",
                    "      prev_vrf_req <= vrf_req_i;",
                    "      prev_vrf_wen <= vrf_wen_i;",
                    "      prev_vrf_arb_req <= vrf_arb_req_i;",
                    "      prev_vrf_arb_gnt <= vrf_arb_gnt_i;",
                    "      prev_hazard_vs1 <= hazard_vs1_i;",
                    "      prev_hazard_vs2 <= hazard_vs2_i;",
                    "      prev_hazard_vd1 <= hazard_vd1_i;",
                    "      prev_hazard_vd2 <= hazard_vd2_i;",
                    "      prev_scalar <= scalar_state;",
                    "    end",
                    "  end",
                    "endmodule",
                    f"bind {lane['module']} {observer_module} "
                    f"{observer_module}_i (",
                    f"  .clk_i({lane['clock_signal']}),",
                    f"  .rst_ni({lane['reset_signal']}),",
                    f"  .lane_id_i({lane['lane_id_signal']}),",
                    f"  .pe_valid_i({lane['pe_valid_signal']}),",
                    f"  .pe_ready_i({lane['pe_ready_signal']}),",
                    f"  .operand_valid_i({lane['operand_valid_signal']}),",
                    f"  .operand_ready_i({lane['operand_ready_signal']}),",
                    f"  .operand_queue_valid_i("
                    f"{lane['operand_queue_valid_signal']}),",
                    f"  .operand_queue_ready_i("
                    f"{lane['operand_queue_ready_signal']}),",
                    f"  .mask_valid_i({lane['mask_valid_signal']}),",
                    f"  .mask_ready_i({lane['mask_ready_signal']}),",
                    f"  .vrf_req_i({lane['vrf_req_signal']}),",
                    f"  .vrf_wen_i({lane['vrf_wen_signal']}),",
                    f"  .vrf_arb_req_i({lane['vrf_arb_req_signal']}),",
                    f"  .vrf_arb_gnt_i({lane['vrf_arb_gnt_signal']}),",
                    f"  .hazard_vs1_i({lane['hazard_vs1_signal']}),",
                    f"  .hazard_vs2_i({lane['hazard_vs2_signal']}),",
                    f"  .hazard_vd1_i({lane['hazard_vd1_signal']}),",
                    f"  .hazard_vd2_i({lane['hazard_vd2_signal']}),",
                    f"  .vfu_valid_i({lane['vfu_valid_signal']}),",
                    f"  .vfu_id_i({lane['vfu_id_signal']}),",
                    f"  .vfu_op_i({lane['vfu_op_signal']}),",
                    f"  .vm_r_i({lane['vm_r_signal']}),",
                    f"  .vm_w_i({lane['vm_w_signal']}),",
                    f"  .bitalu_ready_i({lane['bitalu_ready_signal']}),",
                    f"  .cau_ready_i({lane['cau_ready_signal']}),",
                    f"  .serdiv_ready_i({lane['serdiv_ready_signal']}),",
                    f"  .bitalu_grant_i({lane['bitalu_grant_signal']}),",
                    f"  .cau_grant_i({lane['cau_grant_signal']}),",
                    f"  .serdiv_grant_i({lane['serdiv_grant_signal']})",
                    ");",
                ])

            for observer_index, sequencer in enumerate(
                    normalized_sequencer_ready):
                observer_module = (
                    f"ace_echo_sequencer_ready_observer_{observer_index}"
                )
                ready_width = sequencer["ready_width"]
                state_width = sequencer["state_width"]
                op_width = sequencer["op_width"]
                queue_width = sequencer["queue_width"]
                queue_counts_width = sequencer["queue_counts_width"]
                bind_code.extend([
                    f"module {observer_module} (",
                    "  input logic clk_i, input logic rst_ni,",
                    f"  input logic [{ready_width - 1}:0] ready_i,",
                    "  input logic pe_valid_i,",
                    f"  input logic [{state_width - 1}:0] state_i,",
                    f"  input logic [{op_width - 1}:0] op_i,",
                    "  input logic upstream_valid_i,",
                    "  input logic upstream_ready_i,",
                    "  input logic running_full_i,",
                    "  input logic lane_desync_i,",
                    f"  input logic [{queue_width - 1}:0] queue_ready_i,",
                    f"  input logic [{queue_width - 1}:0] queue_done_i,",
                    f"  input logic [{queue_counts_width - 1}:0] queue_counts_i,",
                    f"  input logic [{queue_width - 1}:0] target_vfus_i,",
                    "  input logic accepted_i",
                    ");",
                    f"  logic [{ready_width - 1}:0] prev_ready;",
                    "  logic prev_pe_valid;",
                    f"  logic [{state_width - 1}:0] prev_state;",
                    f"  logic [{op_width - 1}:0] prev_op;",
                    "  logic prev_upstream_valid, prev_upstream_ready;",
                    "  logic prev_running_full, prev_lane_desync;",
                    f"  logic [{queue_width - 1}:0] prev_queue_ready;",
                    f"  logic [{queue_width - 1}:0] prev_queue_done;",
                    f"  logic [{queue_counts_width - 1}:0] prev_queue_counts;",
                    f"  logic [{queue_width - 1}:0] prev_target_vfus;",
                    "  logic prev_accepted;",
                    "  always @(posedge clk_i or negedge rst_ni) begin",
                    "    if (!rst_ni) begin",
                    "      prev_ready <= '0; prev_pe_valid <= 1'b0;",
                    "      prev_state <= '0; prev_op <= '0;",
                    "      prev_upstream_valid <= 1'b0;",
                    "      prev_upstream_ready <= 1'b0;",
                    "      prev_running_full <= 1'b0;",
                    "      prev_lane_desync <= 1'b0;",
                    "      prev_queue_ready <= '0; prev_queue_done <= '0;",
                    "      prev_queue_counts <= '0; prev_target_vfus <= '0;",
                    "      prev_accepted <= 1'b0;",
                    "    end else begin",
                    "      if (ready_i != prev_ready ||",
                    "          pe_valid_i != prev_pe_valid ||",
                    "          state_i != prev_state || op_i != prev_op ||",
                    "          upstream_valid_i != prev_upstream_valid ||",
                    "          upstream_ready_i != prev_upstream_ready ||",
                    "          running_full_i != prev_running_full ||",
                    "          lane_desync_i != prev_lane_desync ||",
                    "          queue_ready_i != prev_queue_ready ||",
                    "          queue_done_i != prev_queue_done ||",
                    "          queue_counts_i != prev_queue_counts ||",
                    "          target_vfus_i != prev_target_vfus ||",
                    "          accepted_i != prev_accepted) begin",
                    "        $display(\"ACE_ECHO_SEQUENCER_READY time %0t "
                    "valid %0d state %0d op %0d ready %0h all_lanes %0d "
                    "shuffle %0d upstream %0d/%0d full %0d desync %0d "
                    "qready %0h qdone %0h qcnt %0h target %0h accept %0d\", "
                    "$time, pe_valid_i, state_i, op_i,",
                    f"          ready_i, &ready_i[{ready_width - 2}:0],",
                    f"          ready_i[{ready_width - 1}], upstream_valid_i,",
                    "          upstream_ready_i, running_full_i, lane_desync_i,",
                    "          queue_ready_i, queue_done_i, queue_counts_i,",
                    "          target_vfus_i, accepted_i);",
                    "      end",
                    "      prev_ready <= ready_i;",
                    "      prev_pe_valid <= pe_valid_i;",
                    "      prev_state <= state_i; prev_op <= op_i;",
                    "      prev_upstream_valid <= upstream_valid_i;",
                    "      prev_upstream_ready <= upstream_ready_i;",
                    "      prev_running_full <= running_full_i;",
                    "      prev_lane_desync <= lane_desync_i;",
                    "      prev_queue_ready <= queue_ready_i;",
                    "      prev_queue_done <= queue_done_i;",
                    "      prev_queue_counts <= queue_counts_i;",
                    "      prev_target_vfus <= target_vfus_i;",
                    "      prev_accepted <= accepted_i;",
                    "    end",
                    "  end",
                    "endmodule",
                    f"bind {sequencer['module']} {observer_module} "
                    f"{observer_module}_i (",
                    f"  .clk_i({sequencer['clock_signal']}),",
                    f"  .rst_ni({sequencer['reset_signal']}),",
                    f"  .ready_i({sequencer['ready_vector_signal']}),",
                    f"  .pe_valid_i({sequencer['pe_valid_signal']}),",
                    f"  .state_i({sequencer['state_signal']}),",
                    f"  .op_i({sequencer['op_signal']}),",
                    f"  .upstream_valid_i({sequencer['upstream_valid_signal']}),",
                    f"  .upstream_ready_i({sequencer['upstream_ready_signal']}),",
                    f"  .running_full_i({sequencer['running_full_signal']}),",
                    f"  .lane_desync_i({sequencer['lane_desync_signal']}),",
                    f"  .queue_ready_i({sequencer['queue_ready_signal']}),",
                    f"  .queue_done_i({sequencer['queue_done_signal']}),",
                    f"  .queue_counts_i({sequencer['queue_counts_signal']}),",
                    f"  .target_vfus_i({sequencer['target_vfus_signal']}),",
                    f"  .accepted_i({sequencer['accepted_signal']})",
                    ");",
                ])
            for observer_index, scalar in enumerate(
                    normalized_scalar_retires):
                observer_module = (
                    f"ace_echo_scalar_retire_observer_{observer_index}"
                )
                bind_code.extend([
                    f"module {observer_module} (",
                    "  input logic clk_i,",
                    "  input logic rst_ni,",
                    "  input logic retire_valid_i,",
                    "  input logic [31:0] retire_pc_i,",
                    "  input logic [31:0] retire_instruction_i",
                    ");",
                    "  always @(posedge clk_i) begin",
                    "    if (rst_ni && retire_valid_i && "
                    "retire_instruction_i != 0)",
                    "      $display(\"ACE_ECHO_SCALAR_RETIRE instance %m "
                    "time %0t pc %08x instruction %08x\",",
                    "        $time, retire_pc_i, retire_instruction_i);",
                    "  end",
                    "endmodule",
                    f"bind {scalar['module']} {observer_module} "
                    f"{observer_module}_i (",
                    f"  .clk_i({scalar['clock_signal']}),",
                    f"  .rst_ni({scalar['reset_signal']}),",
                    f"  .retire_valid_i({scalar['valid_signal']}),",
                    f"  .retire_pc_i({scalar['pc_signal']}),",
                    "  .retire_instruction_i("
                    f"{scalar['instruction_signal']})",
                    ");",
                ])
            for observer_index, scalar in enumerate(
                    normalized_scalar_dispatches):
                observer_module = (
                    f"ace_echo_scalar_dispatch_observer_{observer_index}"
                )
                bind_code.extend([
                    f"module {observer_module} (",
                    "  input logic clk_i,",
                    "  input logic rst_ni,",
                    "  input logic request_valid_i,",
                    "  input logic request_ready_i,",
                    "  input logic [31:0] msb_instruction_i,",
                    "  input logic [31:0] lsb_instruction_i,",
                    "  input logic [31:0] id_instruction_i,",
                    "  input logic vec_msb_load_i,",
                    "  input logic vec_lsb_load_wait_i,",
                    "  input logic vec_lsb_load_i,",
                    "  input logic instr_vector_i,",
                    "  input logic uncomplete_i,",
                    "  input logic stall_i,",
                    "  input logic vector_wait_i",
                    ");",
                    "  always @(posedge clk_i) begin",
                    "    if (rst_ni && (request_valid_i || instr_vector_i ||",
                    "        vec_lsb_load_wait_i || vec_lsb_load_i))",
                    "      $display(\"ACE_ECHO_SCALAR_DISPATCH instance %m "
                    "time %0t valid %0d ready %0d msb %08x lsb %08x "
                    "id %08x msb_load %0d lsb_wait %0d lsb_load %0d "
                    "instr_vector %0d uncomplete %0d stall %0d "
                    "vector_wait %0d\",",
                    "        $time, request_valid_i, request_ready_i,",
                    "        msb_instruction_i, lsb_instruction_i,",
                    "        id_instruction_i, vec_msb_load_i,",
                    "        vec_lsb_load_wait_i, vec_lsb_load_i,",
                    "        instr_vector_i, uncomplete_i, stall_i,",
                    "        vector_wait_i);",
                    "  end",
                    "endmodule",
                    f"bind {scalar['module']} {observer_module} "
                    f"{observer_module}_i (",
                    f"  .clk_i({scalar['clock_signal']}),",
                    f"  .rst_ni({scalar['reset_signal']}),",
                    f"  .request_valid_i({scalar['valid_signal']}),",
                    f"  .request_ready_i({scalar['ready_signal']}),",
                    "  .msb_instruction_i("
                    f"{scalar['msb_instruction_signal']}),",
                    "  .lsb_instruction_i("
                    f"{scalar['lsb_instruction_signal']}),",
                    "  .id_instruction_i("
                    f"{scalar['id_instruction_signal']}),",
                    f"  .vec_msb_load_i({scalar['vec_msb_load_signal']}),",
                    "  .vec_lsb_load_wait_i("
                    f"{scalar['vec_lsb_load_wait_signal']}),",
                    f"  .vec_lsb_load_i({scalar['vec_lsb_load_signal']}),",
                    f"  .instr_vector_i({scalar['instr_vector_signal']}),",
                    f"  .uncomplete_i({scalar['uncomplete_signal']}),",
                    f"  .stall_i({scalar['stall_signal']}),",
                    f"  .vector_wait_i({scalar['vector_wait_signal']})",
                    ");",
                ])
            insertion = "\n".join(code) + "\n\n"
            location = endmodules[0]
            path.write_text(
                text[:location] + insertion + text[location:]
                + ("\n" + "\n".join(bind_code) + "\n"
                   if bind_code else ""),
                encoding="utf-8",
            )
            return output_name
        if mode != "ucli_vpd":
            raise ValueError(f"unsupported task-boundary trace mode: {mode}")

        script_name = str(observer.get("ucli_script", ""))
        script_candidate = Path(script_name)
        if (not script_name or script_candidate.is_absolute()
                or ".." in script_candidate.parts):
            raise ValueError(
                f"unsafe task-boundary UCLI script: {script_name!r}"
            )
        signals = observer.get("signals")
        if not isinstance(signals, list) or not signals:
            raise ValueError("task-boundary trace requires observed signals")
        signal_pattern = re.compile(r"^[A-Za-z_$][A-Za-z0-9_$.[\]:]*$")
        invalid = [
            signal for signal in signals
            if not isinstance(signal, str)
            or signal_pattern.fullmatch(signal) is None
        ]
        if invalid:
            raise ValueError(
                f"unsafe task-boundary RTL signal names: {invalid!r}"
            )

        path = snapshot / script_name
        text = path.read_text(encoding="utf-8")
        marker = "# ACE-ECHO task-boundary observer"
        if marker in text:
            raise ValueError(f"task-boundary observer already present in {path}")
        commands = [marker, f"dump -file {output_name}"]
        commands.extend(
            f"dump -add {signal} -depth 0 -aggregates"
            for signal in signals
        )
        path.write_text(
            "\n".join(commands) + "\n" + text,
            encoding="utf-8",
        )
        return output_name

    def _copy_evidence(self, build: Path, output: Path,
                       extra_patterns: list[str] | None = None) -> list[Path]:
        output.mkdir(parents=True, exist_ok=True)
        copied: list[Path] = []
        patterns = list(self.flow.get("evidence_paths", []))
        patterns.extend(extra_patterns or [])
        for pattern in patterns:
            for source in sorted(build.glob(pattern)):
                if not source.is_file():
                    continue
                target = output / source.relative_to(build)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
                copied.append(target)
        return copied

    def _case_result(self, case: RtlCase, build: Path,
                     evidence: list[Path], source_root: Path | None = None) -> dict[str, Any]:
        sim_log = build / "sim.log"
        text = sim_log.read_text(encoding="utf-8", errors="replace")
        lines = text.splitlines()
        failure_marker = self.flow.get("progress_markers", {}).get(
            "crc_failure"
        )
        complete = "Test complete!" in text
        crc_failure = bool(
            failure_marker and f"{failure_marker} pluse" in text
        )
        simulator_error_entries = [
            (index, line) for index, line in enumerate(lines)
            if line.startswith("Error:")
        ]
        simulator_errors = [line for _, line in simulator_error_entries]
        axi_unknown_errors = sum(
            "axi_vif." in line and (
                "assertWriteDataUnknown" in line
                or "assertReadDataUnKnown" in line
            )
            for line in simulator_errors
        )
        known_nonfunctional_errors = classify_startup_errors(
            lines, self.flow.get('startup_diagnostics', []), source_root)
        known_error_indices = {record['line'] - 1 for record in known_nonfunctional_errors}
        for rule in self.flow.get(
                "known_nonfunctional_simulator_errors", []):
            required = [str(item) for item in rule.get("line_contains", [])]
            following = rule.get("next_line_equals")
            if not required or following is None:
                raise ValueError(
                    "known_nonfunctional_simulator_errors entries require "
                    "line_contains and next_line_equals"
                )
            for index, line in simulator_error_entries:
                if index in known_error_indices:
                    continue
                if (all(marker in line for marker in required)
                        and index + 1 < len(lines)
                        and lines[index + 1].strip() == str(following)):
                    known_error_indices.add(index)
                    known_nonfunctional_errors.append({
                        "id": str(rule.get("id", "unnamed")),
                        "line": index + 1,
                    })
        non_axi_errors = sum(
            index not in known_error_indices
            and not (
                "axi_vif." in line and (
                    "assertWriteDataUnknown" in line
                    or "assertReadDataUnKnown" in line
                )
            )
            for index, line in simulator_error_entries
        )
        execution_pass = complete and not crc_failure and not non_axi_errors
        warning_classes = []
        if axi_unknown_errors:
            warning_classes.append("AXI_UNKNOWN")
        if known_nonfunctional_errors:
            warning_classes.append("KNOWN_NONFUNCTIONAL")
        return {
            "case_name": case.name,
            "l1_bin": str(case.l1_bin.resolve()),
            "l1_sha256": self._sha256(case.l1_bin),
            "status": "PASS" if execution_pass else "FAIL",
            "qualification_status": (
                "SMOKE_PASS_WITH_" + "_AND_".join(warning_classes)
                + "_WARNINGS"
                if execution_pass and warning_classes
                else "SMOKE_PASS" if execution_pass else "FAIL"
            ),
            "test_complete_seen": complete,
            "crc_failure_seen": crc_failure,
            "simulator_error_count": len(simulator_errors),
            "axi_unknown_assertion_count": axi_unknown_errors,
            "known_nonfunctional_simulator_error_count": (
                len(known_nonfunctional_errors)
            ),
            "known_nonfunctional_simulator_errors": (
                known_nonfunctional_errors
            ),
            "non_axi_simulator_error_count": non_axi_errors,
            "all_output_comparison_status": "NOT_RUN",
            "evidence": [
                {
                    "path": str(path.resolve()),
                    "sha256": self._sha256(path),
                    "bytes": path.stat().st_size,
                }
                for path in evidence
            ],
        }

    def run(self, *, workspace: Path, cases: list[RtlCase], timeout: int,
            keep_build: bool = False,
            sim_horizon_us: int | None = None,
            task_boundary_trace: bool = False,
            build_timeout: int | None = None, simulation_timeout: int | None = None,
            dag_json: Path | None = None, gem5_output_dir: Path | None = None,
            output_layouts: Path | None = None) -> Path:
        if not cases:
            raise ValueError("at least one RTL case is required")
        if (dag_json is None) != (gem5_output_dir is None) or (output_layouts and dag_json is None):
            raise ValueError('output validation requires DAG metadata and Gem5 output directory')
        if dag_json is not None and len(cases) != 1:
            raise ValueError('provide one RTL case per explicit output comparison')
        if dag_json is not None and not self.runner.dry_run:
            from ..output_capacity import check_observed_returns, runtime_dmt_profile
            check_observed_returns(dag_json, gem5_output_dir,
                                   workspace / 'output-capacity-preflight.json',
                                   runtime_dmt=runtime_dmt_profile(self.backend.gem5.venus_config))
        budgets = self.flow.get('timeouts', {})
        build_timeout = build_timeout if build_timeout is not None else budgets.get('build_seconds', timeout)
        simulation_timeout = simulation_timeout if simulation_timeout is not None else budgets.get('simulation_seconds', timeout)
        if min(timeout, build_timeout, simulation_timeout) <= 0:
            raise ValueError('RTL timeouts must be positive')
        success = False
        fresh_simulators = []
        for case in cases:
            self._validate_case_name(case.name)
            if not self.runner.dry_run and not case.l1_bin.is_file():
                raise FileNotFoundError(case.l1_bin)

        source_identity = self._validate_source_identity()
        snapshot = workspace / "rtl-source"
        output = workspace / "rtl"
        environment = self._environment()
        base_variables = {
            str(key): str(value)
            for key, value in self.flow["make_variables"].items()
        }
        case_variable = str(self.flow["runtime_case_variable"])
        copy_variable = str(self.flow["unique_build_variable"])
        case_results: list[dict[str, Any]] = []
        boundary_trace_output: str | None = None

        try:
            if not self.runner.dry_run:
                self._copy_snapshot(snapshot)
                if sim_horizon_us is not None:
                    self._set_sim_horizon(snapshot, sim_horizon_us)
                if task_boundary_trace:
                    boundary_trace_output = (
                        self._configure_task_boundary_trace(snapshot)
                    )
                for index, case in enumerate(cases):
                    self._stage_firmware(
                        snapshot, case, stage_scheduler=index == 0
                    )

            dependencies = self.flow.get("snapshot_dependencies", {})
            for index, preparation in enumerate(
                    dependencies.get("pre_build_make", []), 1):
                self.runner.run(
                    f"prepare-rtl-{index}",
                    ["make", "-C", snapshot,
                     *self._make_args(preparation.get("variables", {})),
                     preparation["target"]],
                    cwd=workspace, timeout=timeout, env=environment,
                )
                if not self.runner.dry_run:
                    expected = preparation.get("expected_output")
                    if expected and not (snapshot / expected).exists():
                        raise FileNotFoundError(
                            "RTL dependency preparation returned success but "
                            f"produced no {expected}"
                        )

            if task_boundary_trace:
                observer = self.flow["task_boundary_trace"]
                targets = observer.get("pre_build_targets", [])
                if not isinstance(targets, list):
                    raise ValueError(
                        "task-boundary pre_build_targets must be a list"
                    )
                for index, target in enumerate(targets, 1):
                    if (not isinstance(target, str) or not target
                            or any(character.isspace()
                                   for character in target)):
                        raise ValueError(
                            f"unsafe task-boundary build target: {target!r}"
                        )
                    self.runner.run(
                        f"prepare-task-boundary-rtl-{index}",
                        ["make", "-C", snapshot,
                         *self._make_args(base_variables), target],
                        cwd=workspace, timeout=timeout, env=environment,
                    )

            for index, case in enumerate(cases):
                copy_id = f"ace_echo_{index}_{case.name.replace('/', '_')}"
                variables = dict(base_variables)
                variables[case_variable] = case.name
                variables[copy_variable] = copy_id
                target = (
                    (
                        self.flow["task_boundary_trace"].get(
                            "fresh_build_target",
                            self.flow["fresh_build_target"])
                        if index == 0 else
                        self.flow["task_boundary_trace"].get(
                            "replay_target", self.flow["replay_target"])
                    )
                    if task_boundary_trace else
                    (
                        self.flow["fresh_build_target"] if index == 0
                        else self.flow["replay_target"]
                    )
                )
                steps = self.flow.get('fresh_build_steps', []) if not task_boundary_trace else []
                if index == 0 and steps:
                    deadline = time.monotonic() + build_timeout
                    for step_index, step in enumerate(steps):
                        if not isinstance(step, str) or not re.fullmatch(r'[A-Za-z0-9_-]+', step):
                            raise ValueError('unsafe RTL build step')
                        remaining = math.ceil(deadline - time.monotonic())
                        if remaining <= 0:
                            raise TimeoutError('RTL build-stage budget exhausted')
                        self.runner.run(f'rtl-build-{step_index+1}',
                            ['make', '-C', snapshot, *self._make_args(variables), step],
                            cwd=workspace, timeout=remaining, env=environment)
                    template = self.flow['fresh_simulator_path']
                    simulator = (snapshot / template.format(**variables)).resolve()
                    if snapshot.resolve() not in simulator.parents:
                        raise ValueError('fresh simulator path escapes snapshot')
                    if not self.runner.dry_run:
                        if not simulator.is_file():
                            raise FileNotFoundError(f'fresh build produced no simulator: {simulator}')
                        fresh_simulators.append(dict(path=str(simulator), sha256=self._sha256(simulator)))
                    target = self.flow['replay_target']
                self.runner.run(
                    "rtl-fresh" if index == 0 else f"rtl-replay-{index}",
                    ["make", "-C", snapshot,
                     *self._make_args(variables), target],
                    cwd=workspace, timeout=(simulation_timeout if steps or index else build_timeout + simulation_timeout), env=environment,
                )
                if not self.runner.dry_run:
                    build = self._build_directory(snapshot, copy_id)
                    if not build.is_dir():
                        raise FileNotFoundError(
                            f"RTL flow produced no build directory: {build}"
                        )
                    evidence = self._copy_evidence(
                        build, output / case.name.replace("/", "_"),
                        ([boundary_trace_output]
                         if boundary_trace_output else None),
                    )
                    case_result = self._case_result(case, build, evidence, source_root=snapshot)
                    if dag_json is not None:
                        comparison_path = output / case.name.replace('/', '_') / 'output-comparison.json'
                        try:
                            comparison = compare_dag_outputs(
                                build/'dma_read_data_file_L2.txt', gem5_output_dir,
                                output_layouts=output_layouts, rtl_log=build/'sim.log',
                                dag_json=dag_json, identity_contract=self.flow.get('return_identity'))
                        except (OSError, ValueError) as error:
                            comparison = dict(status='BLOCKED', error=str(error))
                        comparison_path.write_text(json.dumps(comparison, indent=2)+'\n')
                        case_result['all_output_comparison_status'] = comparison['status']
                        case_result['output_comparison'] = str(comparison_path)
                        if comparison['status'] != 'PASS':
                            case_result['status'] = 'FAIL'
                            case_result['qualification_status'] = 'OUTPUT_COMPARISON_' + comparison['status']
                    case_results.append(case_result)

            if self.runner.dry_run:
                return output / "validation-report.json"

            after = git_identity(self.backend.rtl_root, "rtl")
            source_unchanged = after == source_identity
            status = (
                "PASS" if source_unchanged
                and all(case["status"] == "PASS" for case in case_results)
                else "FAIL"
            )
            warning_classes = []
            if any(case["axi_unknown_assertion_count"]
                   for case in case_results):
                warning_classes.append("AXI_UNKNOWN")
            if any(case["known_nonfunctional_simulator_error_count"]
                   for case in case_results):
                warning_classes.append("KNOWN_NONFUNCTIONAL")
            qualification_status = (
                "SMOKE_PASS_WITH_" + "_AND_".join(warning_classes)
                + "_WARNINGS"
                if status == "PASS" and warning_classes
                else "SMOKE_PASS" if status == "PASS" else "FAIL"
            )
            report = {
                "schema": "ace-echo-rtl-run/v1",
                "backend_id": self.backend.backend_id,
                "status": status,
                "qualification_status": qualification_status,
                "fresh_compile": True,
                "fresh_simulators": fresh_simulators,
                "timeout_budgets": dict(build_seconds=build_timeout, simulation_seconds=simulation_timeout,
                    mode='SPLIT' if self.flow.get('fresh_build_steps') and not task_boundary_trace else 'COMBINED_LEGACY'),
                "replay_count": max(0, len(cases) - 1),
                "simulation_horizon_us_override": sim_horizon_us,
                "task_boundary_trace_enabled": task_boundary_trace,
                "task_boundary_trace_output": boundary_trace_output,
                "source_identity_before": source_identity,
                "source_identity_after": after,
                "source_unchanged": source_unchanged,
                "source_snapshot_removed": status == 'PASS' and not keep_build,
                "cases": case_results,
                "qualification_scope": self.rtl.get("claim_scope"),
                "snapshot_dependencies": self.flow.get(
                    "snapshot_dependencies", {}
                ),
                "limitations": [
                    "PASS follows the selected RTL testbench oracle; all-output "
                    "qualification additionally requires an explicit output "
                    "comparison report.",
                    "AXI unknown-data assertions are surfaced per case. They "
                    "remain qualification warnings even when the legacy "
                    "firmware completion and CRC oracle pass.",
                    *(
                        ["The repository UCLI simulation horizon was explicitly "
                         "overridden for this run."]
                        if sim_horizon_us is not None else []
                    ),
                    *(
                        ["The task-boundary waveform observes only "
                         "backend-declared RTL signals from the disposable "
                         "snapshot; it does not modify RTL behavior."]
                        if task_boundary_trace else []
                    ),
                ],
            }
            report_path = output / "validation-report.json"
            report_path.parent.mkdir(parents=True, exist_ok=True)
            report_path.write_text(
                json.dumps(report, indent=2) + "\n", encoding="utf-8"
            )
            if status != "PASS":
                raise ValueError(f"RTL validation failed; see {report_path}")
            success = True
            return report_path
        finally:
            # Failed/blocked attempts retain raw evidence for diagnosis.
            if success and not keep_build and snapshot.exists():
                shutil.rmtree(snapshot)
