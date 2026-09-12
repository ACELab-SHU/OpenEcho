#!/usr/bin/env python3
"""Materialize an nrPDCCH test-vector parameter file as L1 C storage."""

import argparse
import sys
from pathlib import Path

# ACE-ECHO stages the same canonical parser beside this script in isolated
# builds. Direct builds inside the platform can import it from src instead.
try:
    from bas_parameters import parse_parameters as parse_bas_parameters
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "src"))
    from ace_echo.bas_parameters import parse_parameters as parse_bas_parameters


# This is the L1 DMA ABI of nrPDCCH_2p0, not the declaration order in BAS.
SPECS = (
    ("dfe_input_0", "char", 4420, ".rfdata_data"),
    ("dfe_input_1", "char", 4420, ".rfdata_data"),
    ("dfe_input_2", "char", 4420, ".rfdata_data"),
    ("scsSSB", "short", 1, ".dag"),
    ("symbolNum0", "short", 1, ".dag"),
    ("symbolNum1", "short", 1, ".dag"),
    ("symbolNum2", "short", 1, ".dag"),
    ("cch_nrb", "short", 1, ".dag"),
    ("csetNRB", "short", 1, ".dag"),
    ("csetSubcarriers", "short", 2048, ".dag"),
    ("csetDuration", "short", 1, ".dag"),
    ("pdcch_config", "char", 192, ".dag"),
    ("c0Carrier", "char", 64, ".dag"),
    ("aLevIdx", "short", 1, ".dag"),
    ("cIdx", "short", 1, ".dag"),
    ("numCand", "short", 1, ".dag"),
    ("freq0", "char", 8, ".dag"),
    ("initialInfo", "char", 192, ".dag"),
    ("csetPattern", "char", 1, ".dag"),
)

def parse_parameters(text):
    parsed = parse_bas_parameters(text)
    specs = {name: c_type for name, c_type, _, _ in SPECS}
    for name, item in parsed.items():
        if name not in specs:
            raise ValueError("unknown TV9 parameter: " + name)
        if item["type"] != specs[name]:
            raise ValueError("TV9 parameter type mismatch: " + name)
    return {name: item["values"] for name, item in parsed.items()}


def check_range(name, c_type, values):
    lower, upper = (-128, 255) if c_type == "char" else (-32768, 65535)
    for value in values:
        if value < lower or value > upper:
            raise ValueError(
                "%s value %d does not fit the %s byte representation"
                % (name, value, c_type)
            )


def format_initializer(values):
    if not values:
        return ""
    lines = []
    for start in range(0, len(values), 16):
        lines.append("  " + ", ".join(str(value) for value in values[start:start + 16]) + ",")
    return "\n".join(lines)


def emit_declaration(name, c_type, capacity, section, values):
    initializer = format_initializer(values)
    if initializer:
        initializer += "\n"
    return (
        "static volatile __attribute__((section(\"%s\"), aligned(64))) %s %s[%d] = {\n"
        "%s"
        "};\n\n"
    ) % (section, c_type, name, capacity, initializer)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    values_by_name = parse_parameters(args.input.read_text())
    generated = [
        "/* Auto-generated from %s. Do not edit by hand. */\n\n" % args.input.name
    ]

    for name, c_type, capacity, section in SPECS:
        if name not in values_by_name:
            raise ValueError("missing required Test Vector field: %s" % name)
        values = values_by_name[name]
        if len(values) > capacity:
            raise ValueError(
                "%s has %d values, but the ABI capacity is %d"
                % (name, len(values), capacity)
            )
        check_range(name, c_type, values)
        generated.append(emit_declaration(name, c_type, capacity, section, values))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(generated))


if __name__ == "__main__":
    main()
