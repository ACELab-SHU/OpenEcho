from __future__ import annotations

import json
from pathlib import Path
import re
from typing import Any

from .output_validity import compare_output_bits, file_digest, read_layouts
from .output_layout_contract import discover_layouts


_RETURN_HEADER = re.compile(
    r"len:\s*([0-9a-fA-F]+).*?tid:\s*(\d+).*?"
    r"tname:\s*([^|]+?)\s*\|\s*retid:\s*(\d+)"
)
_DATA_LINE = re.compile(r"^[0-9a-fA-FxXzZ]+$")
_NATIVE_RETURN_HEADER = re.compile(
    r"len:\s*([0-9a-fA-F]+).*?ret_source_tile:\s*\d+.*?"
    r"task_id:\s*(\d+).*?retid:\s*(\d+)"
)
_RAW_DMA_HEADER = re.compile(
    r"src:\s*([0-9a-fA-F]+).*?dst:\s*([0-9a-fA-F]+).*?"
    r"len:\s*([0-9a-fA-F]+)"
)


def _expected_returns(path: Path) -> list[dict[str, Any]]:
    lines = path.read_text(encoding="utf-8", errors="strict").splitlines()
    records: list[dict[str, Any]] = []
    index = 0
    while index < len(lines):
        match = _RETURN_HEADER.search(lines[index])
        native = False
        if match is None:
            match = _NATIVE_RETURN_HEADER.search(lines[index])
            native = match is not None
        if match is None:
            index += 1
            continue
        length = int(match.group(1), 16)
        task_id = int(match.group(2))
        task_name = f"task_{task_id}" if native else match.group(3).strip()
        port = int(match.group(3) if native else match.group(4))
        index += 1
        while index < len(lines) and lines[index].strip() != "data:":
            index += 1
        if index == len(lines):
            raise ValueError(
                f"missing data section for task {task_id} port {port}"
            )
        index += 1
        values: list[int | None] = []
        raw_values: list[str] = []
        while index < len(lines) and _DATA_LINE.fullmatch(
                lines[index].strip()):
            text = lines[index].strip()
            if len(text) % 2:
                raise ValueError(f"odd hex digit count at line {index + 1}")
            # The reference recorder prints each 512-bit DMA beat as %h.
            # Convert it back to ascending byte-address order.
            tokens = [text[offset:offset + 2]
                      for offset in range(0, len(text), 2)]
            for token in reversed(tokens):
                raw_values.append(token)
                values.append(
                    None if re.search('[xXzZ]', token) else int(token, 16)
                )
            index += 1
        if len(values) < length:
            raise ValueError(
                f"task {task_id} port {port} has {len(values)} recorded "
                f"bytes but declares {length}"
            )
        records.append({
            "task_id": task_id,
            "task_name": task_name,
            "port": port,
            "length": length,
            "values": values[:length],
            "raw_values": raw_values[:length],
        })
    if not records:
        raise ValueError(f"no task return records found in {path}")
    return records


def _raw_dma_records(path: Path) -> list[dict[str, Any]]:
    """Decode unannotated Venus RTL DMA records.

    Older Venus1 testbenches record source, destination, length and payload,
    but do not include the task/return-port identity emitted by the newer
    Scheduler recorder.  Keep this parser separate from the identity join so
    the raw evidence remains immutable and independently auditable.
    """
    lines = path.read_text(encoding="utf-8", errors="strict").splitlines()
    records: list[dict[str, Any]] = []
    index = 0
    while index < len(lines):
        match = _RAW_DMA_HEADER.search(lines[index])
        if match is None:
            index += 1
            continue
        source = int(match.group(1), 16)
        destination = int(match.group(2), 16)
        length = int(match.group(3), 16)
        index += 1
        while index < len(lines) and lines[index].strip() != "data:":
            index += 1
        if index == len(lines):
            raise ValueError(
                f"missing data section for DMA {source:#x}->{destination:#x}"
            )
        index += 1
        values: list[int | None] = []
        raw_values: list[str] = []
        while index < len(lines) and _DATA_LINE.fullmatch(
                lines[index].strip()):
            text = lines[index].strip()
            if len(text) % 2:
                raise ValueError(f"odd hex digit count at line {index + 1}")
            tokens = [text[offset:offset + 2]
                      for offset in range(0, len(text), 2)]
            for token in reversed(tokens):
                raw_values.append(token)
                values.append(
                    None if re.search('[xXzZ]', token) else int(token, 16)
                )
            index += 1
        if len(values) < length:
            raise ValueError(
                f"DMA {source:#x}->{destination:#x} has {len(values)} "
                f"recorded bytes but declares {length}"
            )
        records.append({
            "source": source,
            "destination": destination,
            "length": length,
            "values": values[:length],
            "raw_values": raw_values[:length],
        })
    if not records:
        raise ValueError(f"no DMA records found in {path}")
    return records


def _legacy_returns_from_trace(
        dma_path: Path, trace_path: Path,
        local_address_mask: int = 0x1fffff) -> list[dict[str, Any]]:
    """Join legacy RTL returns with semantic Gem5 task/port identities.

    The join uses only ordered architectural facts that both simulators
    expose: local source address and transfer length.  It never uses a DAG
    name, task PC, payload value or timing, so the same mechanism applies to
    every Venus1 DAG produced by the backend.
    """
    templates: list[dict[str, Any]] = []
    for line_number, line in enumerate(
            trace_path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        event = json.loads(line)
        event_name = event.get("event")
        if event_name not in ("dma_output", "return_admit"):
            continue
        port_field = "output_port" if event_name == "dma_output" else "retid"
        templates.append({
            "task_id": int(event["task_id"]),
            "task_name": str(event["task_name"]),
            "port": int(event[port_field]),
            "source": int(str(event["source"]), 0),
            "length": int(event["bytes"]),
            "trace_line": line_number,
        })
    if not templates:
        raise ValueError(
            f"no dma_output or return_admit events found in {trace_path}"
        )

    raw = _raw_dma_records(dma_path)
    records: list[dict[str, Any]] = []
    raw_index = 0
    for template in templates:
        match = None
        while raw_index < len(raw):
            candidate = raw[raw_index]
            raw_index += 1
            if (
                candidate["length"] == template["length"]
                and (candidate["source"] & local_address_mask)
                    == (template["source"] & local_address_mask)
            ):
                match = candidate
                break
        if match is None:
            raise ValueError(
                "legacy DMA trace has no ordered return matching "
                f"task {template['task_id']} port {template['port']} "
                f"source {template['source']:#x} length "
                f"{template['length']}"
            )
        records.append({
            "task_id": template["task_id"],
            "task_name": template["task_name"],
            "port": template["port"],
            "length": template["length"],
            "values": match["values"],
            "raw_values": match["raw_values"],
            "rtl_source": f"{match['source']:#x}",
            "rtl_destination": f"{match['destination']:#x}",
            "trace_template_line": template["trace_line"],
        })
    return records


def _compare_expected_records(
        expected_records: list[dict[str, Any]], actual_dir: Path,
        expected_dma: Path, schema: str,
        trace_template: Path | None = None,
        output_layouts: Path | None = None) -> dict[str, Any]:
    keys = {(r['task_id'], r['port']) for r in expected_records}
    if len(keys) != len(expected_records):
        raise ValueError('duplicate task/port returns in reference trace')
    layouts = read_layouts(output_layouts, keys)
    results: list[dict[str, Any]] = []
    total_bytes = 0
    known_bytes = 0
    for expected in expected_records:
        task_id = expected["task_id"]
        port = expected["port"]
        path = actual_dir / f"task_{task_id}_port_{port}.bin"
        actual = path.read_bytes() if path.is_file() else b""
        values = expected["values"]
        validity = compare_output_bits(expected.get('raw_values', values), actual,
                                       layouts.get((task_id, port)))
        first_bit = validity['first_mismatch_bit']
        first_mismatch = first_bit//8 if first_bit is not None else None
        if first_mismatch is None and len(actual) != expected["length"]:
            first_mismatch = min(len(actual), expected["length"])
        status = validity['status']
        known = sum(value is not None for value in values)
        total_bytes += expected["length"]
        known_bytes += known
        result = {
            "task_id": task_id,
            "task_name": expected["task_name"],
            "port": port,
            "expected_length": expected["length"],
            "actual_length": len(actual),
            "known_expected_bytes": known,
            "status": status,
            "first_mismatch": first_mismatch,
            "expected_value": (
                values[first_mismatch]
                if first_mismatch is not None and first_mismatch < len(values)
                else None
            ),
            "actual_value": (
                actual[first_mismatch]
                if first_mismatch is not None and first_mismatch < len(actual)
                else None
            ),
            "actual": str(path.resolve()),
            "actual_sha256": file_digest(path) if path.is_file() else None,
            "validity": validity,
        }
        for field in ("rtl_source", "rtl_destination",
                      "trace_template_line"):
            if field in expected:
                result[field] = expected[field]
        results.append(result)
    passed = sum(result["status"] == "PASS" for result in results)
    report = {
        "schema": schema,
        "status": "PASS" if passed == len(results) else "FAIL",
        "expected_dma": str(expected_dma.resolve()),
        "actual_dir": str(actual_dir.resolve()),
        "outputs": len(results),
        "passed_outputs": passed,
        "expected_bytes": total_bytes,
        "known_expected_bytes": known_bytes,
        "results": results,
        "expected_dma_sha256": file_digest(expected_dma),
        "layout_sha256": file_digest(output_layouts) if output_layouts else None,
        "output_layouts": json.loads(output_layouts.read_text()) if output_layouts else None,
        "comparison_scope": "valid_output_bits" if output_layouts else "all_requested_bits",
        "warnings": [dict(w, task_id=r['task_id'], port=r['port'])
                     for r in results for w in r['validity']['warnings']],
        "transport_bit_exact_status": "PASS" if all(r['validity']['transport_bit_exact_status']=='PASS' for r in results) else "FAIL",
        "bus_assertions_evaluated": False,
    }
    if trace_template is not None:
        report["trace_template"] = str(trace_template.resolve())
    return report


def compare_dag_outputs(
        expected_dma: Path, actual_dir: Path,
        trace_template: Path | None = None,
        output_layouts: Path | None = None, *, rtl_log: Path | None = None,
        dag_json: Path | None = None, identity_contract: dict | None = None) -> dict[str, Any]:
    output_layouts = discover_layouts(dag_json, output_layouts)
    identity = None
    if any(item is not None for item in (rtl_log, dag_json, identity_contract)):
        if trace_template is not None or any(item is None for item in (rtl_log, dag_json, identity_contract)):
            raise ValueError('RTL identity reconciliation requires log, DAG and contract, without legacy template')
        from .rtl_return_identity import resolve_return_identity
        records, identity = resolve_return_identity(expected_dma, rtl_log, dag_json, identity_contract)
        schema = 'ace-echo-reconciled-dag-output-compare/v1'
    elif trace_template is None:
        records = _expected_returns(expected_dma)
        schema = "ace-echo-dag-output-compare/v1"
    else:
        records = _legacy_returns_from_trace(expected_dma, trace_template)
        schema = "ace-echo-legacy-dag-output-compare/v1"
    report = _compare_expected_records(
        records, actual_dir, expected_dma, schema, trace_template, output_layouts
    )
    if identity is not None:
        report['identity'] = identity
    return report


def compare_dma_returns(expected_dma: Path, actual_dma: Path,
                        output_layouts: Path | None = None) -> dict[str, Any]:
    """Compare two RTL DMA-return traces by task and return-port identity."""
    expected_records = _expected_returns(expected_dma)
    actual_records = _expected_returns(actual_dma)

    def index(records: list[dict[str, Any]], label: str):
        result: dict[tuple[int, int], dict[str, Any]] = {}
        duplicates: list[dict[str, int]] = []
        for record in records:
            key = (record["task_id"], record["port"])
            if key in result:
                duplicates.append({"task_id": key[0], "port": key[1]})
            else:
                result[key] = record
        if duplicates:
            raise ValueError(f"duplicate {label} DMA returns: {duplicates}")
        return result

    expected = index(expected_records, "expected")
    actual = index(actual_records, "actual")
    layouts = read_layouts(output_layouts, set(expected))
    results: list[dict[str, Any]] = []
    expected_bytes = 0
    known_expected_bytes = 0
    compared_known_bytes = 0

    for key, reference in expected.items():
        observed = actual.get(key)
        values = reference["values"]
        expected_bytes += reference["length"]
        known = sum(value is not None for value in values)
        known_expected_bytes += known
        first_mismatch = None
        reason = None
        validity = compare_output_bits(reference.get('raw_values', values),
                                       observed.get('raw_values', observed['values']) if observed else [],
                                       layouts.get(key))
        if observed is None:
            reason = "missing_return"
        elif observed["length"] != reference["length"]:
            reason = "length_mismatch"
        else:
            bit = validity['first_mismatch_bit']
            first_mismatch = bit//8 if bit is not None else None
            if validity['status'] != 'PASS':
                reason = 'valid_bit_mismatch' if validity['lengths_exact'] else 'layout_length_mismatch'
            compared_known_bytes += sum(a is not None and b == a
                                        for a,b in zip(values,observed['values']))
        results.append({
            "task_id": key[0],
            "task_name": reference["task_name"],
            "port": key[1],
            "expected_length": reference["length"],
            "actual_length": observed["length"] if observed else None,
            "known_expected_bytes": known,
            "status": "PASS" if reason is None else "FAIL",
            "reason": reason,
            "validity": validity,
            "first_mismatch": first_mismatch,
            "expected_value": (
                values[first_mismatch] if first_mismatch is not None else None
            ),
            "actual_value": (
                observed["values"][first_mismatch]
                if observed is not None and first_mismatch is not None else None
            ),
        })

    extra = [
        {"task_id": key[0], "port": key[1],
         "task_name": actual[key]["task_name"],
         "length": actual[key]["length"]}
        for key in actual.keys() - expected.keys()
    ]
    passed = sum(result["status"] == "PASS" for result in results)
    status = (
        "PASS" if passed == len(results) and not extra else "FAIL"
    )
    return {
        "schema": "ace-echo-dma-return-compare/v1",
        "status": status,
        "expected_dma": str(expected_dma.resolve()),
        "actual_dma": str(actual_dma.resolve()),
        "expected_returns": len(expected),
        "actual_returns": len(actual),
        "passed_returns": passed,
        "expected_bytes": expected_bytes,
        "known_expected_bytes": known_expected_bytes,
        "compared_known_bytes": compared_known_bytes,
        "extra_returns": extra,
        "results": results,
        "expected_dma_sha256": file_digest(expected_dma),
        "actual_dma_sha256": file_digest(actual_dma),
        "layout_sha256": file_digest(output_layouts) if output_layouts else None,
        "output_layouts": json.loads(output_layouts.read_text()) if output_layouts else None,
        "comparison_scope": "valid_output_bits" if output_layouts else "all_requested_bits",
        "warnings": [dict(w, task_id=r['task_id'], port=r['port'])
                     for r in results for w in r['validity']['warnings']],
        "transport_bit_exact_status": "PASS" if not extra and all(r['validity']['transport_bit_exact_status']=='PASS' for r in results) else "FAIL",
        "bus_assertions_evaluated": False,
    }


def write_dag_output_result(report: dict[str, Any], output: Path | None) -> str:
    payload = json.dumps(report, indent=2) + "\n"
    if output is not None:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(payload, encoding="utf-8")
    return payload
