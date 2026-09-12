from __future__ import annotations

from pathlib import Path
import json
import os
import sys
import hashlib

from ..backend import BackendProfile
from ..config import PlatformConfig
from ..process import Runner
from ..output_capacity import check_observed_returns, write_report, runtime_dmt_profile


class Gem5Adapter:
    def __init__(self, config: PlatformConfig, runner: Runner,
                 backend: BackendProfile | None = None):
        self.config = config
        self.runner = runner
        self.backend = backend
        self.root = (
            backend.gem5_root
            if backend is not None else config.projects.gem5_root
        )
        self.gem5 = backend.gem5 if backend is not None else config.gem5

    @property
    def task_materializer(self) -> Path:
        return self.root / "tools/venus_single_task_manifest.py"

    @property
    def dag_driver(self) -> Path:
        return self.root / "tools/venus_dag.py"

    @property
    def l1_decoder(self) -> Path:
        return self.root / "tools/venus_l1_dag.py"

    def _require_binary(self, mode: str) -> Path:
        """Fail before producing replay artifacts; never substitute fast for verification."""
        binary = self.gem5.binary(mode)
        if not self.runner.dry_run and (not binary.is_file() or not os.access(binary, os.X_OK)):
            build_mode = 'debug' if mode == 'verification' else 'opt'
            raise FileNotFoundError(
                f"Gem5 {mode} executable is missing or not executable: {binary}. "
                "For the standard checkout-local backend, run "
                f"python3 scripts/bootstrap.py build-gem5 --mode {build_mode} --jobs 4 "
                "from the ACE-ECHO root, or use bootstrap.py install-gem5 with an "
                "approved same-source build receipt. For a custom backend, install "
                "the executable at its configured path. No simulation or fallback was started."
            )
        return binary

    def _run_manifest(
        self, manifest: Path, output: Path, mode: str, timeout: int,
        scheduler_firmware_elf: Path | None = None,
        firmware_completion_gpio_mask: int = 0,
    ) -> None:
        binary = self._require_binary(mode)
        if not self.runner.dry_run:
            output.mkdir(parents=True, exist_ok=True)
        command: list[str | Path] = [
            binary,
            f"--outdir={output / 'm5out'}",
            self.gem5.config_script,
            f"--dag-manifest={manifest}",
            f"--venus-config={self.gem5.venus_config}",
            f"--venus-sim-mode={self.gem5.observation_mode(mode)}",
        ]
        if scheduler_firmware_elf is not None:
            command.extend([
                f"--scheduler-firmware-elf={scheduler_firmware_elf.resolve()}",
                f"--scheduler-firmware-trace={output / 'firmware-trace.jsonl'}",
                "--scheduler-firmware-completion-gpio-mask="
                f"{firmware_completion_gpio_mask:#x}",
            ])
        self.runner.run(
            "gem5",
            command,
            # Venus verification dumps use a process-relative Debug path.
            # Give every run a private cwd so parallel DAGs cannot overwrite
            # or merge one another's per-VINS evidence.
            cwd=output,
            timeout=timeout,
        )

    def run_task(self, *, dag_json: Path, combined_bin: Path,
                 case_dir: Path, task: int, output: Path, mode: str,
                 timeout: int) -> Path:
        self._require_binary(mode)
        manifest_dir = output / "manifest"
        self.runner.run(
            "materialize-task",
            [
                sys.executable, self.task_materializer,
                "--dag-json", dag_json.resolve(),
                "--combined-bin", combined_bin.resolve(),
                "--case-dir", case_dir.resolve(),
                "--task", str(task),
                "--output-dir", manifest_dir,
            ],
            cwd=self.root,
            timeout=timeout,
        )
        manifest = manifest_dir / "venus_dag_manifest.json"
        self._run_manifest(manifest, output, mode, timeout)
        if not self.runner.dry_run:
            check_observed_returns(dag_json, manifest_dir,
                                   output / 'output-capacity-report.json', task_id=task, runtime_dmt=runtime_dmt_profile(self.gem5.venus_config))
        return manifest

    def _hydrate_descriptor(self, dag_json: Path, combined_bin: Path,
                            output: Path) -> Path:
        raw = json.loads(dag_json.read_text(encoding="utf-8"))
        combined = combined_bin.read_bytes()
        for task in raw:
            if not isinstance(task, dict) or "current_taskId" not in task:
                continue
            for item in task.get("all_input", []):
                # DSL input ownership is encoded independently from pass
                # mode: 1/5 are static external inputs, 2/6 are dynamic DAG
                # inputs, and 0/4 are producer dependencies.
                input_type = int(item.get("type", "0"), 0)
                if input_type not in (1, 2, 5, 6) or item.get("data"):
                    continue
                offset = int(item["offset"])
                slice_length = int(item.get("slice_length", 0))
                length = slice_length if slice_length > 0 else int(item["length"])
                payload = combined[offset:offset + length]
                if len(payload) != length:
                    raise ValueError(
                        f"task {task['current_taskId']} input {item.get('name')} "
                        "is outside the combined DAG image"
                    )
                item["data"] = "0x" + payload[::-1].hex()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(raw, indent=2) + "\n", encoding="utf-8")
        return output

    def run_dag(self, *, dag_json: Path, combined_bin: Path,
                case_dir: Path, output: Path,
                mode: str, timeout: int) -> Path:
        binary = self._require_binary(mode)
        hydrated = output / "hydrated_dag.json"
        if not self.runner.dry_run:
            self._hydrate_descriptor(dag_json, combined_bin, hydrated)
        else:
            hydrated = dag_json
        self.runner.run(
            "run-dag",
            [
                sys.executable, self.dag_driver, hydrated,
                "run-dag-inprocess",
                "--case-dir", case_dir.resolve(),
                "--combined-bin", combined_bin.resolve(),
                "--run-dir", output,
                "--gem5", binary,
                "--config", self.gem5.config_script,
                "--venus-config", self.gem5.venus_config,
                "--sim-mode", self.gem5.observation_mode(mode),
                "--timeout", str(timeout),
            ],
            cwd=self.root,
            timeout=timeout + 60,
        )
        if not self.runner.dry_run:
            check_observed_returns(dag_json, output,
                                   output / 'output-capacity-report.json', runtime_dmt=runtime_dmt_profile(self.gem5.venus_config))
        return output / "venus_dag_manifest.json"

    def run_application_contract(
        self, *, l1_elf: Path, output: Path, mode: str, timeout: int,
        prefix: str | None = None, l2_backing_spec: Path | None = None,
        task_input_type_bits: int = 3,
        task_container_spmd_fields: bool = True,
        execute_firmware: bool = False,
        firmware_completion_gpio_mask: int = 0,
    ) -> Path:
        self._require_binary(mode)
        manifest_dir = output / "manifest"
        command: list[str | Path] = [
            sys.executable, self.l1_decoder, l1_elf.resolve(),
            "--output-dir", manifest_dir,
            "--task-input-type-bits", str(task_input_type_bits),
        ]
        if not task_container_spmd_fields:
            command.append("--task-container-without-spmd-fields")
        if execute_firmware:
            command.append("--all-dags")
        if prefix:
            command.extend(["--prefix", prefix])
        if l2_backing_spec:
            command.extend(["--l2-backing-spec", l2_backing_spec.resolve()])
        self.runner.run(
            "decode-scheduler-contract", command,
            cwd=self.root, timeout=timeout,
        )
        manifest = manifest_dir / "venus_dag_manifest.json"
        self._run_manifest(
            manifest, output, mode, timeout,
            scheduler_firmware_elf=l1_elf if execute_firmware else None,
            firmware_completion_gpio_mask=firmware_completion_gpio_mask,
        )
        if not self.runner.dry_run:
            document = json.loads(manifest.read_text())
            # Firmware decoding records the exact staged compiled JSON. Do
            # not reconstruct capacities from ELF return lengths or filenames.
            layout = document.get('dmt_layout', {})
            if layout.get('source_json'):
                dag_path = Path(layout['source_json'])
                if hashlib.sha256(dag_path.read_bytes()).hexdigest() != layout.get('source_json_sha256'):
                    raise ValueError('runtime capacity metadata digest changed')
                check_observed_returns(dag_path, manifest_dir,
                                       output / 'output-capacity-report.json', runtime_dmt=runtime_dmt_profile(self.gem5.venus_config))
            else:
                write_report(output / 'output-capacity-report.json', [dict(
                    code='RETURN_LENGTH_UNRESOLVED', severity='WARNING',
                    reason='no single-DAG compiled allocation metadata; explicit per-DAG validation required')])
        return manifest
