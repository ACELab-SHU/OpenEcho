"""Optional BAS-syntax parameter input; no evaluator/compiler execution here.

The legacy no-file compile path never calls this materializer. Apostrophes are
comments, including inside multiline initializers. Numeric syntax is the BAS
literal list subset (not expressions, hexadecimal, or executable Python).
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import re


TYPES = {"char": 8, "short": 16, "int": 32, "float": 32, "double": 64}
PARAM = re.compile(r"^[ \t]*parameter\s+(char|short|int|float|double)\s+([A-Za-z_]\w*)\s*=\s*\{([^{}]*)\}[ \t\r]*(?=\n|\Z)", re.M | re.I)
RUNTIME = re.compile(r"^[ \t]*(dag_input|dfedata)\s+(char|short|int|float|double)\s+([A-Za-z_]\w*)\s*\[\s*(\d+)\s*\][ \t\r]*(?=\n|\Z)", re.M | re.I)
NUMBER = re.compile(r"-?\s*(?:\d+|(?:\d*\.\d+)(?:E[+-]?\d+)?|[1-9]\d*E[+-]?\d+)\Z")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def uncomment(text: str) -> str:
    # Preserve offsets/line numbers and apostrophes inside BAS double-quoted strings.
    return re.sub(r'"[^"\n]*"|\'[^\n]*',
                  lambda m: " " * len(m[0]) if m[0].startswith("'") else m[0], text)


def parse_parameters(text: str) -> dict:
    active = uncomment(text)
    result = {}
    end = 0
    for match in PARAM.finditer(active):
        if active[end:match.start()].strip():
            raise ValueError("invalid parameter syntax near line %d" % (active[:end].count("\n") + 1))
        c_type, name, body = match.groups()
        c_type = c_type.lower()
        if name in result:
            raise ValueError("duplicate parameter: " + name)
        values, literals = [], []
        for token in body.split(","):
            token = token.strip()
            if not NUMBER.fullmatch(token):
                raise ValueError("invalid BAS numeric literal for %s: %r" % (name, token))
            literal = re.sub(r"\s+", "", token)
            is_float = "." in literal or "E" in literal
            value = float(literal) if is_float else int(literal, 10)
            if c_type in ("char", "short", "int"):
                width = TYPES[c_type]
                if is_float or not -(1 << (width - 1)) <= value < (1 << width):
                    raise ValueError("%s value %s does not fit %s representation" % (name, literal, c_type))
                # C must not interpret decimal-leading-zero BAS values as octal.
                literal = str(value)
            elif not math.isfinite(value) or (c_type == "float" and abs(value) > 3.4028234663852886e38):
                raise ValueError("non-finite/out-of-range parameter: " + name)
            elif not is_float:
                literal = str(value)
            values.append(value)
            literals.append(literal)
        result[name] = {"type": c_type, "values": values, "literals": literals}
        end = match.end()
    if active[end:].strip():
        raise ValueError("only active 'parameter TYPE NAME = {numbers}' declarations are allowed")
    return result


def declaration(name: str, item: dict) -> str:
    return "parameter %s %s = {%s}\n" % (item["type"], name, ", ".join(item["literals"]))


def runtime_include(runtime: list[dict]) -> str:
    result = ["/* Generated from explicit BAS parameter inputs; do not edit. */\n"]
    for item in runtime:
        section = ".rfdata_data" if item["kind"] == "dfedata" else ".dag"
        result.append('static volatile __attribute__((section("%s"), aligned(64))) %s %s[%d] = {\n' % (
            section, item["type"], item["name"], item["capacity"]))
        vals = item["literals"]
        result.extend("  " + ", ".join(vals[i:i + 16]) + ",\n" for i in range(0, len(vals), 16))
        result.append("};\n\n")
    return "".join(result)


def materialize(bas: str, parameters: str) -> tuple[str, dict]:
    supplied = parse_parameters(parameters)
    active = uncomment(bas)
    inline = {}
    for m in PARAM.finditer(active):
        if m[2] in inline:
            raise ValueError("duplicate BAS parameter: " + m[2])
        inline[m[2]] = m
    runtime_specs = {}
    for m in RUNTIME.finditer(active):
        kind, c_type, name, capacity = m.groups()
        if name in runtime_specs or name in inline:
            raise ValueError("duplicate/conflicting BAS input declaration: " + name)
        runtime_specs[name] = {"name": name, "type": c_type.lower(), "kind": kind.lower(), "capacity": int(capacity)}
    replacements, additions, runtime, changes = [], [], [], []
    for name, item in supplied.items():
        if name in runtime_specs:
            spec = runtime_specs[name]
            if item["type"] != spec["type"]:
                raise ValueError("runtime input type mismatch: " + name)
            if len(item["values"]) > spec["capacity"]:
                raise ValueError("runtime input exceeds BAS capacity: " + name)
            runtime.append(dict(spec, **item))
            changes.append({"name": name, "action": "runtime_input", "elements": len(item["values"]), "capacity": spec["capacity"]})
        elif name in inline:
            m = inline[name]
            if m[1].lower() != item["type"]:
                raise ValueError("BAS parameter type mismatch: " + name)
            replacements.append((m.start(), m.end(), declaration(name, item).rstrip("\n")))
            changes.append({"name": name, "action": "override_inline", "elements": len(item["values"])})
        else:
            if re.search(r"\b(?:global|return_value)\s+\w+\s+" + re.escape(name) + r"\b", active, re.I):
                raise ValueError("cannot replace global/return_value with parameter: " + name)
            if not re.search(r"\b" + re.escape(name) + r"\b", active):
                raise ValueError("external parameter is not referenced by BAS: " + name)
            additions.append(declaration(name, item))
            changes.append({"name": name, "action": "external_definition", "elements": len(item["values"])})
    if runtime and {p["name"] for p in runtime} != set(runtime_specs):
        missing = sorted(set(runtime_specs) - {p["name"] for p in runtime})
        raise ValueError("external runtime input set must be complete; missing: " + ", ".join(missing))
    # No active external values means an exact no-op (commented test vectors stay inert).
    if not supplied:
        return bas, {"changes": [], "runtime": []}
    merged = active
    for start, end, text in sorted(replacements, reverse=True):
        merged = merged[:start] + text + merged[end:]
    return "".join(additions) + merged, {"changes": changes, "runtime": runtime}


def read_binding(dag_json: Path, dag_bin: Path) -> dict | None:
    """Recognize only a digest-bound sibling emitted by the compile adapter."""
    path = dag_json.parent / "parameters.json"
    if not path.is_file():
        dag = json.loads(dag_json.read_text())
        if isinstance(dag, list) and any(isinstance(task, dict) and task.get("_ace_echo_parameters") for task in dag):
            raise ValueError("missing parameters.json for externally initialized DAG; preserve the compile bundle")
        return None
    binding = json.loads(path.read_text())
    if binding.get("schema") != "ace-echo-bas-parameters/v1":
        raise ValueError("unsupported parameter binding: " + str(path))
    if binding.get("dag_json_sha256") != sha256(dag_json.read_bytes()) or binding.get("dag_bin_sha256") != sha256(dag_bin.read_bytes()):
        raise ValueError("external parameters do not match the staged DAG artifacts")
    return binding
