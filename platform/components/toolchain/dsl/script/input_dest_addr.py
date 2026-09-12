import os
import re
import json
import sys

_script_dir = os.path.dirname(os.path.abspath(__file__))
if _script_dir not in sys.path:
    sys.path.insert(0, _script_dir)

from type_codes import (
    TYPE_PTR_TEMP,
    TYPE_PTR_PARAM_GLOBAL,
    TYPE_PTR_DAG_DFE,
    TYPE_PARAM_GLOBAL,
    TYPE_DAG_DFE_STATIC,
    POINTER_TYPES,
    POINTER_INPUT_STRUCT_BYTES,
)

def _base_task_name(name):
    return name.split('*')[0] if isinstance(name, str) and '*' in name else name

def _build_task_length_maps(dag_path):
    spec_path = os.path.join(dag_path, "input_tasks_spec.json")
    if not os.path.exists(spec_path):
        return {}, {}, {}

    with open(spec_path, 'r') as f:
        spec_tasks = json.load(f)

    output_len_by_task_var = {}
    parent_lookup = {}
    output_len_by_var = {}

    for task in spec_tasks:
        task_id = _base_task_name(task.get("taskId", ""))
        for out in task.get("all_output", []):
            out_name = out.get("name")
            out_len = out.get("length")
            if out_name is None or out_len is None:
                continue
            out_len = int(out_len)
            output_len_by_task_var[(task_id, out_name)] = out_len
            output_len_by_var.setdefault(out_name, out_len)

    for task in spec_tasks:
        consumer = _base_task_name(task.get("taskId", ""))
        for p in task.get("parentTasks", []):
            in_var = p.get("outputVar")
            producer = _base_task_name(p.get("taskId", ""))
            out_var = p.get("outputVar")
            if in_var and producer and out_var:
                parent_lookup[(consumer, in_var)] = (producer, out_var)

    return output_len_by_task_var, parent_lookup, output_len_by_var

def _resolve_variable_length(variable_name, variable_info, function_name, all_data,
                             output_len_by_task_var, parent_lookup,
                             output_len_by_var):
    # 指针入参：标量输入区只排 pointer_struct 槽，不按 all_data 数据长度占坑
    if variable_info.get('is_pointer') or variable_info.get('type') in POINTER_TYPES:
        return POINTER_INPUT_STRUCT_BYTES

    if variable_name in all_data and 'length' in all_data[variable_name]:
        return int(all_data[variable_name]['length'])

    consumer = _base_task_name(function_name)
    rel = parent_lookup.get((consumer, variable_name))
    if rel:
        producer, out_var = rel
        length = output_len_by_task_var.get((producer, out_var))
        if length is None:
            raise ValueError(
                f"Cannot resolve output length for temp input '{variable_name}' "
                f"in task '{consumer}' from producer '{producer}'."
            )
        return int(length)

    if variable_name in output_len_by_var:
        return int(output_len_by_var[variable_name])

    return 64

def update_json_with_dest_address(dag_path, task_json_path, venus_test_path, task_out_json_path, lane_num, VenusInputStructAddr):
    with open(task_json_path, 'r') as file:
        data = json.load(file)

    function_names = list(data.keys())
    output_len_by_task_var, parent_lookup, output_len_by_var = _build_task_length_maps(dag_path)
    VENUS_VRFADDR = 0x00100000
    print(f"VenusInputStructAddr:{VenusInputStructAddr}")

    # Load C signature types (produced by venus_test/type_verify.py)
    input_type_path = os.path.join(venus_test_path, "input_type.json")
    input_type_data = {}
    if os.path.exists(input_type_path):
        with open(input_type_path, 'r', encoding='utf-8', errors='ignore') as f:
            try:
                input_type_data = json.load(f)
            except Exception:
                input_type_data = {}

    def _arg_type_for(function_name, index):
        actual = _base_task_name(function_name)
        ti = input_type_data.get(actual) or {}
        args = ti.get("args") or []
        if 0 <= int(index) < len(args):
            return (args[int(index)] or {}).get("type")
        return None

    def _is_scalar_semantic(function_name, variable_name, variable_info):
        """
        Decide scalar/vector semantics using C signature type first.
        - Vector types: "__vNNNiM" must be treated as vector even if num==1.
        - Scalar types: short/int/char and *_struct are scalar semantics.
        Fallback to historical num==1 if type info is absent.
        """
        t = _arg_type_for(function_name, variable_info.get("index", 0))
        if isinstance(t, str) and t.startswith("__v"):
            return False
        if t in ("short", "int", "char") or (isinstance(t, str) and t.endswith("_struct")):
            return True
        # Fallback: keep legacy behavior
        return variable_info.get('num') == 1 or variable_info.get('num') == '1'

    def _build_reg_alias_map(asm_lines):
        """
        Build a simple register alias map from prologue moves:
        - mv dst, src
        - addi dst, src, 0
        Returns dict dst->src, and a resolver to root reg.
        """
        alias = {}
        mv_pat = re.compile(r'^\s*(mv|addi)\s+([a-z0-9]+)\s*,\s*([a-z0-9]+)(?:\s*,\s*0)?\s*$')
        for ln in asm_lines:
            m = mv_pat.match(ln.strip())
            if not m:
                continue
            op, dst, src = m.group(1), m.group(2), m.group(3)
            if op == "mv" or op == "addi":
                alias[dst] = src
        def resolve(r):
            seen = set()
            cur = r
            while cur in alias and cur not in seen:
                seen.add(cur)
                cur = alias[cur]
            return cur
        return resolve

    def _parse_vns_bind_groups(asm_file_path):
        """
        Parse analysis.s and extract vns_bind groups.
        A group is started by 'vns_delimit N' and contains consecutive vns_bind lines until 'vns_delimit 0' or next delimit.
        We only keep groups that have at least one vns_bind and a stable base_reg.
        Returns list of dicts: {base_reg, root_reg, arg_index(optional), min_vns_id}.
        """
        with open(asm_file_path, 'r') as f:
            lines = f.readlines()
        # build alias resolver from lines before first vns_delimit
        prologue = []
        for ln in lines:
            if 'vns_delimit' in ln:
                break
            prologue.append(ln)
        resolve = _build_reg_alias_map(prologue)

        groups = []
        current = None
        for ln in lines:
            s = ln.strip()
            dm = re.match(r'^vns_delimit\s+(\d+)$', s)
            if dm:
                # close group on delimit boundaries
                if current and current.get("binds"):
                    groups.append(current)
                n = int(dm.group(1))
                current = {"delimit": n, "binds": []}
                continue
            bm = re.match(r'^vns_bind\s+vns(\d+)\s*,\s*([a-z0-9]+)\s*,\s*(\d+)$', s)
            if bm and current is not None:
                vns_id = int(bm.group(1))
                base_reg = bm.group(2)
                current["binds"].append((vns_id, base_reg))
                continue

        if current and current.get("binds"):
            groups.append(current)

        result = []
        for g in groups:
            # require stable base_reg
            base_regs = {b for (_v, b) in g["binds"]}
            if len(base_regs) != 1:
                continue
            base_reg = list(base_regs)[0]
            root = resolve(base_reg)
            m = re.fullmatch(r'a([0-7])', root)
            arg_index = int(m.group(1)) if m else None
            min_vns = min(v for (v, _b) in g["binds"])
            result.append({
                "base_reg": base_reg,
                "root_reg": root,
                "arg_index": arg_index,
                "min_vns_id": min_vns,
                "delimit": g.get("delimit", 0),
            })
        return result
    for function_name in function_names:
        actual_function_name = function_name.split('*')[0] if '*' in function_name else function_name
        asm_file_path = os.path.join(venus_test_path, 'ir', 'analysis', actual_function_name + '_analysis.s')

        if not os.path.exists(asm_file_path):
            print(f"Assembly file not found for function {function_name}. Skipping...")
            continue

        # Initialize variables for parsing assembly file
        in_function = False
        in_vns_delimit = False
        index = 0
        scalar_i = 0
        # 标量输入在 Venus 输入结构体带中的排布：上一项结束后第一个空闲字节；下一项起始 = align64(cursor)
        input_struct_cursor = None
        vns_delimit_count = 0
        final_vector_index = 0
        num_params = len(data[function_name]['input'])
        last_bound_variable = None
        all_index = 0

        for variable_info in data[function_name]['input'].values():
            variable_info['updated'] = False

        # C signature is the source of truth for pointer ABI.  Some BAS
        # parameters are ordinary arrays syntactically, but the task consumes
        # them through pointer_struct, so only the pointer descriptor belongs in
        # the scalar input window.
        for variable_name, variable_info in data[function_name]['input'].items():
            arg_type = _arg_type_for(function_name, variable_info.get("index", 0))
            if arg_type == "pointer_struct":
                variable_info['is_pointer'] = True

        # 处理指针类型：查找被指向变量的地址
        with open(os.path.join(dag_path, "all_data.json"), 'r') as all_data_file:
            all_data = json.load(all_data_file)

        # 加载临时变量分配表（若存在），key = 变量名
        dag_name = os.path.basename(dag_path)
        temp_alloc_path = os.path.join(dag_path, "..", "..", "variable", "map", f"{dag_name}_temp_alloc.json")
        temp_alloc_map = {}
        temp_alloc_by_producer = {}  # (producer_task_id, varname) -> temp_offset
        temp_var_producer_map = {}   # varname -> producer_task_id
        temp_producer_port_map = {}  # varname -> producer_output_port
        temp_lifetime_map = {}       # varname -> {birth_order, last_use_order}
        if os.path.exists(temp_alloc_path):
            with open(temp_alloc_path, 'r') as f:
                temp_alloc_list = json.load(f)
            temp_alloc_map = {entry['name']: entry for entry in temp_alloc_list}

        # 加载 temp_variables.json 构建 (producer_task_id, varname) 复合键映射
        temp_vars_path = os.path.join(dag_path, "temp_variables.json")
        if os.path.exists(temp_vars_path):
            with open(temp_vars_path, 'r') as f:
                temp_vars_list = json.load(f)
            for tv in temp_vars_list:
                vname = tv['name']
                prod = tv['producer_task_id']
                temp_var_producer_map[vname] = prod
                temp_producer_port_map[vname] = tv.get('producer_output_port', 0)
                # 保存生命周期信息
                temp_lifetime_map[vname] = {
                    'birth_order': tv.get('birth_order', 0),
                    'last_use_order': tv.get('last_use_order', float('inf'))
                }
                if vname in temp_alloc_map:
                    # 优先使用 temp_offset，如果不存在则使用 dest_address（兼容旧格式）
                    if 'temp_offset' in temp_alloc_map[vname]:
                        temp_alloc_by_producer[(prod, vname)] = temp_alloc_map[vname]['temp_offset']
                    else:
                        temp_alloc_by_producer[(prod, vname)] = temp_alloc_map[vname].get('dest_address', 0)

        # 获取当前 task 的执行顺序（用于生命周期检查）
        # 假设 task 名称中包含顺序信息，或者从其他地方获取
        # 这里需要根据实际情况调整获取方式
        current_task_order = None
        # 尝试从 function_name 中提取顺序信息，或者从其他 JSON 文件中读取
        # 暂时使用一个简单的方法：从 temp_variables.json 中查找当前 function 作为 consumer 的记录
        task_order_map = {}  # task_id -> order
        if os.path.exists(temp_vars_path):
            for tv in temp_vars_list:
                task_order_map[tv['producer_task_id']] = tv.get('producer_task_order', 0)

        for variable_name, variable_info in data[function_name]['input'].items():
            if variable_info.get('is_pointer', False):              # 识别指针
                # 指针类型：查找被指向变量的地址
                pointed_var_name = variable_name.split('*')[0]      # 去掉可能的*后缀
                # 优先用 (producer_task_id, varname) 复合键查临时变量分配表
                producer_id = temp_var_producer_map.get(pointed_var_name)
                addr = None
                if producer_id is not None:
                    addr = temp_alloc_by_producer.get((producer_id, pointed_var_name))
                if addr is None and pointed_var_name in temp_alloc_map:
                    # 优先使用 temp_offset，如果不存在则使用 dest_address（兼容旧格式）
                    if 'temp_offset' in temp_alloc_map[pointed_var_name]:
                        offset = temp_alloc_map[pointed_var_name]['temp_offset']
                        # 将 temp_offset 转换为十六进制字符串格式
                        addr = hex(offset) if isinstance(offset, int) else offset
                    else:
                        addr = temp_alloc_map[pointed_var_name].get('dest_address', None)
                if addr is not None:
                    # 指向 temp：0b100（新编码）
                    # 生命周期检查：检查被指向的 temp 是否已经释放
                    if pointed_var_name in temp_lifetime_map:
                        lifetime = temp_lifetime_map[pointed_var_name]
                        current_task_id = actual_function_name
                        current_task_order = task_order_map.get(current_task_id, None)

                        if current_task_order is None:
                            if 'task_order' in data[function_name]:
                                current_task_order = data[function_name]['task_order']

                        if current_task_order is not None:
                            last_use = lifetime['last_use_order']
                            if current_task_order > last_use:
                                raise ValueError(
                                    f"Error: Task '{current_task_id}' (order {current_task_order}) "
                                    f"is trying to take address of temp variable '{pointed_var_name}' "
                                    f"which has already been released (last_use_order: {last_use}). "
                                    f"This is a dangling pointer error."
                                )

                    # 设置指针类型和 parentTasksPort，但不设置 dest_address
                    # dest_address 应该由后续的 vns_bind 或标量分配流程来设置
                    variable_info['type'] = TYPE_PTR_TEMP
                    variable_info['is_pointer'] = True
                    # 设置 parentTasksPort 为被指向 temp 的生产者端口
                    if producer_id is not None:
                        port_value = temp_producer_port_map.get(pointed_var_name, 0)
                        variable_info['parentTasksPort'] = f"0b{port_value:010b}"
                    # 不设置 updated=True，让后续流程为指针本身分配 dest_address
                elif pointed_var_name in all_data:
                    vt = all_data[pointed_var_name].get('type')
                    if vt == TYPE_PARAM_GLOBAL:
                        variable_info['type'] = TYPE_PTR_PARAM_GLOBAL
                    elif vt == TYPE_DAG_DFE_STATIC:
                        variable_info['type'] = TYPE_PTR_DAG_DFE
                    else:
                        variable_info['type'] = TYPE_PTR_PARAM_GLOBAL
                    variable_info['is_pointer'] = True
                    # 对于 parameter/global/dfedata/dag_input，parentTasksPort 按现有非 temp 口径处理
                    if 'parentTasksPort' not in variable_info or not variable_info['parentTasksPort']:
                        variable_info['parentTasksPort'] = '0b0000000000'
                    # 不设置 updated=True，让后续流程为指针本身分配 dest_address
                else:
                    # 未找到被指向的变量，给出警告但不阻止编译
                    # 可能是 return_value 或其他尚未处理的变量类型
                    print(f"Warning: Task '{actual_function_name}' is trying to take address of "
                          f"variable '{pointed_var_name}' which is not found in temp_alloc or all_data. "
                          f"Skipping pointer processing for this variable.")
                    # 不设置 updated=True，让后续流程处理

        # Open and parse assembly file
        # New root-correct vns_bind mapping:
        # Treat one vns_delimit group as ONE argument binding group (not N arguments).
        bind_groups = _parse_vns_bind_groups(asm_file_path)
        if bind_groups:
            # Prepare ordered input variables and vector-variable list (C signature is the ground truth).
            ordered_inputs = sorted(
                [(vn, vi) for vn, vi in data[function_name]['input'].items()],
                key=lambda x: int(x[1].get("index", 0))
            )
            vector_vars = []
            for vn, vi in ordered_inputs:
                idx = int(vi.get("index", 0))
                t = _arg_type_for(function_name, idx)
                if isinstance(t, str) and t.startswith("__v"):
                    vector_vars.append((vn, vi, t))

            used_group = [False] * len(bind_groups)

            # Pass 1: bind groups that can be mapped to an explicit arg_index (a0~a7) uniquely.
            for gi, g in enumerate(bind_groups):
                arg_index = g.get("arg_index")
                if arg_index is None:
                    continue
                arg_type = _arg_type_for(function_name, arg_index)
                if not (isinstance(arg_type, str) and arg_type.startswith("__v")):
                    continue
                for vn, vi, _t in vector_vars:
                    if vi.get("updated"):
                        continue
                    if int(vi.get("index", -1)) != int(arg_index):
                        continue
                    vns_id = int(g["min_vns_id"])
                    dest_address = hex(vns_id * lane_num * 8 + VENUS_VRFADDR)
                    vi["dest_address"] = dest_address
                    vi["updated"] = True
                    used_group[gi] = True
                    all_index = all_index + 1
                    print(f"DEBUG vns_group_bind: arg{arg_index} ({vn}) type={arg_type} -> vns{vns_id} dest_address={dest_address}")
                    break

            # Pass 2: sequentially bind remaining unbound vector vars to remaining groups.
            remaining_groups = [g for (u, g) in zip(used_group, bind_groups) if not u]
            rg_iter = iter(remaining_groups)
            for vn, vi, t in vector_vars:
                if vi.get("updated"):
                    continue
                try:
                    g = next(rg_iter)
                except StopIteration:
                    break
                vns_id = int(g["min_vns_id"])
                dest_address = hex(vns_id * lane_num * 8 + VENUS_VRFADDR)
                vi["dest_address"] = dest_address
                vi["updated"] = True
                all_index = all_index + 1
                print(f"DEBUG vns_group_bind_seq: ({vn}) index={vi.get('index')} type={t} -> vns{vns_id} dest_address={dest_address}")

        for variable_name, variable_info in data[function_name]['input'].items():
            if variable_info['updated'] == False:
                print(f"DEBUG input_dest_addr: Processing {variable_name}, type={variable_info.get('type')}, num={variable_info.get('num')}")
                # 指针类型（0b100/0b101/0b110）不从 temp_alloc_map 取 dest_address
                # b000 类型的 temp 输入也不从 temp_alloc_map 取 dest_address
                if variable_info.get('type') in list(POINTER_TYPES) + ['0b000']:
                    # 跳过，走标量分配流程
                    print(f"DEBUG: {variable_name} has type {variable_info.get('type')}, skipping temp_alloc_map check")
                    pass
                else:
                    # 非 temp、非指针的变量
                    # 检查是否是 temp 变量（在 temp_alloc_map 中）
                    base_var = variable_name.split('*')[0] if '*' in variable_name else variable_name
                    if base_var in temp_alloc_map:
                        # temp 变量作为输入，不从 temp_alloc_map 取 dest_address
                        # 继续走标量分配流程
                        print(f"DEBUG: {variable_name} is temp var, skipping temp_alloc_map")
                        pass
                    else:
                        # 非 temp 变量，可能需要特殊处理（但通常不会走到这里）
                        print(f"DEBUG: {variable_name} is not temp var")
                        pass

                print(f"DEBUG: {variable_name} entering scalar allocation")
                with open(os.path.join(dag_path, "all_data.json"), 'r') as all_data_file:
                    all_data = json.load(all_data_file)
                print(f"variable_name:{variable_name}")
                print(f"variable_info:{variable_info}")
                # 先获取变量长度
                variable_length = _resolve_variable_length(
                    variable_name, variable_info, function_name, all_data,
                    output_len_by_task_var, parent_lookup, output_len_by_var
                )
                # 下一项起始 = 向上对齐到 64 字节的「上一项结束之后」；首项固定在 VenusInputStructAddr（配置为 64 对齐）
                if input_struct_cursor is None:
                    start_int = VenusInputStructAddr
                else:
                    start_int = ((input_struct_cursor + 63) // 64) * 64
                dest_address = hex(start_int)
                variable_info['dest_address'] = dest_address
                variable_info['updated'] = True
                input_struct_cursor = start_int + variable_length
                all_index = all_index + 1

        with open("../venus_test/task_input_num.json", "r") as file:
            dsl_data = json.load(file)

        expected_input_num = dsl_data[actual_function_name]
        final_vector_index = all_index
        if(expected_input_num != final_vector_index):
            raise ValueError(f"Input number for task '{actual_function_name}' is {final_vector_index}, expected {expected_input_num}")
        all_index = 0

        for variable_name, variable_info in data[function_name]['input'].items():
            if variable_info['updated'] == False:
                print(f"Attention: Variable {variable_name} not updated, assigning dest_address")
                # 对于 b000 与三种指针码，不从 temp_alloc_map 取 dest_address
                # dest_address 是 tile 目标地址，应该走标量分配流程
                with open(os.path.join(dag_path, "all_data.json"), 'r') as all_data_file:
                    all_data = json.load(all_data_file)
                variable_length = _resolve_variable_length(
                    variable_name, variable_info, function_name, all_data,
                    output_len_by_task_var, parent_lookup, output_len_by_var
                )
                if input_struct_cursor is None:
                    start_int = VenusInputStructAddr
                else:
                    start_int = ((input_struct_cursor + 63) // 64) * 64
                dest_address = hex(start_int)
                variable_info['dest_address'] = dest_address
                input_struct_cursor = start_int + variable_length
            if 'updated' in variable_info:
                del variable_info['updated']

    with open(task_out_json_path, 'w') as outfile:
        json.dump(data, outfile, indent=4)

    print(f"JSON file {task_json_path} has been updated.")

def process_dag_folders(task_base_path, venus_test_base_path, lane_num, VenusInputStructAddr):
    for dag_folder in os.listdir(task_base_path):
        dag_base = dag_folder.split(".json")[0]
        dag_path = os.path.join(task_base_path, dag_base)
        if os.path.isdir(dag_path):
            task_json_path = os.path.join(dag_path, "task_input_output.json")
            venus_test_path = os.path.join(venus_test_base_path)
            task_out_json_path = os.path.join(dag_path, "task_input_dest_addr.json")
            update_json_with_dest_address(dag_path, task_json_path, venus_test_path, task_out_json_path, lane_num, VenusInputStructAddr)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python input_dest_addr.py <lane_num> <VenusInputStructAddr>")
        sys.exit(1)
    lane_num = int(sys.argv[1])
    VenusInputStructAddr = int(int(sys.argv[2], 16) - 0x80000000)

    task_base_path = '../IJ'
    venus_test_base_path = '../venus_test'

    process_dag_folders(task_base_path, venus_test_base_path, lane_num, VenusInputStructAddr)
