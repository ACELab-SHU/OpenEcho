"""Narrow, source-digest-scoped classification; never suppress raw errors."""
from __future__ import annotations
from decimal import Decimal
import hashlib
from pathlib import Path
import re


_ERROR = re.compile(
    r'^Error: "(?P<path>[^"]+)", \d+: (?P<instance>[^:]+): '
    r'at time (?P<time>\d+(?:\.\d+)?(?:[eE][+-]?\d+)?) '
    r'(?P<unit>fs|ps|ns|us|ms|s)\s*$'
)


def classify_startup_errors(lines: list[str], rules: list[dict],
                            source_root: Path | None) -> list[dict]:
    if not rules or source_root is None:
        return []
    root = source_root.resolve()
    classified = []
    claimed = set()
    for rule in rules:
        required = {'id', 'source_file', 'source_sha256', 'instance',
                    'message', 'max_occurrences'}
        if set(rule) != required:
            raise ValueError('invalid startup diagnostic rule fields')
        relative = Path(rule['source_file'])
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('startup diagnostic source must be relative')
        source = (root / relative).resolve()
        if root not in source.parents:
            raise ValueError('startup diagnostic source escapes snapshot')
        if (type(rule['max_occurrences']) is not int or rule['max_occurrences'] < 1
                or not re.fullmatch(r'[0-9a-f]{64}', rule['source_sha256'])):
            raise ValueError('invalid startup diagnostic bound/digest')
        if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != rule['source_sha256']:
            continue
        matches = []
        for index, line in enumerate(lines):
            match = _ERROR.fullmatch(line)
            if (match is None or index + 1 == len(lines)
                    or index in claimed or Decimal(match['time']) != 0
                    or match['instance'] != rule['instance']
                    or Path(match['path']).resolve() != source
                    or lines[index + 1].strip() != rule['message']):
                continue
            matches.append(index)
        # Repeated startup errors are not silently accepted up to a quota.
        if len(matches) > rule['max_occurrences']:
            continue
        for index in matches:
            claimed.add(index)
            classified.append(dict(id=rule['id'], line=index + 1,
                                   classification='SOURCE_SCOPED_TIME_ZERO_WARNING',
                                   source_sha256=rule['source_sha256']))
    return classified
