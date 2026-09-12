"""Associate immutable native DMA payloads with acknowledged RTL requests.

The supported protocol has one serialized L2 DMA. A request acceptance and
its DMA start must be in the same interval before the next accepted request.
Never select an identity by payload similarity or a fixed time offset.
"""
from __future__ import annotations
from bisect import bisect_right
from decimal import Decimal
import json
from pathlib import Path
import re

from .output_validity import file_digest
from .output_capacity import output_capacity

_SCALE = dict(fs=1, ps=1000, ns=1000000, us=1000000000, ms=1000000000000, s=1000000000000000)
_STAMP = r'([0-9]+(?:\.[0-9]+)?)\s*(fs|ps|ns|us|ms|s)'
_ACK = re.compile(r'^\[\s*' + _STAMP + r'\]\s+arbiter (recycle|firing) task: dag\s+(\d+) task\s+(\d+) '
                  r'(?:processing result received from|fired to) tile\s+(\d+)\. data_count:\s*(\d+)\s*$')
_HEADER = re.compile(r'^src:\s*([0-9a-fA-F]+)\s*\|\s*dst:\s*([0-9a-fA-F]+)\s*\|\s*len:\s*([0-9a-fA-F]+).*?\|\s*time:\s*' + _STAMP + r'\s*$')
_NATIVE = re.compile(r'ret_source_tile:\s*(\d+)\s*\|\s*task_id:\s*(\d+)\s*\|\s*retid:\s*(\d+)')


def _time(number, unit):
    return Decimal(number) * _SCALE[unit]


def resolve_return_identity(dma_path: Path, log_path: Path, dag_path: Path,
                            contract: dict) -> tuple[list[dict], dict]:
    from .dag_output_compare import _expected_returns
    if set(contract) != {'protocol', 'tile_address_base', 'tile_address_stride', 'tile_count'} or contract['protocol'] != 'venus-serial-ack-v1':
        raise ValueError('unsupported RTL return identity contract')
    base, stride, count = (int(str(contract[k]), 0) for k in ('tile_address_base', 'tile_address_stride', 'tile_count'))
    if base < 0 or stride <= 0 or count <= 0:
        raise ValueError('invalid tile address geometry')
    # UART/AXI debug dumps can contain arbitrary bytes between ASCII events.
    # Preserve them losslessly; malformed event lines still fail coverage.
    log = log_path.read_text(encoding='utf-8', errors='surrogateescape')
    if 'Test complete!' not in log:
        raise ValueError('return identity requires a completed native simulation')
    acks = []
    for line_no, line in enumerate(log.splitlines(), 1):
        match = _ACK.fullmatch(line)
        if match:
            acks.append(dict(time=_time(match[1], match[2]), kind=match[3], dag=int(match[4]),
                             task=int(match[5]), tile=int(match[6]), port=int(match[7])-1, line=line_no))
    if not acks or any(a['time'] >= b['time'] for a,b in zip(acks, acks[1:])):
        raise ValueError('missing or ambiguous request acceptance timeline')
    times = [a['time'] for a in acks]
    headers = []
    for line_no, line in enumerate(dma_path.read_text(errors='strict').splitlines(), 1):
        match = _HEADER.fullmatch(line)
        if match:
            native = _NATIVE.search(line)
            headers.append(dict(time=_time(match[4], match[5]), src=int(match[1],16),
                                dst=int(match[2],16), length=int(match[3],16), line=line_no,
                                raw_identity=tuple(map(int,native.groups())) if native else None))
    if not headers or any(a['time'] >= b['time'] for a,b in zip(headers,headers[1:])):
        raise ValueError('missing or unordered DMA starts')
    slots = {}
    for h in headers:
        index = bisect_right(times, h['time']) - 1
        if index < 0 or index in slots:
            raise ValueError('DMA start has no unique accepted request')
        slots[index] = h
    # Reconcile every request, not only the two records that happen to differ.
    if len(slots) != len(acks):
        raise ValueError('accepted requests and DMA starts have incomplete coverage')
    raw = _expected_returns(dma_path)
    native_headers = [h for h in headers if h['raw_identity'] is not None]
    if len(native_headers) != len(raw):
        raise ValueError('native return payload/header count mismatch')
    raw_by_line = dict(zip((h['line'] for h in native_headers), raw))
    dag = json.loads(dag_path.read_text())
    if not isinstance(dag,list):
        raise ValueError('identity contract requires compiled task-list DAG metadata')
    expected = {}
    for task in dag:
        if 'current_taskId' not in task:
            continue
        if task['Output_Num'] != len(task['all_output']):
            raise ValueError('inconsistent DAG output count')
        for port, output in enumerate(task['all_output']):
            key = (task['current_taskId'], port)
            if key in expected:
                raise ValueError('duplicate compiled output identity')
            expected[key] = output
    corrected, joins, seen, bases, dag_ids = [], [], set(), set(), set()
    for index, ack in enumerate(acks):
        h = slots[index]
        if ack['kind'] != 'recycle':
            if h['raw_identity'] is not None:
                raise ValueError('return header falls in a fire-request interval')
            continue
        dag_ids.add(ack['dag'])
        key = (ack['task'], ack['port'])
        if key not in expected or key in seen:
            raise ValueError('unknown/repeated invocation or duplicate acknowledged return')
        if not 0 <= ack['tile'] < count or not base <= h['src'] < base + stride*count or (h['src'] - base)//stride != ack['tile']:
            raise ValueError('DMA source address disagrees with acknowledged tile')
        output = expected[key]
        # Use compiler-exported occupied allocation when present. Legacy DAGs
        # remain conservative; never infer padding capacity from DMA data.
        capacity = output_capacity(output)
        if h['length'] <= 0 or h['length'] > capacity:
            raise ValueError(f"DMA length exceeds compiled output capacity {key}: observed {h['length']}, capacity {capacity}; ack line {ack['line']}, DMA line {h['line']}")
        bases.add(h['dst'] - output['temp_offset'])
        record = raw_by_line.get(h['line'])
        if record is None:
            raise ValueError('accepted return lacks a native payload record')
        seen.add(key)
        corrected.append(dict(record, task_id=key[0], port=key[1], task_name=f'task_{key[0]}'))
        actual_id = (ack['tile'], key[0], key[1])
        joins.append(dict(ack_line=ack['line'], dma_line=h['line'], task_id=key[0], port=key[1],
                          dag_id=ack['dag'], tile=ack['tile'], source=hex(h['src']), destination=hex(h['dst']),
                          bytes=h['length'], raw_identity=list(h['raw_identity']),
                          identity_corrected=h['raw_identity'] != actual_id))
    if not expected or seen != set(expected) or len(corrected) != len(raw) or len(dag_ids) != 1:
        raise ValueError('single-invocation DAG output coverage is not exact')
    if len(bases) != 1 or next(iter(bases)) < 0:
        raise ValueError('DMA destinations disagree with compiled temporary layout')
    return corrected, dict(schema='ace-echo-rtl-return-identity/v1', status='PASS',
        scope='single-DAG single-invocation serialized DMA', outputs=len(corrected),
        corrected_labels=sum(j['identity_corrected'] for j in joins),
        inferred_consistent_temporary_base=hex(next(iter(bases))),
        method='acknowledgement intervals + source-tile geometry + compiled capacities/relative destinations',
        payload_used_for_identity=False, contract=contract, joins=joins,
        dma_sha256=file_digest(dma_path), rtl_log_sha256=file_digest(log_path), dag_sha256=file_digest(dag_path))
