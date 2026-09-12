from __future__ import annotations

from dataclasses import asdict, dataclass
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import time
from typing import Mapping, Sequence


@dataclass(frozen=True)
class CommandResult:
    argv: list[str]
    cwd: str
    returncode: int | None
    duration_seconds: float
    log: str
    dry_run: bool
    termination_reason: str = "EXITED"
    cleanup: str = "NOT_NEEDED"


class CommandFailed(RuntimeError):
    def __init__(self, result: CommandResult):
        self.result = result
        super().__init__(
            f"command failed ({result.returncode}): "
            + shlex.join(result.argv)
            + f"; log: {result.log}"
        )


class Runner:
    def __init__(self, command_dir: Path, dry_run: bool = False):
        self.command_dir = command_dir
        self.command_dir.mkdir(parents=True, exist_ok=True)
        self.dry_run = dry_run
        self._ordinal = 0

    @staticmethod
    def _group_exists(pgid: int) -> bool:
        try:
            os.killpg(pgid, 0)
            return True
        except ProcessLookupError:
            return False

    @classmethod
    def _stop_group(cls, child: subprocess.Popen, grace: float = 2.0) -> str:
        # Only this command's newly created session is targeted, never the
        # caller's session or a process selected by name.
        if not cls._group_exists(child.pid):
            child.wait()
            return "REAPED"
        try:
            os.killpg(child.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        deadline = time.monotonic() + grace
        while time.monotonic() < deadline:
            child.poll()
            if not cls._group_exists(child.pid):
                child.wait()
                return "GROUP_TERMINATED"
            time.sleep(0.02)
        try:
            os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        child.wait()
        return "GROUP_KILLED_PARENT_REAPED"

    def run(
        self,
        name: str,
        argv: Sequence[str | Path],
        *,
        cwd: Path,
        timeout: int,
        env: Mapping[str, str] | None = None,
        check: bool = True,
    ) -> CommandResult:
        self._ordinal += 1
        if timeout <= 0:
            raise ValueError("command timeout must be positive")
        normalized = [str(value) for value in argv]
        stem = f"{self._ordinal:02d}-{name}"
        log_path = self.command_dir / f"{stem}.log"
        result_path = self.command_dir / f"{stem}.json"
        started = time.monotonic()
        if self.dry_run:
            log_path.write_text(shlex.join(normalized) + "\n", encoding="utf-8")
            result = CommandResult(
                argv=normalized,
                cwd=str(cwd.resolve()),
                returncode=None,
                duration_seconds=0.0,
                log=str(log_path.resolve()),
                dry_run=True,
            )
        else:
            if os.name != "posix":
                raise RuntimeError(
                    "managed execution requires a POSIX host with process groups; "
                    "use the configured Linux execution host (dry-run is portable)"
                )
            merged_env = os.environ.copy()
            if env:
                merged_env.update(env)
            with log_path.open("w", encoding="utf-8") as log:
                reason, cleanup, interrupted = "EXITED", "NOT_NEEDED", None
                child = None
                try:
                    child = subprocess.Popen(
                        normalized,
                        cwd=cwd,
                        env=merged_env,
                        stdout=log,
                        stderr=subprocess.STDOUT,
                        start_new_session=True,
                    )
                    returncode = child.wait(timeout=timeout)
                    if self._group_exists(child.pid):
                        cleanup = self._stop_group(child)
                        reason = "ORPHANED_CHILDREN"
                        returncode = 125
                except subprocess.TimeoutExpired:
                    returncode = 124
                    reason = "TIMEOUT"
                    cleanup = self._stop_group(child)
                except (KeyboardInterrupt, SystemExit) as error:
                    interrupted = error
                    returncode = 130
                    reason = "CANCELLED"
                    if child is not None:
                        cleanup = self._stop_group(child)
                except OSError as error:
                    if child is not None:
                        self._stop_group(child)
                    log.write(f"launch error: {error}\n")
                    returncode, reason = 127, "LAUNCH_ERROR"
            result = CommandResult(
                argv=normalized,
                cwd=str(cwd.resolve()),
                returncode=returncode,
                duration_seconds=time.monotonic() - started,
                log=str(log_path.resolve()),
                dry_run=False,
                termination_reason=reason,
                cleanup=cleanup,
            )
        result_path.write_text(json.dumps(asdict(result), indent=2) + "\n",
                               encoding="utf-8")
        if not self.dry_run and interrupted is not None:
            raise interrupted
        if check and result.returncode not in (None, 0):
            raise CommandFailed(result)
        return result
