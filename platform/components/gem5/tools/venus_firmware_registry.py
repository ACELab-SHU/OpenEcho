"""ELF-derived identities for firmware fires; never a predicted fire sequence.

Identity covers descriptor structure, return routing and executable code. Task
data and dynamic/global values are deliberately read from live DMA backing.
"""
import hashlib
import json
from pathlib import Path

CFG = 0x1ff0000
TABLES = {
    'task_container': (0x0000, 0x6000),
    'global_para': (0x6000, 0x2100),
    'output_num': (0x8100, 64),
    'task_num': (0x8200, 64),
    'return_value': (0x8400, 64),
    'output_addr': (0x8700, 0x6800),
}


def validate_input_execution_mode(manifest, execute_firmware):
    """A code-only registry may never autostart as a populated contract."""
    if manifest.get('input_materialization') == 'live-firmware-dma':
        if not execute_firmware:
            raise ValueError('live-firmware-dma inputs require the firmware scheduler engine')
        if manifest.get('initial_inputs') or manifest.get('l2_backing_inputs'):
            raise ValueError('live firmware manifest must not preload input fixtures')


def identity_segments(elf, runtime):
    prefix = runtime['prefix']
    tables, segments = {}, []
    for suffix, (offset, maximum) in TABLES.items():
        symbol = f'{prefix}_{suffix}'
        if symbol not in elf.symbols:
            raise ValueError(f'firmware registry requires {symbol}')
        payload = elf.symbol_data(symbol)
        if (len(payload) > maximum or len(payload) % 64 or
                (not payload and suffix != 'global_para') or
                (suffix in ('task_num', 'output_num', 'return_value') and len(payload) != 64)):
            raise ValueError(f'invalid firmware identity size: {symbol}')
        tables[suffix] = payload
        if payload:
            segments.append((CFG + offset, payload))
    count = len(runtime['tasks'])
    if not 1 <= count <= 64 or len(tables['task_container']) != count * 384:
        raise ValueError('firmware registry task container/count mismatch')
    for slot in runtime['dmt_layout']['slots']:
        index = slot['task'] * 16 + slot['port']
        offset = (index // 10) * 64 + (index % 10) * 6
        raw = tables['output_addr'][offset:offset + 6]
        value = int.from_bytes(raw, 'little')
        if (len(raw) != 6 or (value & 0xffff) != slot['capacity'] or
                (value >> 16) != slot['relative_offset']):
            raise ValueError('payload JSON DMT layout disagrees with ELF output_addr')
    # The packed return list has ten-bit keys, then a zero terminator bit,
    # then one padding bits (Scheduler read_dag_json.py). The JSON supplies
    # the count; verify the complete table against the immutable ELF.
    returns = []
    value = int.from_bytes(tables['return_value'], 'little')
    slots = {(s['task'], s['port']): s for s in runtime['dmt_layout']['slots']}
    document = json.loads(Path(runtime['dmt_layout']['json']).read_text())
    nodes = [n['return_output'] for n in document if 'return_output' in n]
    if len(nodes) != 1 or not 1 <= len(nodes[0]) <= 16:
        raise ValueError('firmware registry requires one bounded return list')
    expected = 0
    for index, entry in enumerate(nodes[0]):
        key = (value >> (index * 10)) & 0x3ff
        pair = (key >> 4, key & 15)
        declared = int(str(entry['parentTasksPort']), 0)
        if (pair not in slots or key != declared or
                pair[0] != int(entry['parentTasks']) or
                pair[1] != int(entry.get('index', pair[1]))):
            raise ValueError('ELF return table disagrees with payload JSON')
        expected |= key << (index * 10)
        returns.append({'task': pair[0], 'port': pair[1], 'source': 0,
                        'length': slots[pair]['capacity']})
    used = 10 * len(returns)
    expected |= ((1 << (512 - used)) - 2) << used
    if len(tables['return_value']) != 64 or value != expected:
        raise ValueError('ELF return table padding/count mismatch')
    for task in runtime['tasks']:
        address, length = task['code_address'], task['code_length']
        if length <= 0 or address + length > len(runtime['blob']):
            raise ValueError('task code outside ELF DAG binary')
        segments.append((address, runtime['blob'][address:address + length]))
    return segments, returns


def attach_identity(manifest_path, elf, runtime):
    manifest_path = Path(manifest_path)
    segments, returns = identity_segments(elf, runtime)
    manifest = json.loads(manifest_path.read_text())
    records = []
    for index, (address, data) in enumerate(segments):
        path = manifest_path.parent / f'firmware_identity_{index:03d}.bin'
        path.write_bytes(data)
        records.append({'address': address, 'file': str(path.resolve()),
                        'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)})
    manifest['firmware_identity'] = records
    manifest['outputs'] = returns
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')


def materialize_registry(elf_path, prefix, output_dir, type_bits, spmd):
    from venus_l1_dag import Elf32, decode_runtime_dag, discover_prefix, materialize
    elf = Elf32(elf_path)
    suffix = '_task_container'
    prefixes = sorted(name[:-len(suffix)] for name in elf.symbols if name.endswith(suffix))
    if not prefixes:
        raise ValueError('ELF has no linked runtime DAGs')
    primary = discover_prefix(elf, prefix or prefixes[0])
    prefixes.remove(primary)
    manifests = []
    for index, name in enumerate([primary] + prefixes):
        runtime = decode_runtime_dag(elf_path, name, type_bits, spmd)
        destination = output_dir if index == 0 else output_dir / f'dag_{index:03d}'
        path = materialize(runtime, destination, live_firmware_inputs=True)
        attach_identity(path, elf, runtime)
        manifests.append(path)
    manifest = json.loads(manifests[0].read_text())
    manifest['firmware_dag_registry'] = [
        {'manifest': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
        for path in manifests[1:]]
    manifests[0].write_text(json.dumps(manifest, indent=2) + '\n')
    return manifests[0]
