from __future__ import annotations

from dataclasses import asdict, dataclass
import json
from pathlib import Path


@dataclass(frozen=True)
class CompareResult:
    schema: str
    status: str
    dtype: str
    expected: str
    actual: str
    expected_elements: int
    actual_elements: int
    first_mismatch: int | None
    expected_value: int | None
    actual_value: int | None


def _values(path: Path, dtype: str) -> list[int]:
    if dtype == "raw":
        return list(path.read_bytes())
    bits = int(dtype[1:])
    mask = (1 << bits) - 1
    text = path.read_text(encoding="utf-8")
    tokens = text.replace(",", " ").split()
    return [int(token, 0) & mask for token in tokens]


def compare_files(expected: Path, actual: Path, dtype: str) -> CompareResult:
    lhs = _values(expected, dtype)
    rhs = _values(actual, dtype)
    first = next((index for index, pair in enumerate(zip(lhs, rhs))
                  if pair[0] != pair[1]), None)
    if first is None and len(lhs) != len(rhs):
        first = min(len(lhs), len(rhs))
    return CompareResult(
        schema="ace-echo-bitexact/v1",
        status="PASS" if first is None else "FAIL",
        dtype=dtype,
        expected=str(expected.resolve()), actual=str(actual.resolve()),
        expected_elements=len(lhs), actual_elements=len(rhs),
        first_mismatch=first,
        expected_value=(lhs[first] if first is not None and first < len(lhs) else None),
        actual_value=(rhs[first] if first is not None and first < len(rhs) else None),
    )


def write_result(result: CompareResult, output: Path | None) -> str:
    payload = json.dumps(asdict(result), indent=2) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(payload, encoding="utf-8")
    return payload
