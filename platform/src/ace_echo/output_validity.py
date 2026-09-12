"""Domain-independent valid-bit comparison with separate padding warnings."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re


def file_digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def layout_masks(layout: dict) -> list[int]:
    if not isinstance(layout, dict) or set(layout) != {
            'schema', 'transport_bytes', 'valid_bit_ranges'}:
        raise ValueError('layout requires schema, transport_bytes and valid_bit_ranges only')
    if layout['schema'] != 'ace-echo-output-layout/v1':
        raise ValueError('unsupported output layout schema')
    length = layout['transport_bytes']
    if type(length) is not int or length <= 0:
        raise ValueError('transport_bytes must be positive')
    ranges = layout['valid_bit_ranges']
    if not isinstance(ranges, list) or not ranges:
        raise ValueError('valid_bit_ranges must not be empty')
    end = 0
    # Validate before allocation. Bit 0 is the low bit of byte at offset 0.
    for pair in ranges:
        if not isinstance(pair, list) or len(pair) != 2 or any(type(v) is not int for v in pair):
            raise ValueError('valid_bit_ranges entries must be [bit_offset, bit_count]')
        start, count = pair
        if start < end or count <= 0 or start+count > length*8:
            raise ValueError('ranges must be ordered, disjoint and within the transfer')
        end = start+count
    masks = [0]*length
    for start, count in ranges:
        for byte in range(start//8, (start+count+7)//8):
            lo = max(start-byte*8, 0)
            hi = min(start+count-byte*8, 8)
            masks[byte] |= ((1 << (hi-lo))-1) << lo
    return masks


def _byte(value) -> tuple[int, int]:
    """Return value/known-bit mask, retaining known nibbles next to X/Z."""
    if value is None:
        return 0, 0
    if type(value) is int and 0 <= value <= 255:
        return value, 255
    if isinstance(value, str) and re.fullmatch(r'[0-9a-fA-FxXzZ]{2}', value):
        number = known = 0
        for digit in value:
            number <<= 4
            known <<= 4
            if digit.lower() not in 'xz':
                number |= int(digit, 16)
                known |= 15
        return number, known
    raise ValueError('output byte must be 0..255, None, or two hex/X/Z digits')


def compare_output_bits(expected, actual, layout: dict | None = None) -> dict:
    length = layout['transport_bytes'] if isinstance(layout, dict) and 'transport_bytes' in layout else len(expected)
    masks = layout_masks(layout) if layout is not None else [255]*length
    lhs = [_byte(v) for v in expected]
    rhs = [_byte(v) for v in actual]
    lengths_exact = len(lhs) == len(rhs) == length
    first_valid = first_transport = None
    counts = {k: 0 for k in ('valid_mismatch_bits', 'unknown_valid_bits',
                            'padding_unknown_bits', 'padding_nonzero_bits', 'padding_difference_bits')}
    for i, mask in enumerate(masks):
        if i >= len(lhs) or i >= len(rhs):
            continue  # Missing data is a length error, not evidence of X.
        a, ka = lhs[i]
        b, kb = rhs[i]
        unknown = (~(ka & kb)) & 255
        different = (a ^ b) & ka & kb
        invalid = (unknown | different) & mask
        padding = (~mask) & 255
        counts['valid_mismatch_bits'] += bin(different & mask).count('1')
        counts['unknown_valid_bits'] += bin(unknown & mask).count('1')
        counts['padding_unknown_bits'] += bin(unknown & padding).count('1')
        counts['padding_nonzero_bits'] += bin(((a & ka) | (b & kb)) & padding).count('1')
        counts['padding_difference_bits'] += bin(different & padding).count('1')
        if invalid and first_valid is None:
            first_valid = i*8 + (invalid & -invalid).bit_length()-1
        raw_invalid = unknown | different
        if raw_invalid and first_transport is None:
            first_transport = i*8 + (raw_invalid & -raw_invalid).bit_length()-1
    warnings = [{'code': code, 'bit_count': counts[key]} for key,code in (
        ('padding_unknown_bits','PADDING_UNKNOWN'),
        ('padding_nonzero_bits','PADDING_NONZERO'),
        ('padding_difference_bits','PADDING_DIFFERENCE')) if counts[key]]
    passed = lengths_exact and first_valid is None
    return {
        'schema':'ace-echo-output-validity/v1',
        'status':'PASS' if passed else 'FAIL',
        'comparison_scope':'valid_output_bits' if layout is not None else 'all_requested_bits',
        'transport_bit_exact_status':'PASS' if lengths_exact and first_transport is None else 'FAIL',
        'expected_bytes':len(lhs), 'actual_bytes':len(rhs), 'transport_bytes':length,
        'valid_bits':sum(bin(m).count('1') for m in masks),
        'lengths_exact':lengths_exact, 'first_mismatch_bit':first_valid,
        'warnings':warnings, **counts,
    }


def read_layouts(path: Path | None, keys: set[tuple[int, int]]) -> dict:
    if path is None:
        return {}
    document = json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(document, dict) or set(document) != {'schema','outputs'} or document['schema'] != 'ace-echo-output-layouts/v1':
        raise ValueError('expected ace-echo-output-layouts/v1 with outputs')
    if not isinstance(document['outputs'], list) or not document['outputs']:
        raise ValueError('outputs must be a nonempty list')
    result = {}
    for item in document['outputs']:
        if not isinstance(item, dict) or set(item) != {'task_id','port','layout'}:
            raise ValueError('output entry requires task_id, port, layout')
        if any(type(item[k]) is not int or item[k] < 0 for k in ('task_id','port')):
            raise ValueError('invalid task/port identifier')
        key = (item['task_id'],item['port'])
        if key not in keys or key in result:
            raise ValueError('unknown or duplicate layout output identity')
        layout_masks(item['layout'])
        result[key] = item['layout']
    return result


def compare_binary_output(expected: Path, actual: Path, layout_path: Path) -> dict:
    layout = json.loads(layout_path.read_text(encoding='utf-8'))
    result = compare_output_bits(expected.read_bytes(),actual.read_bytes(),layout)
    result.update(expected=str(expected.resolve()), actual=str(actual.resolve()),
                  layout_sha256=file_digest(layout_path), expected_sha256=file_digest(expected),
                  actual_sha256=file_digest(actual), layout=layout)
    return result
