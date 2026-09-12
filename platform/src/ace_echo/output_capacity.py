"""Output memory safety, independent of semantic-bit/padding comparison.

Never infer allocation size from a neighbouring address or observed payload.
Legacy metadata without an allocation record retains its declared length as
the conservative capacity. Compiler IR checks every emitted return path;
nonconstant lengths remain explicitly pending until observed at runtime.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys


def _positive(value, name):
    if type(value) is not int or value <= 0:
        raise ValueError(f"invalid {name}: {value!r}")
    return value


def output_capacity(output):
    declared = _positive(output['length'], 'output length')
    capacity = _positive(output.get('allocation_bytes', declared), 'output capacity')
    if capacity < declared:
        raise ValueError('output allocation capacity is smaller than declared length')
    return capacity


def _tasks(dag):
    if not isinstance(dag, list):
        raise ValueError('output capacity requires compiled task-list metadata')
    seen = set()
    for task in dag:
        if 'current_taskId' not in task:
            continue
        tid = task['current_taskId']
        if tid in seen or task['Output_Num'] != len(task['all_output']):
            raise ValueError('duplicate task or inconsistent output count')
        seen.add(tid)
        yield task


def export_allocations(dag, memory_map):
    """Export the DSL row-exclusive allocator's occupied size, not sizeof(C).

    temp_memory_map.row_bytes describes allocation granularity. The legacy
    allocated_size_bytes field is the *unrounded* size; do not mistake it for
    occupied capacity. Exact name, offset and size joins are mandatory.
    """
    row = _positive(memory_map['row_bytes'], 'allocator row_bytes')
    total = _positive(memory_map['total_bytes'], 'temporary pool size')
    allocations = {}
    for item in memory_map['allocations']:
        if item['name'] in allocations:
            raise ValueError('duplicate allocator variable')
        allocations[item['name']] = item
    for task in _tasks(dag):
        for output in task['all_output']:
            item = allocations.get(output['name'])
            if item is None or item['size_bytes'] != output['length'] or item['temp_offset'] != output['temp_offset']:
                raise ValueError(f"output allocation join mismatch: {output['name']}")
            size = _positive(item['size_bytes'], 'allocated size')
            occupied = (size + row - 1) // row * row
            offset = item['temp_offset']
            if type(offset) is not int or offset < 0 or offset % row or offset + occupied > total:
                raise ValueError(f"output allocation outside/alignment mismatch: {output['name']}")
            output['allocation_bytes'] = occupied
    return dag


def _diagnostic(task, port, code, severity, **details):
    return dict(code=code, severity=severity, task_id=task['current_taskId'],
                task_name=task.get('debug_task_name'), port=port, **details)


def _length_check(task, port, length, runtime_dmt=False, **evidence):
    output = task['all_output'][port]
    capacity = output_capacity(output)
    if length < 0 or length > capacity:
        code = 'OUTPUT_EXCEEDS_STATIC_ESTIMATE' if runtime_dmt and length >= 0 else 'OUTPUT_CAPACITY_EXCEEDED'
        severity = 'WARNING' if runtime_dmt and length >= 0 else 'ERROR'
        return _diagnostic(task, port, code, severity,
                           declared_bytes=output['length'], allocation_bytes=capacity,
                           return_bytes=length, **evidence)
    if length == 0:
        return _diagnostic(task, port, 'OUTPUT_EMPTY_RETURN', 'WARNING',
                           declared_bytes=output['length'], allocation_bytes=capacity,
                           return_bytes=length, **evidence)
    if length != output['length']:
        return _diagnostic(task, port, 'OUTPUT_LENGTH_DIFFERS', 'WARNING',
                           declared_bytes=output['length'], allocation_bytes=capacity,
                           return_bytes=length, **evidence)
    return None


def write_report(path, diagnostics, **details):
    errors = [d for d in diagnostics if d['severity'] == 'ERROR']
    pending = any(d['code'] == 'RETURN_LENGTH_UNRESOLVED' for d in diagnostics)
    report = dict(schema='ace-echo-output-capacity/v1',
                  status='FAIL' if errors else ('PENDING_RUNTIME' if pending else 'PASS'),
                  diagnostics=diagnostics, **details)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n')
    for d in diagnostics:
        print(f"{d['severity']}: {d['code']} task={d.get('task_id')} port={d.get('port')} "
              f"return={d.get('return_bytes')} capacity={d.get('allocation_bytes')}", file=sys.stderr)
    if errors:
        first = errors[0]
        raise ValueError(f"{first['code']}: task {first.get('task_id')} port {first.get('port')}; "
                         f"output capacity validation failed; report: {path}")
    return report


_STORE = re.compile(r'store volatile i32\s+([^,\n]+),\s*(?:ptr|i32\*)\s+inttoptr\s*'
                    r'\(i32\s+(-?\d+)\s+to\s+(?:ptr|i32\*)\)')


def check_compiled_returns(dag, ir_dir, registers, report_path, runtime_dmt=False):
    diagnostics, sources = [], {}
    if registers is not None:
        if registers.get('format') != 'venus-mmio-return-ir-v1':
            raise ValueError('unsupported compiled return register ABI')
        base = int(registers['length_base'], 0)
        count_addr = int(registers['count_address'], 0)
        stride = _positive(registers['stride_bytes'], 'return register stride')
        ports = _positive(registers['port_count'], 'return register count')
        if not 0 <= base < 2**32 or not 0 <= count_addr < 2**32:
            raise ValueError('invalid return register address')
    for task in _tasks(dag):
        for output in task['all_output']:
            output_capacity(output)
        name = task.get('debug_task_name', '').split('*')[0]
        source = ir_dir / (name + '.0.ll')
        found, count_seen = set(), False
        if registers is not None and source.is_file() and source.parent.resolve() == ir_dir.resolve():
            raw = source.read_bytes()
            sources[str(source)] = hashlib.sha256(raw).hexdigest()
            text = raw.decode('utf-8')
            for match in _STORE.finditer(text):
                value, address = match[1].strip(), int(match[2]) & 0xffffffff
                line = text.count('\n', 0, match.start()) + 1
                if address == count_addr:
                    count_seen = True
                    if value != str(task['Output_Num']):
                        diagnostics.append(_diagnostic(task, None, 'OUTPUT_COUNT_MISMATCH', 'ERROR',
                                                       ir=str(source), line=line, actual=value,
                                                       expected=task['Output_Num']))
                if address < base or (address-base) % stride:
                    continue
                port = (address-base) // stride
                if port >= ports:
                    continue
                if port >= task['Output_Num']:
                    diagnostics.append(_diagnostic(task, port, 'OUTPUT_COUNT_MISMATCH', 'ERROR', ir=str(source), line=line))
                    continue
                found.add(port)
                if not re.fullmatch(r'-?\d+', value):
                    diagnostics.append(_diagnostic(task, port, 'RETURN_LENGTH_UNRESOLVED', 'WARNING', ir=str(source), line=line))
                else:
                    issue = _length_check(task, port, int(value) & 0xffffffff, runtime_dmt=runtime_dmt, ir=str(source), line=line)
                    if issue:
                        diagnostics.append(issue)
        for port in range(task['Output_Num']):
            if port not in found or not count_seen:
                diagnostics.append(_diagnostic(task, port, 'RETURN_LENGTH_UNRESOLVED', 'WARNING',
                                               reason='no recognized compiler return evidence'))
    return write_report(report_path, diagnostics, scope='all emitted constant return paths', ir_sha256=sources)


def check_observed_returns(dag_path: Path, output_dir: Path, report_path: Path, task_id=None, runtime_dmt=False):
    dag = json.loads(dag_path.read_text())
    diagnostics, checked, expected = [], [], set()
    for task in _tasks(dag):
        if task_id is not None and task['current_taskId'] != task_id:
            continue
        for port, output in enumerate(task['all_output']):
            expected.add((task['current_taskId'], port))
            path = output_dir / f"task_{task['current_taskId']}_port_{port}.bin"
            if not path.is_file():
                diagnostics.append(_diagnostic(task, port, 'OUTPUT_MISSING', 'ERROR', path=str(path)))
                continue
            length = path.stat().st_size
            issue = _length_check(task, port, length, runtime_dmt=runtime_dmt, path=str(path))
            if issue:
                diagnostics.append(issue)
            checked.append(dict(task_id=task['current_taskId'], port=port, return_bytes=length,
                                allocation_bytes=output_capacity(output)))
    # Venus1 copies the runtime DMT length without clamping to C capacity.
    # Static annotations can warn about an ABI risk, but cannot prove that
    # RTL corrupts live data. Legacy profiles retain their fixed-length gate.
    if task_id is None:
        for consumer in _tasks(dag):
            for edge in consumer.get('all_input', []):
                if 'consumer_capacity_bytes' not in edge:
                    continue
                capacity = edge['consumer_capacity_bytes']
                if type(capacity) is not int or capacity <= 0:
                    diagnostics.append(dict(code='INPUT_CAPACITY_UNRESOLVED', severity='WARNING',
                                            task_id=consumer['current_taskId']))
                    continue
                extent = capacity if runtime_dmt else _positive(edge.get('length', capacity), 'consumer transfer length')
                if not runtime_dmt and (extent > capacity or int(edge.get('type', '0'), 0) != 0):
                    raise ValueError('invalid consumer DMA extent')
                encoded = int(edge['parentTasksPort'], 0)
                parent, port = encoded >> 4, encoded & 15
                path = output_dir / f"task_{parent}_port_{port}.bin"
                if path.is_file() and path.stat().st_size > extent:
                    diagnostics.append(dict(
                        code='INPUT_EXCEEDS_STATIC_CAPACITY' if runtime_dmt else 'INPUT_PAYLOAD_EXCEEDS_TRANSFER',
                        severity='WARNING' if runtime_dmt else 'ERROR',
                        task_id=consumer['current_taskId'], task_name=consumer.get('debug_task_name'),
                        port=port, parent_task_id=parent, input_name=edge.get('name'),
                        return_bytes=path.stat().st_size,
                        transfer_bytes=(path.stat().st_size & 0xffff) if runtime_dmt else extent,
                        consumer_capacity_bytes=capacity, path=str(path)))
                if runtime_dmt and path.is_file() and path.stat().st_size > 0xffff:
                    diagnostics.append(dict(code='RTL_DMT_LENGTH_TRUNCATED', severity='WARNING',
                        task_id=consumer['current_taskId'], port=port, parent_task_id=parent,
                        return_bytes=path.stat().st_size, transfer_bytes=path.stat().st_size & 0xffff))
    for path in output_dir.glob('task_*_port_*.bin'):
        match = re.fullmatch(r'task_(\d+)_port_(\d+)\.bin', path.name)
        if match:
            key = tuple(map(int, match.groups()))
            if (task_id is None or key[0] == task_id) and key not in expected:
                diagnostics.append(dict(code='OUTPUT_UNEXPECTED', severity='ERROR',
                                        task_id=key[0], port=key[1], path=str(path)))
    if not checked and not diagnostics:
        diagnostics.append(dict(code='OUTPUT_COVERAGE_EMPTY', severity='ERROR'))
    return write_report(report_path, diagnostics, scope='observed invocation only; not unexecuted branches',
                        dependency_length_source='runtime_dmt_16bit' if runtime_dmt else 'legacy_static',
                        dag_sha256=hashlib.sha256(dag_path.read_bytes()).hexdigest(), outputs=checked)


def runtime_dmt_profile(venus_config):
    """Profiles explicitly qualified against Venus1 task_manager DMT format."""
    return venus_config in ('venus1p0-64x512', 'venus1p0-64x512-legacy-bpll',
                            'venus1p0-64x512-300mhz')
