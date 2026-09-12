#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(PROJECT_ROOT / "src"))

from ace_echo.forge import (  # noqa: E402
    ForgeContractError,
    canonical_digest,
    validate_backend_manifest,
    validate_forge_request,
    validate_reference_adapter,
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("kind", choices=["request", "backend", "reference"])
    parser.add_argument("path", type=Path)
    parser.add_argument("--digest", action="store_true")
    args = parser.parse_args()
    try:
        if args.kind == "request":
            value, _ = validate_forge_request(args.path)
            value = {key: item for key, item in value.items()
                     if not key.startswith("_")}
        elif args.kind == "backend":
            value = validate_backend_manifest(args.path)
        else:
            value = validate_reference_adapter(args.path)
    except ForgeContractError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print("PASS")
    if args.digest:
        print(canonical_digest(value))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
