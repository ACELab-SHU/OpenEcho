import os
import re
import json
import copy

from type_codes import (
    POINTER_TYPES,
    POINTER_INPUT_STRUCT_BYTES,
    TYPE_PTR_TEMP,
    TYPE_DAG_DFE_STATIC,
)


def _lookup_symbol_tile_addr(variable_map_path, symbol_name):
    """从 linker variable.map 解析符号 tile 地址（与 map_variable 同源）。"""
    if not variable_map_path or not os.path.isfile(variable_map_path):
        return None
    with open(variable_map_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    m = re.search(
        rf'^\s*0x([0-9a-fA-F]+)\s+{re.escape(symbol_name)}\s*$',
        content,
        re.MULTILINE,
    )
    if m:
        return int(m.group(1), 16)
    return None


def _combined_data_base_from_map(variable_map_path):
    """combined_data / ram 段起点，用于 map 无符号行时的回退。"""
    if not variable_map_path or not os.path.isfile(variable_map_path):
        return None
    with open(variable_map_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    m = re.search(r'combined_data\s+0x([0-9a-fA-F]+)', content)
    if m:
        return int(m.group(1), 16)
    m = re.search(r'ram\s+0x([0-9a-fA-F]+)', content)
    if m:
        return int(m.group(1), 16)
    return None


def _resolve_parameter_data_tile_addr(variable_map_path, symbol_name, all_data_entry):
    """parameter 指针 pointed_addr：优先 map 符号地址，否则段基址 + all_data.offset。"""
    addr = _lookup_symbol_tile_addr(variable_map_path, symbol_name)
    if addr is not None:
        return addr
    base = _combined_data_base_from_map(variable_map_path)
    if base is not None and all_data_entry is not None and 'offset' in all_data_entry:
        return base + int(all_data_entry['offset'])
    return None


def _apply_pointer_slice_dest(var, variable_map_path, all_data):
    name = var.get('name')
    if not name or name not in all_data:
        return
    tile = _resolve_parameter_data_tile_addr(
        variable_map_path, name, all_data[name],
    )
    if tile is not None:
        var['slice_data_dest_str'] = f"0x{tile:X}"


def _apply_temp_pointer_slice_dest(var, temp_alloc_map):
    if var.get('type') != TYPE_PTR_TEMP or not temp_alloc_map:
        return
    name = var.get('name')
    if name and name in temp_alloc_map:
        var['slice_data_dest_str'] = hex(int(temp_alloc_map[name]['temp_offset']))


def fill(final_json, all_data_json_path, output_json, combined_json_path, input_tasks_json_path, temp_alloc_map=None):
    if temp_alloc_map is None:
        temp_alloc_map = {}
    with open(output_json, 'r') as f:
        output_data = json.load(f)

    all_data = {}
    if os.path.isfile(all_data_json_path) and os.path.getsize(all_data_json_path) > 0:
        with open(all_data_json_path, 'r') as f:
            all_data = json.load(f)

    with open(combined_json_path, 'r') as f:
        combined_data = json.load(f)

    # 读取 input_tasks.json 以获取 para_Input 信息
    input_tasks_data = {}
    if os.path.isfile(input_tasks_json_path) and os.path.getsize(input_tasks_json_path) > 0:
        with open(input_tasks_json_path, 'r') as f:
            input_tasks_list = json.load(f)
        input_tasks_data = {task['taskId']: task for task in input_tasks_list}

    input_key = "all_input"
    variable_map_path = os.path.join(os.path.dirname(all_data_json_path), 'variable.map')

    data_offset_var = 0
    for task_name, task_data in combined_data.items():
        if task_name == "variable" :
            data_offset_var = task_data['data_offset']
            break

    for item in output_data:
        # 步骤1：填充已有的 all_input 变量信息
        if input_key in item and item[input_key] is not None:
            for var in item[input_key]:
                _apply_temp_pointer_slice_dest(var, temp_alloc_map)
                if 'name' in var and var['name'] in all_data:
                    # 保存原有的 type（指针类型 0b100/0b101/0b110，不要被覆盖）
                    original_type = var.get('type', None)
                    info = copy.deepcopy(all_data[var['name']])
                    info['offset'] = all_data[var['name']]['offset'] + data_offset_var
                    var.update(info)
                    # 如果原来是指针类型，恢复 type；length 为指针槽大小，非数据区长度
                    if original_type in POINTER_TYPES:
                        var['type'] = original_type
                        if all_data[var['name']].get('type') != TYPE_DAG_DFE_STATIC:
                            var['length'] = POINTER_INPUT_STRUCT_BYTES
                        if original_type != TYPE_PTR_TEMP:
                            _apply_pointer_slice_dest(var, variable_map_path, all_data)
                        else:
                            _apply_temp_pointer_slice_dest(var, temp_alloc_map)

        # 步骤2：从 input_tasks.json 中补充缺失的 parameter 指针输入
        if 'current_taskId' in item:
            task_id = None
            # 根据 current_taskId 找到对应的 taskId
            for task in input_tasks_list:
                if 'taskId' in task:
                    # 简单匹配：假设 output_data 中的顺序与 input_tasks 一致
                    # 更可靠的方法是通过 debug_task_name 匹配
                    pass

            # 通过 debug_task_name 匹配
            if 'debug_task_name' in item:
                task_id = item['debug_task_name']
                if task_id in input_tasks_data:
                    task_info = input_tasks_data[task_id]
                    # 检查 para_Input 中是否有 is_pointer=true 的输入
                    if 'para_Input' in task_info:
                        for para_input in task_info['para_Input']:
                            if para_input.get('is_pointer', False):
                                # 检查这个 parameter 指针是否已经在 all_input 中
                                para_name = para_input['name']
                                already_exists = False
                                if input_key in item and item[input_key] is not None:
                                    for existing_var in item[input_key]:
                                        if existing_var.get('name') == para_name:
                                            already_exists = True
                                            break

                                # 如果不存在，添加到 all_input
                                if not already_exists:
                                    if input_key not in item or item[input_key] is None:
                                        item[input_key] = []

                                    # 从 all_data 获取基础信息
                                    new_input = {
                                        "name": para_name,
                                        "type": para_input['type'],
                                        "parentTasksPort": "0b0000000000",
                                        "dest_address": "null"
                                    }

                                    if para_name in all_data:
                                        info = copy.deepcopy(all_data[para_name])
                                        info['offset'] = all_data[para_name]['offset'] + data_offset_var
                                        # 保存指针类型，防止被 all_data 中的 type 覆盖
                                        pointer_type = new_input['type']
                                        new_input.update(info)
                                        # 恢复指针类型
                                        new_input['type'] = pointer_type
                                        if pointer_type in POINTER_TYPES:
                                            if all_data[para_name].get('type') != TYPE_DAG_DFE_STATIC:
                                                new_input['length'] = POINTER_INPUT_STRUCT_BYTES
                                            if pointer_type != TYPE_PTR_TEMP:
                                                _apply_pointer_slice_dest(
                                                    new_input, variable_map_path, all_data,
                                                )
                                            else:
                                                _apply_temp_pointer_slice_dest(
                                                    new_input, temp_alloc_map,
                                                )

                                    item[input_key].append(new_input)

        # 透传 all_output 字段（如果存在）
        if 'all_output' in item:
            pass  # all_output 已存在，保持不变

    # 步骤3：重新计算所有 task 的 Input_Num 和 Output_Num
    for item in output_data:
        # 跳过 return_output 节点（它没有 all_input/all_output）
        if 'return_output' in item:
            continue

        # 重新计算 Input_Num
        if 'all_input' in item and item['all_input'] is not None:
            item['Input_Num'] = len(item['all_input'])
        else:
            item['Input_Num'] = 0

        # 重新计算 Output_Num
        if 'all_output' in item and item['all_output'] is not None:
            item['Output_Num'] = len(item['all_output'])
        else:
            # 如果没有 all_output，保持原有的 Output_Num（兼容旧格式）
            pass

    with open(final_json, 'w') as f:
        json.dump(output_data, f, indent=4)

def process_dag_files(map_folder_path, heft_base_path, final_base_path):
    for dag_folder in os.listdir(final_base_path):
        dag_base = dag_folder.split(".json")[0]
        dag_path = os.path.join(final_base_path, dag_base)
        if os.path.isdir(dag_path):

            final_dag_path = os.path.join(final_base_path, dag_base)
            os.makedirs(final_dag_path, exist_ok=True)

            final_json_path = os.path.join(final_dag_path, "final_all_input.json")
            var_json_path = os.path.join(map_folder_path, dag_base, "all_data.json")

            heft_json_path = os.path.join(heft_base_path, f"{dag_base}.json")
            combined_json_path = os.path.join(final_dag_path, "combined_task_without_padding.json")
            input_tasks_json_path = os.path.join(final_dag_path, "input_tasks.json")
            temp_alloc_path = os.path.join('./variable/map/', f"{dag_base}_temp_alloc.json")
            temp_alloc_map = {}
            if os.path.exists(temp_alloc_path):
                with open(temp_alloc_path, 'r') as f:
                    temp_alloc_list = json.load(f)
                temp_alloc_map = {entry['name']: entry for entry in temp_alloc_list}
            fill(final_json_path, var_json_path, heft_json_path, combined_json_path, input_tasks_json_path, temp_alloc_map)

map_folder_path = "./IJ"
heft_base_path = "./heft_new/DAG"
final_base_path = "./IJ"
process_dag_files(map_folder_path, heft_base_path, final_base_path)
