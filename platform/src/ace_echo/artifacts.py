from __future__ import annotations

from dataclasses import asdict, dataclass
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_name(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9_.-]+", "-", value).strip("-.")
    return value or "run"


def git_identity(root: Path, component_name: str | None = None) -> dict[str, object]:
    def invoke(*args: str) -> str | None:
        try:
            result = subprocess.run(
                ["git", "-C", str(root), *args], capture_output=True,
                text=True, timeout=10, check=False,
            )
        except (OSError, subprocess.TimeoutExpired):
            return None
        return result.stdout.strip() if result.returncode == 0 else None

    status = invoke("status", "--porcelain")
    identity: dict[str, object] = {
        "root": str(root.resolve()),
        "head": invoke("rev-parse", "HEAD"),
        "branch": invoke("branch", "--show-current"),
        "dirty": bool(status) if status is not None else None,
    }
    if identity["head"] is None:
        manifests = [
            root.parent / "manifest.json",
            root.parent.parent / "components/manifest.json",
        ]
        component = {}
        manifest = manifests[0]
        for candidate in manifests:
            try:
                raw = json.loads(candidate.read_text(encoding="utf-8"))
                component = raw.get("components", {}).get(
                    component_name or root.name, {})
            except (OSError, ValueError, TypeError):
                component = {}
            if component:
                manifest = candidate
                break
        if component:
            identity.update({
                "head": component.get("source_head"),
                "branch": component.get("source_branch"),
                "dirty": component.get("working_tree_snapshot"),
                "identity_source": str(manifest.resolve()),
            })
    return identity


@dataclass(frozen=True)
class RunRecord:
    schema: str
    run_id: str
    scope: str
    target: str
    mode: str
    status: str
    coverage: str
    created_at: str
    config: str
    components: dict[str, dict[str, object]]
    inputs: list[dict[str, str]]
    outputs: list[dict[str, str]]
    diagnostics: list[str]


class RunWorkspace:
    def __init__(self, root: Path, scope: str, target: str, mode: str,
                 config: Path, components: dict[str, Path],
                 explicit: Path | None = None):
        timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        run_id = f"{timestamp}-{safe_name(scope)}-{safe_name(target)}"
        self.path = (explicit or root / run_id).resolve()
        if self.path.exists() and any(self.path.iterdir()):
            raise FileExistsError(f"run directory is not empty: {self.path}")
        self.path.mkdir(parents=True, exist_ok=True)
        self.commands = self.path / "commands"
        self.artifacts = self.path / "artifacts"
        self.commands.mkdir()
        self.artifacts.mkdir()
        self.record = RunRecord(
            schema="ace-echo-run/v1",
            run_id=run_id,
            scope=scope,
            target=target,
            mode=mode,
            status="RUNNING",
            coverage="unproven",
            created_at=datetime.now(timezone.utc).isoformat(),
            config=str(config.resolve()),
            components={name: git_identity(path, name)
                        for name, path in components.items()},
            inputs=[], outputs=[], diagnostics=[],
        )
        self.write()

    def add_input(self, path: Path) -> None:
        resolved = path.resolve()
        entry = {"path": str(resolved)}
        if resolved.is_file():
            entry["sha256"] = sha256(resolved)
        self.record.inputs.append(entry)
        self.write()

    def add_output(self, path: Path) -> None:
        resolved = path.resolve()
        if any(item["path"] == str(resolved) for item in self.record.outputs):
            return
        entry = {"path": str(resolved)}
        if resolved.is_file():
            entry["sha256"] = sha256(resolved)
        self.record.outputs.append(entry)
        self.write()

    def finish(self, status: str, coverage: str,
               diagnostics: list[str] | None = None) -> None:
        object.__setattr__(self.record, "status", status)
        object.__setattr__(self.record, "coverage", coverage)
        if diagnostics:
            self.record.diagnostics.extend(diagnostics)
        self.write()

    def write(self) -> None:
        (self.path / "run.json").write_text(
            json.dumps(asdict(self.record), indent=2) + "\n",
            encoding="utf-8",
        )
