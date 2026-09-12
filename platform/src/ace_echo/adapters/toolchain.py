from __future__ import annotations

import json
from pathlib import Path
import shutil

from ..backend import BackendProfile
from ..config import PlatformConfig
from ..process import Runner
from ..output_capacity import export_allocations, check_compiled_returns, runtime_dmt_profile
from ..bas_parameters import materialize, runtime_include, sha256
from ..output_layout_contract import export_contract, parse_contract


class ToolchainAdapter:
    """Transitional DSL/LLVM compiler adapter; it never runs VEMU Emulator."""

    def __init__(self, config: PlatformConfig, runner: Runner,
                 backend: BackendProfile | None = None):
        self.config = config
        self.runner = runner
        self.backend = backend
        self.dsl_root = (
            backend.dsl_root
            if backend is not None else config.projects.toolchain_root / "dsl"
        )
        self.workload_root = (
            backend.workload_root
            if backend is not None else config.projects.workload_root
        )
        self.venus_llvm_bin = (
            backend.venus_llvm_bin
            if backend is not None else config.compiler.venus_llvm_bin
        )
        self.riscv_gcc_root = (
            backend.riscv_gcc_root
            if backend is not None else config.compiler.riscv_gcc_root
        )
        self.root = self.dsl_root.parent

    @staticmethod
    def _workload_path(workload: Path, entry: str) -> Path:
        """Resolve logical task paths through an optional Echo Hub layout index."""
        root = workload.resolve()
        relative = Path(entry)
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError(f"workload path must stay inside its root: {entry}")
        parts = relative.parts
        index = root / "task-index.json"
        if len(parts) >= 2 and parts[0] == "tasks" and index.is_file():
            document = json.loads(index.read_text(encoding="utf-8"))
            if document.get("schema") != "echo-hub-task-index/v1":
                raise ValueError(f"unsupported task index: {index}")
            tasks = document.get("tasks")
            if not isinstance(tasks, dict):
                raise ValueError(f"invalid tasks in {index}")
            mapping = tasks.get(parts[1])
            if mapping is not None:
                if not isinstance(mapping, dict) or not isinstance(mapping.get("path"), str):
                    raise ValueError(f"invalid task location: {parts[1]}")
                destination = Path(mapping["path"])
                if destination.is_absolute() or ".." in destination.parts:
                    raise ValueError(f"task index path escapes workload: {parts[1]}")
                relative = destination.joinpath(*parts[2:])
        result = (root / relative).resolve()
        try:
            result.relative_to(root)
        except ValueError as error:
            raise ValueError(f"workload path escapes root: {entry}") from error
        return result


    @staticmethod
    def _task_sources(workload: Path, target_dir: Path) -> list[Path]:
        """Resolve an optional DAG composition manifest to common task sources."""
        manifest = target_dir / "task-sources.json"
        if not manifest.is_file():
            return sorted(target_dir.glob("*.c"))
        document = json.loads(manifest.read_text(encoding="utf-8"))
        if document.get("schema") != "ace-echo-task-source-set/v1":
            raise ValueError(f"unsupported task source manifest: {manifest}")
        entries = document.get("sources")
        if not isinstance(entries, list) or not entries:
            raise ValueError(f"task source manifest has no sources: {manifest}")
        root = workload.resolve()
        sources: list[Path] = []
        names: set[str] = set()
        for entry in entries:
            if not isinstance(entry, str):
                raise ValueError(f"non-string task source in {manifest}")
            source = ToolchainAdapter._workload_path(workload, entry)
            try:
                source.relative_to(root)
            except ValueError as error:
                raise ValueError(
                    f"task source escapes workload root: {entry}") from error
            if not source.is_file() or source.suffix != ".c":
                raise FileNotFoundError(source)
            if source.name in names:
                raise ValueError(f"duplicate task source basename: {source.name}")
            names.add(source.name)
            sources.append(source)
        return sources

    @staticmethod
    def _dag_source(target_dir: Path, target: str) -> Path:
        """Resolve the BAS entry point without guessing among replay variants.

        A workload may retain both a fully materialized software-replay BAS and
        a runtime-input hardware BAS.  The optional manifest makes that choice
        explicit while preserving the historic ``<target>/<target>.bas``
        convention for every workload without a manifest.
        """
        manifest = target_dir / "dag-source.json"
        if not manifest.is_file():
            source = target_dir / f"{target}.bas"
        else:
            document = json.loads(manifest.read_text(encoding="utf-8"))
            if document.get("schema") != "ace-echo-dag-source/v1":
                raise ValueError(f"unsupported DAG source manifest: {manifest}")
            entry = document.get("source")
            if not isinstance(entry, str) or not entry:
                raise ValueError(f"DAG source manifest has no source: {manifest}")
            source = (target_dir / entry).resolve()
            try:
                source.relative_to(target_dir.resolve())
            except ValueError as error:
                raise ValueError(
                    f"DAG source escapes target directory: {entry}") from error
        if not source.is_file() or source.suffix != ".bas":
            raise FileNotFoundError(source)
        return source

    @staticmethod
    def _parameter_source(target_dir: Path, explicit: Path | None) -> Path | None:
        if explicit is not None:
            path = explicit.expanduser().resolve()
        else:
            manifest = target_dir / "dag-source.json"
            if not manifest.is_file():
                return None
            entry = json.loads(manifest.read_text()).get("parameters")
            if entry is None:
                return None
            if not isinstance(entry, str) or not entry:
                raise ValueError("invalid parameters path in " + str(manifest))
            path = (target_dir / entry).resolve()
            if target_dir.resolve() not in path.parents:
                raise ValueError("parameters file escapes target directory")
        if not path.is_file():
            raise FileNotFoundError(path)
        return path

    def compile_dag(self, *, target: str, output: Path, timeout: int,
                    params: Path | None = None) -> Path:
        workload = self.workload_root
        target_dir = self._workload_path(workload, f"tasks/{target}")
        contract_path = target_dir / 'output-validity.json'
        contract_bytes = contract_path.read_bytes() if contract_path.is_file() else None
        if contract_bytes is not None:
            parse_contract(contract_bytes)
        parameter_source = self._parameter_source(target_dir, params)
        binding = None
        merged = None
        if parameter_source is not None:
            dag_source = self._dag_source(target_dir, target)
            bas_bytes, parameter_bytes = dag_source.read_bytes(), parameter_source.read_bytes()
            merged, details = materialize(bas_bytes.decode("utf-8"), parameter_bytes.decode("utf-8"))
            binding = dict(details, schema="ace-echo-bas-parameters/v1",
                           bas_source=str(dag_source), bas_source_sha256=sha256(bas_bytes),
                           parameter_source=str(parameter_source), parameter_source_sha256=sha256(parameter_bytes),
                           merged_bas_sha256=sha256(merged.encode("utf-8")))
        build_root = output / "toolchain-build"
        build_dsl = build_root / "dsl"
        if not self.runner.dry_run:
            shutil.copytree(self.dsl_root, build_dsl)
        if not self.runner.dry_run:
            if not target_dir.is_dir():
                raise FileNotFoundError(target_dir)
            venus_test = build_dsl / "venus_test"
            for pattern in ("Task*", "*.c"):
                for old in venus_test.glob(pattern):
                    if old.is_file() or old.is_symlink():
                        old.unlink()
            for old in build_dsl.glob("*.bas"):
                old.unlink()
            for header in sorted((workload / "include").glob("*.h")):
                shutil.copy2(header, venus_test / header.name)
            # Keep workload-local configuration headers beside their task
            # sources.  This lets a task carry a backend-overridable memory
            # map without leaking profile-specific constants into the shared
            # workload include directory.
            for header in sorted(target_dir.glob("*.h")):
                shutil.copy2(header, venus_test / header.name)
            for source in self._task_sources(workload, target_dir):
                shutil.copy2(source, venus_test / source.name)
            dag_source = self._dag_source(target_dir, target)
            # The DSL Makefile addresses the entry point through TARGET_DAG;
            # normalize only the run-local copy and leave the source immutable.
            shutil.copy2(dag_source, build_dsl / f"{target}.bas")
            if merged is not None:
                (build_dsl / f"{target}.bas").write_text(merged, encoding="utf-8")
        make_variables = []
        if self.backend is not None:
            make_variables = [
                f"{key}={value}"
                for key, value in sorted(
                    self.backend.toolchain_make_variables.items()
                )
            ]
        self.runner.run(
            "compile-dag",
            [
                "make", "-C", build_dsl, "all", f"TARGET_DAG={target}",
                f"LLVM_PATH={self.venus_llvm_bin}",
                f"RVPATH={self.riscv_gcc_root}",
                *make_variables,
            ],
            cwd=self.root, timeout=timeout,
        )
        artifact_dir = output / "toolchain"
        if not self.runner.dry_run:
            artifact_dir.mkdir(parents=True, exist_ok=True)
            sources = {
                build_dsl / "final_output/dag1.json": artifact_dir / "dag1.json",
                build_dsl / "bin/dag1.bin": artifact_dir / "dag1.bin",
            }
            for source, destination in sources.items():
                if not source.is_file():
                    raise FileNotFoundError(source)
                shutil.copy2(source, destination)
            case_dir = artifact_dir / "tasks"
            case_dir.mkdir()
            for task_bin in sorted((build_dsl / "venus_test/ir").glob("*.bin")):
                shutil.copy2(task_bin, case_dir / task_bin.name)
            dag_path = artifact_dir / "dag1.json"
            dag = json.loads(dag_path.read_text())
            memory_map = build_dsl / "IJ/dag1/temp_memory_map.json"
            if memory_map.is_file():
                export_allocations(dag, json.loads(memory_map.read_text()))
                shutil.copy2(memory_map, artifact_dir / "temp_memory_map.json")
                dag_path.write_text(json.dumps(dag, indent=2) + "\n")
            registers = (self.backend.manifest.get('abi', {}).get('compiled_return_registers')
                         if self.backend is not None else None)
            check_compiled_returns(dag, build_dsl / "venus_test/ir", registers,
                                   artifact_dir / "output-capacity-report.json",
                                   runtime_dmt=runtime_dmt_profile((self.backend.gem5 if self.backend else self.config.gem5).venus_config))
            if binding is not None:
                # Metadata belongs on a task, not on the strict return_output
                # aggregate consumed by the firmware DMT decoder.
                next(task for task in dag if "current_taskId" in task)["_ace_echo_parameters"] = {"runtime_inputs": bool(binding["runtime"])}
                dag_path.write_text(json.dumps(dag, indent=2) + "\n")
                # Preserve both the explicit input and the exact compiler input.
                (artifact_dir / "input.params").write_bytes(parameter_bytes)
                (artifact_dir / "materialized.bas").write_text(merged, encoding="utf-8")
                if binding["runtime"]:
                    header = runtime_include(binding["runtime"]).encode("utf-8")
                    (artifact_dir / "ace_echo_inputs.inc").write_bytes(header)
                    binding["runtime_include_sha256"] = sha256(header)
                binding["dag_json_sha256"] = sha256(dag_path.read_bytes())
                binding["dag_bin_sha256"] = sha256((artifact_dir / "dag1.bin").read_bytes())
                (artifact_dir / "parameters.json").write_text(json.dumps(binding, indent=2) + "\n")
            if contract_bytes is not None:
                export_contract(contract_bytes, dag_path)
        return artifact_dir
