from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import socket
from typing import Any


class RtlPreflightError(ValueError):
    pass


@dataclass(frozen=True)
class ArityOccurrence:
    path: str
    line: int
    arity: int


def _strip_comments(text: str) -> str:
    """Remove comments while preserving newlines and byte offsets."""
    pattern = re.compile(r"//[^\n]*|/\*.*?\*/", re.DOTALL)

    def blank(match: re.Match[str]) -> str:
        return "".join("\n" if char == "\n" else " " for char in match.group())

    return pattern.sub(blank, text)


def _matching_paren(text: str, opening: int) -> int:
    depth = 0
    for index in range(opening, len(text)):
        char = text[index]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return index
    raise RtlPreflightError(f"unterminated argument list at offset {opening}")


def _argument_count(arguments: str) -> int:
    if not arguments.strip():
        return 0
    depths = {"(": 0, "[": 0, "{": 0}
    closing = {")": "(", "]": "[", "}": "{"}
    commas = 0
    in_string = False
    escaped = False
    for char in arguments:
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char in depths:
            depths[char] += 1
        elif char in closing:
            depths[closing[char]] -= 1
        elif char == "," and all(value == 0 for value in depths.values()):
            commas += 1
    return commas + 1


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _local_listen_port(port: int) -> bool:
    """Read Linux socket tables when sandbox policy forbids localhost connect."""
    encoded_port = f"{port:04X}"
    for table in (Path("/proc/net/tcp"), Path("/proc/net/tcp6")):
        try:
            lines = table.read_text(encoding="ascii").splitlines()[1:]
        except OSError:
            continue
        for line in lines:
            fields = line.split()
            if len(fields) < 4:
                continue
            local_address = fields[1]
            state = fields[3]
            address_port = local_address.rpartition(":")[2].upper()
            if address_port == encoded_port and state == "0A":
                return True
    return False


def _execution_context() -> dict[str, Any]:
    sandbox_network_disabled = os.environ.get(
        "CODEX_SANDBOX_NETWORK_DISABLED", ""
    ).strip().lower() in {"1", "true", "yes", "on"}
    try:
        network_namespace = os.readlink("/proc/self/ns/net")
    except OSError:
        network_namespace = None
    return {
        "sandbox_network_disabled": sandbox_network_disabled,
        "network_namespace": network_namespace,
    }


def _tool_environment_checks(
        rtl_config: dict[str, Any],
        execution_context: dict[str, Any]) -> list[dict[str, Any]]:
    """Check configured RTL tools and licenses without changing host state."""
    flow = rtl_config.get("flow")
    if not isinstance(flow, dict):
        return []
    required_environment = flow.get("required_environment", {})
    if not isinstance(required_environment, dict):
        required_environment = {}
    path_prepend = flow.get("path_prepend", [])
    if not isinstance(path_prepend, list):
        path_prepend = []

    results: list[dict[str, Any]] = []
    search_path = os.pathsep.join([
        *(str(Path(item).expanduser()) for item in path_prepend),
        os.environ.get("PATH", ""),
    ])
    configured_tools = rtl_config.get("preflight", {}).get(
        "required_tools", ["vlogan", "vcs"]
    )
    if not isinstance(configured_tools, list):
        configured_tools = []
    for tool in configured_tools:
        resolved = shutil.which(str(tool), path=search_path)
        results.append({
            "id": f"rtl-tool:{tool}",
            "status": "PASS" if resolved else "BLOCKED",
            "reason": (
                "tool is executable through backend path_prepend"
                if resolved else
                "tool is absent from backend path_prepend and PATH"
            ),
            "resolved_path": resolved,
        })

    for variable in ("VCS_HOME", "DESIGNWARE_HOME", "SLI_USER_SLISERV_PATH"):
        value = required_environment.get(variable)
        if value is None:
            continue
        path = Path(str(value)).expanduser()
        usable = path.is_dir() if variable != "SLI_USER_SLISERV_PATH" else (
            path.is_file() and os.access(path, os.X_OK)
        )
        results.append({
            "id": f"rtl-environment-path:{variable}",
            "status": "PASS" if usable else "BLOCKED",
            "reason": (
                "configured path is usable"
                if usable else
                "configured path is missing or unusable"
            ),
            "configured_path": str(path),
        })

    license_variables = rtl_config.get("preflight", {}).get(
        "license_environment",
        ["LM_LICENSE_FILE", "SNPSLMD_LICENSE_FILE"],
    )
    if not isinstance(license_variables, list):
        license_variables = []
    timeout = rtl_config.get("preflight", {}).get(
        "license_connect_timeout_seconds", 1.0
    )
    try:
        timeout = max(0.1, float(timeout))
    except (TypeError, ValueError):
        timeout = 1.0
    endpoint_pattern = re.compile(r"^(\d+)@([^@]+)$")
    for variable in license_variables:
        value = required_environment.get(str(variable))
        candidates: list[dict[str, Any]] = []
        if isinstance(value, str) and value:
            for raw_candidate in value.split(os.pathsep):
                candidate = raw_candidate.strip()
                if not candidate:
                    continue
                endpoint = endpoint_pattern.fullmatch(candidate)
                if endpoint:
                    port = int(endpoint.group(1))
                    host = endpoint.group(2)
                    reason_code = None
                    if host in {"localhost", "127.0.0.1", "::1"}:
                        reachable = _local_listen_port(port)
                        if reachable:
                            candidate_status = "PASS"
                            detail = "local TCP endpoint is listening"
                        elif execution_context["sandbox_network_disabled"]:
                            candidate_status = "BLOCKED"
                            reason_code = "REQUIRES_HOST_RECHECK"
                            detail = (
                                "sandbox-local loopback cannot establish host "
                                "license availability; rerun preflight in the "
                                "same host network context as VCS"
                            )
                        else:
                            candidate_status = "BLOCKED"
                            reason_code = "LICENSE_ENDPOINT_UNREACHABLE"
                            detail = (
                                "local TCP endpoint has no listening socket in "
                                "the VCS execution context"
                            )
                    else:
                        try:
                            with socket.create_connection(
                                (host, port), timeout=timeout
                            ):
                                pass
                            candidate_status = "PASS"
                            detail = "TCP endpoint accepted a connection"
                        except OSError as error:
                            candidate_status = "BLOCKED"
                            reason_code = "LICENSE_ENDPOINT_UNREACHABLE"
                            detail = f"TCP endpoint is unreachable: {error}"
                    candidates.append({
                        "candidate": candidate,
                        "kind": "flexlm_endpoint",
                        "status": candidate_status,
                        "reason_code": reason_code,
                        "detail": detail,
                    })
                else:
                    license_path = Path(candidate).expanduser()
                    usable = (
                        license_path.is_file()
                        and license_path.stat().st_size > 0
                    )
                    candidates.append({
                        "candidate": str(license_path),
                        "kind": "license_file",
                        "status": "PASS" if usable else "BLOCKED",
                        "detail": (
                            "license file exists and is non-empty"
                            if usable else
                            "license file is missing or empty"
                        ),
                    })
        usable = any(item["status"] == "PASS" for item in candidates)
        requires_host_recheck = (
            not usable and any(
                item.get("reason_code") == "REQUIRES_HOST_RECHECK"
                for item in candidates
            )
        )
        results.append({
            "id": f"rtl-license:{variable}",
            "status": "PASS" if usable else "BLOCKED",
            "reason_code": (
                "REQUIRES_HOST_RECHECK"
                if requires_host_recheck else None
            ),
            "reason": (
                "at least one configured license source is reachable"
                if usable else
                "localhost license source must be checked from the host "
                "network context"
                if requires_host_recheck else
                "no configured license source is reachable"
            ),
            "configured_value": value,
            "candidates": candidates,
        })
    return results


def _function_declarations(path: Path, name: str) -> list[ArityOccurrence]:
    text = _strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    result: list[ArityOccurrence] = []
    pattern = re.compile(
        rf"\bfunction\b[^;]*?\b{re.escape(name)}\s*\(", re.DOTALL
    )
    for match in pattern.finditer(text):
        opening = match.end() - 1
        closing = _matching_paren(text, opening)
        result.append(ArityOccurrence(
            path=str(path),
            line=text.count("\n", 0, match.start()) + 1,
            arity=_argument_count(text[opening + 1:closing]),
        ))
    return result


def _function_calls(path: Path, name: str) -> list[ArityOccurrence]:
    text = _strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    result: list[ArityOccurrence] = []
    pattern = re.compile(rf"\b{re.escape(name)}\s*\(")
    for match in pattern.finditer(text):
        prefix = text[max(0, match.start() - 160):match.start()]
        if re.search(r"\bfunction\b[^;]*$", prefix, re.DOTALL):
            continue
        opening = match.end() - 1
        closing = _matching_paren(text, opening)
        result.append(ArityOccurrence(
            path=str(path),
            line=text.count("\n", 0, match.start()) + 1,
            arity=_argument_count(text[opening + 1:closing]),
        ))
    return result


def run_rtl_preflight(backend_path: Path,
                      rtl_root_override: Path | None = None) -> dict[str, Any]:
    backend_path = backend_path.expanduser().resolve()
    backend = json.loads(backend_path.read_text(encoding="utf-8"))
    rtl_config = backend.get("engines", {}).get("rtl", {})
    checks = rtl_config.get("preflight", {}).get(
        "systemverilog_function_arity", []
    )
    if not isinstance(checks, list) or not checks:
        raise RtlPreflightError(
            "backend has no engines.rtl.preflight.systemverilog_function_arity checks"
        )
    raw_root = rtl_root_override or Path(backend["paths"]["rtl_root"])
    rtl_root = raw_root.expanduser()
    if not rtl_root.is_absolute():
        rtl_root = (backend_path.parent / rtl_root).resolve()
    else:
        rtl_root = rtl_root.resolve()

    results: list[dict[str, Any]] = []
    source_digests: dict[str, str] = {}
    for check in checks:
        name = str(check["function"])
        declaration_path = rtl_root / str(check["declaration_file"])
        call_paths = [rtl_root / str(item) for item in check["call_files"]]
        for path in [declaration_path, *call_paths]:
            if not path.is_file():
                raise RtlPreflightError(f"RTL preflight input is missing: {path}")
            source_digests[str(path.relative_to(rtl_root))] = _sha256(path)
        declarations = _function_declarations(declaration_path, name)
        calls = [
            occurrence
            for path in call_paths
            for occurrence in _function_calls(path, name)
        ]
        declared_arities = sorted({item.arity for item in declarations})
        mismatches = [
            item for item in calls if item.arity not in declared_arities
        ]
        status = "PASS"
        reason = "all configured calls match a declaration arity"
        if not declarations:
            status = "FAIL"
            reason = "no configured function declaration was found"
        elif not calls:
            status = "FAIL"
            reason = "no configured function call was found"
        elif mismatches:
            status = "FAIL"
            reason = "one or more call arities do not match the declaration"
        results.append({
            "id": str(check.get("id", f"sv-function-arity:{name}")),
            "function": name,
            "status": status,
            "reason": reason,
            "declarations": [asdict(item) for item in declarations],
            "calls": [asdict(item) for item in calls],
            "mismatches": [asdict(item) for item in mismatches],
        })

    execution_context = _execution_context()
    results.extend(_tool_environment_checks(rtl_config, execution_context))
    statuses = {item["status"] for item in results}
    status = "FAIL" if "FAIL" in statuses else (
        "BLOCKED" if "BLOCKED" in statuses else "PASS"
    )
    return {
        "schema": "ace-echo-rtl-preflight/v1",
        "status": status,
        "backend_id": backend.get("backend_id"),
        "backend_manifest": str(backend_path),
        "rtl_root": str(rtl_root),
        "read_only": True,
        "execution_context": execution_context,
        "checks": results,
        "source_sha256": source_digests,
    }
