#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Rewrite Venus vector typedef aliases before clang so __vNNNiM matches ext_vector_type.

The LLVM Venus backend derives vector lane/layout metadata from the numeric prefix
in identifiers matching __v<lanes>i<bits>. That must agree with
__attribute__((ext_vector_type(N))) where N is the element count and element width
comes from the typedef base type (char -> i8, short -> i16, etc.).

Application source may keep legacy names like __v2048i16 with ext_vector_type(5000);
this pass renames to __v5000i16 everywhere in the translation unit feed to clang.
DSL tools that read the original .c on disk (scalar_o, type_verify) keep using
ext_vector_type-based rules and are unaffected.

Keep scalar size table aligned with scalar_o.get_scalar_type_size.
"""
import re
import sys
from typing import List, Optional

_SCALAR_ELEM_BYTES = {
    "char": 1,
    "unsigned char": 1,
    "signed char": 1,
    "short": 2,
    "unsigned short": 2,
    "short int": 2,
    "int": 4,
    "unsigned int": 4,
    "unsigned": 4,
    "long": 4,
    "unsigned long": 4,
    "float": 4,
    "double": 8,
    "long double": 16,
    "long long": 8,
    "unsigned long long": 8,
    "uint8_t": 1,
    "int8_t": 1,
    "uint16_t": 2,
    "int16_t": 2,
    "uint32_t": 4,
    "int32_t": 4,
    "uint64_t": 8,
    "int64_t": 8,
}


def _elem_bits_for_base(base_type: str) -> Optional[int]:
    sz = _SCALAR_ELEM_BYTES.get(base_type.strip())
    if sz is None:
        return None
    return sz * 8


# Same structural pattern as scalar_o.parse_vector_typedef_sizes (single-line typedef).
_TYPEDEF_EXT_VEC_RE = re.compile(
    r"typedef\s+([A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)*)\s+([A-Za-z_]\w*)\s+__attribute__\s*"
    r"\(\(\s*ext_vector_type\s*\(\s*(\d+)\s*\)\s*\)\)\s*;",
    re.MULTILINE,
)

def normalize_venus_ext_vector_source(text: str) -> str:
    """
    For each matching typedef, if the alias does not equal __v{N}i{elem_bits},
    replace all whole-word occurrences of the old alias with the canonical name.
    """
    repl: dict[str, str] = {}
    tokens = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/'
    declarations = re.sub(tokens, ' ', text, flags=re.DOTALL)
    for m in _TYPEDEF_EXT_VEC_RE.finditer(declarations):
        base_type, alias, n_s = m.group(1), m.group(2), m.group(3)
        n = int(n_s)
        bits = _elem_bits_for_base(base_type)
        if bits is None:
            continue
        canonical = f"__v{n}i{bits}"
        if alias != canonical:
            repl[alias] = canonical

    if not repl:
        return text
    # If two different aliases map to the same canonical name, the file would be
    # invalid C after rewrite; keep the last mapping and let the compiler fail clearly.
    # Rewrite identifiers simultaneously, preserving string/comment contents.
    pattern = tokens + r'|\b(?:' + '|'.join(re.escape(name) for name in repl) + r')\b'
    return re.sub(pattern, lambda m: repl.get(m[0], m[0]), text, flags=re.DOTALL)


def main(argv: List[str]) -> int:
    if len(argv) != 2:
        print("usage: venus_ext_vector_normalize.py <file.c>", file=sys.stderr)
        return 2
    path = argv[1]
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        src = f.read()
    sys.stdout.write(normalize_venus_ext_vector_source(src))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
