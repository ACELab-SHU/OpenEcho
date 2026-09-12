#!/usr/bin/env python3
"""Convert VEMU nrPDCCH TV9 DAGRet logs into C byte-array goldens."""

import argparse
from pathlib import Path


# Order and lengths are the nrPDCCH_2p0 JSON return_output ABI.
SPECS = (
    ("pdcchbits", 128),
    ("dci", 64),
    ("crc_result", 2),
    ("pdsch_config", 1984),
    ("pdsch_start_symbol", 2),
    ("pdsch_symbol_length", 2),
)


def parse_bytes(path):
    values = []
    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        text = line.strip()
        if not text:
            continue
        try:
            value = int(text, 0)
        except ValueError as error:
            raise ValueError("%s:%d is not a byte: %r" %
                             (path, line_number, text)) from error
        if 0 <= value <= 0xff:
            values.append(value)
            continue
        # VEMU prints signed char values through a 32-bit hexadecimal path,
        # e.g. char(-1) becomes 0xffffffff.  A DAGRet line still represents
        # one output byte, so accept only a valid sign-extension and retain
        # its raw low-byte representation.
        if 0 <= value <= 0xffffffff and (value & 0xffffff00) == 0xffffff00:
            values.append(value & 0xff)
            continue
        raise ValueError("%s:%d byte out of range: %d" %
                         (path, line_number, value))
    return values


def format_initializer(values):
    return "\n".join(
        "  " + ", ".join("0x%02x" % value for value in values[start:start + 16]) + ","
        for start in range(0, len(values), 16)
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-dir", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    generated = [
        "/* Auto-generated from VEMU DAGRet logs for nrPDCCH_2p0 TV9. */\n",
        "/* Output order follows the JSON return_output ABI. */\n\n",
    ]
    for name, expected_length in SPECS:
        path = args.input_dir / ("DAGRet_%s.log" % name)
        values = parse_bytes(path)
        if len(values) != expected_length:
            raise ValueError("%s has %d bytes; expected %d" %
                             (path, len(values), expected_length))
        generated.append(
            "static const unsigned char %s_expected[%d] = {\n%s\n};\n\n" %
            (name, expected_length, format_initializer(values))
        )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(generated))


if __name__ == "__main__":
    main()
