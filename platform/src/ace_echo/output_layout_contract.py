"""Explicit workload output contracts bound to an immutable compiled DAG.

Never infer valid bits from values, unknown bytes, names, or vector capacity.
"""
from __future__ import annotations

import json
from pathlib import Path

from .output_capacity import output_capacity
from .output_validity import file_digest, layout_masks, read_layouts


def parse_contract(raw: bytes) -> dict:
    doc = json.loads(raw)
    if not isinstance(doc, dict) or set(doc) != {'schema', 'basis', 'outputs'}:
        raise ValueError('output validity contract requires schema, basis, outputs')
    if doc['schema'] != 'ace-echo-output-contract/v1':
        raise ValueError('unsupported output validity contract')
    if not isinstance(doc['basis'], str) or not doc['basis'].strip():
        raise ValueError('output validity contract requires an ABI basis')
    if not isinstance(doc['outputs'], list) or not doc['outputs']:
        raise ValueError('output validity contract outputs must be nonempty')
    seen = set()
    for item in doc['outputs']:
        if not isinstance(item, dict) or set(item) != {'task', 'output', 'layout'}:
            raise ValueError('contract output requires task, output, layout')
        if any(not isinstance(item[k], str) or not item[k] for k in ('task', 'output')):
            raise ValueError('contract task/output names must be nonempty strings')
        key = (item['task'], item['output'])
        if key in seen:
            raise ValueError('duplicate output validity declaration')
        seen.add(key)
        layout_masks(item['layout'])
    return doc


def resolve_contract(doc: dict, dag: list) -> dict:
    outputs = []
    for item in doc['outputs']:
        matches = [(t, p, o) for t in dag if 'current_taskId' in t
                   and t.get('debug_task_name') == item['task']
                   for p, o in enumerate(t['all_output']) if o.get('name') == item['output']]
        if len(matches) != 1:
            raise ValueError('output contract identity missing or ambiguous: %s/%s' %
                             (item['task'], item['output']))
        task, port, output = matches[0]
        if item['layout']['transport_bytes'] > output_capacity(output):
            raise ValueError('output validity transport exceeds allocation capacity')
        outputs.append({'task_id': int(task['current_taskId']), 'port': port,
                        'layout': item['layout']})
    return {'schema': 'ace-echo-output-layouts/v1', 'outputs': outputs}


def export_contract(raw: bytes, dag_json: Path) -> Path:
    """Only write to a fresh compile/rejudgment directory, not old evidence."""
    doc = parse_contract(raw)
    layouts = resolve_contract(doc, json.loads(dag_json.read_text()))
    root = dag_json.parent
    contract = root / 'output-validity.json'
    layout = root / 'output-layouts.json'
    binding = root / 'output-layouts.binding.json'
    if any(p.exists() for p in (contract, layout, binding)):
        raise ValueError('output validity artifacts already exist')
    dag_bin = dag_json.with_suffix('.bin')
    # Check inputs before creating any artifacts.
    dag_hash, bin_hash = file_digest(dag_json), file_digest(dag_bin)
    contract.write_bytes(raw)
    layout.write_text(json.dumps(layouts, indent=2) + '\n')
    binding.write_text(json.dumps({
        'schema': 'ace-echo-output-layout-binding/v1',
        'dag_json_sha256': dag_hash, 'dag_bin_sha256': bin_hash,
        'layout_sha256': file_digest(layout), 'contract_sha256': file_digest(contract)
    }, indent=2) + '\n')
    return layout


def discover_layouts(dag_json: Path | None, explicit: Path | None = None) -> Path | None:
    """An explicit case layout wins; otherwise require a complete hash binding."""
    if explicit is not None or dag_json is None:
        return explicit
    root = dag_json.parent
    layout, binding, contract = (root / n for n in (
        'output-layouts.json', 'output-layouts.binding.json', 'output-validity.json'))
    if not any(p.exists() for p in (layout, binding, contract)):
        return None  # Legacy artifacts remain strict.
    if not all(p.is_file() for p in (layout, binding, contract)):
        raise ValueError('incomplete compiled output validity bundle')
    record = json.loads(binding.read_text())
    expected = {'schema': 'ace-echo-output-layout-binding/v1',
                'dag_json_sha256': file_digest(dag_json),
                'dag_bin_sha256': file_digest(dag_json.with_suffix('.bin')),
                'layout_sha256': file_digest(layout), 'contract_sha256': file_digest(contract)}
    if record != expected:
        raise ValueError('stale or modified compiled output validity bundle')
    dag = json.loads(dag_json.read_text())
    if json.loads(layout.read_text()) != resolve_contract(parse_contract(contract.read_bytes()), dag):
        raise ValueError('compiled layouts disagree with their source contract')
    keys = {(int(t['current_taskId']), p) for t in dag if 'current_taskId' in t
            for p, _ in enumerate(t['all_output'])}
    read_layouts(layout, keys)
    return layout
