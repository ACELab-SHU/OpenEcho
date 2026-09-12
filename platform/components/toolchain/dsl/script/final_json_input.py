"""Bind plain dependency DMA extents to the normalized consumer vector ABI.

Producer lengths are allocation bounds, not consumer VRF capacities. Pointers,
concatenations and slices retain their existing explicit transport contracts.
"""
import json
from pathlib import Path
import re
import warnings


def bind_consumer_lengths(dag, bindings, signatures):
    tasks = {t['current_taskId']: t for t in dag if 'current_taskId' in t}
    for task in tasks.values():
        name = task['debug_task_name']
        if name not in bindings or name.split('*')[0] not in signatures:
            warnings.warn('missing consumer ABI metadata: ' + name)
            continue
        inputs = bindings[name]['input']
        args = signatures[name.split('*')[0]]['args']
        for edge in task.get('all_input', []):
            if int(edge.get('type', '0'), 0) != 0:
                continue
            if int(edge.get('concat_value', 0)) or int(edge.get('slice_length', 0)):
                continue
            address = int(edge['dest_address'], 0)
            candidates = [v for v in inputs.values()
                          if int(v['dest_address'], 0) == address]
            if len(candidates) != 1:
                warnings.warn('ambiguous consumer ABI binding: %s/%s' % (name, edge['name']))
                continue
            index = int(candidates[0]['index'])
            arg_type = args[index]['type']
            match = re.fullmatch(r'__v([1-9][0-9]*)i(8|16|32|64)', arg_type)
            if not match:
                if arg_type.startswith('__v'):
                    warnings.warn('unsupported consumer vector ABI: ' + arg_type)
                continue
            capacity = int(match[1]) * int(match[2]) // 8
            encoded = int(edge['parentTasksPort'], 0)
            producer = tasks[encoded >> 4]['all_output'][encoded & 15]
            declared = int(producer['length'])
            if declared <= 0:
                raise ValueError('nonpositive producer capacity')
            # Neither allocation is resized. The observed return must fit the
            # selected transfer extent; the platform verifies that separately.
            edge['consumer_capacity_bytes'] = capacity
            edge['consumer_arg_index'] = index
            edge['consumer_arg_type'] = arg_type
            # Retain the old hint for other hardware profiles. Venus1 ignores
            # it for transport: the runtime DMT vreturn decides the DMA.
            edge['length'] = min(declared, capacity)
    return dag


def main():
    signatures = json.loads(Path('venus_test/input_type.json').read_text())
    for path in sorted(Path('final_output').glob('*.json')):
        bindings = json.loads((Path('IJ') / path.stem / 'task_input_dest_addr.json').read_text())
        dag = bind_consumer_lengths(json.loads(path.read_text()), bindings, signatures)
        path.write_text(json.dumps(dag, indent=4) + '\n')


if __name__ == '__main__':
    main()
