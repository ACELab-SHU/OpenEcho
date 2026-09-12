import json
import os

def process_dag_folder(dag_folder_path):
    input_tasks_spec_path = os.path.join(dag_folder_path, 'input_tasks_spec.json')
    task_input_dest_addr_path = os.path.join(dag_folder_path, 'task_input_dest_addr.json')

    with open(input_tasks_spec_path, 'r') as f:
        task_data = json.load(f)

    with open(task_input_dest_addr_path, 'r') as f:
        dest_data = json.load(f)

    # 加载临时变量分配表（若存在），key = 变量名
    dag_name = os.path.basename(dag_folder_path)
    temp_alloc_path = os.path.join(dag_folder_path, '..', '..', 'variable', 'map', f'{dag_name}_temp_alloc.json')
    temp_alloc_map = {}
    if os.path.exists(temp_alloc_path):
        with open(temp_alloc_path, 'r') as f:
            temp_alloc_list = json.load(f)
        temp_alloc_map = {entry['name']: entry for entry in temp_alloc_list}

    for task in task_data:
        # Match parentTasks
        previous_dest_address = None
        previous_length = 0
        previous_concat_value = 0

        for parent_task in task.get('parentTasks', []):
            concat_dest_address = parent_task['dest_address']
            concat_value = parent_task['concat_value']
            concat_length = parent_task['concat_length']

            if concat_dest_address == "null":
                continue
            dest_address = "null"  # default dest_address
            dest_task_id = task['taskId']

            for dest_param, dest_param_value in dest_data[dest_task_id]['input'].items():
                if dest_param == concat_dest_address:
                   dest_address = dest_param_value.get('dest_address', "null")  # Assign the value directly
                   # 保留 type、is_pointer、parentTasksPort 字段（如果存在）
                   is_pointer = dest_param_value.get('is_pointer', False)
                   parent_task['is_pointer'] = is_pointer
                   if 'type' in dest_param_value:
                       parent_task['type'] = dest_param_value['type']
                   if 'parentTasksPort' in dest_param_value:
                       parent_task['parentTasksPort'] = dest_param_value['parentTasksPort']
                   break
            # Assign dest_address value to parent_task['dest_address']
            parent_task['dest_address'] = dest_address

            current_dest_address = dest_address

            if concat_value == previous_concat_value and previous_concat_value != 0:
                if previous_dest_address:
                    old_address = int(previous_dest_address, 16)
                    new_address = old_address + previous_length
                    parent_task['dest_address'] = hex(new_address)

                previous_dest_address = parent_task['dest_address']
                previous_length = concat_length
            else:
                # # Check if the new dest_address is less than the previous dest_address
                # if previous_dest_address is not None and current_dest_address != "null":
                #     previous_dest_address_int = int(previous_dest_address, 16)
                #     current_dest_address_int = int(current_dest_address, 16)

                #     if current_dest_address_int < previous_dest_address_int:
                #         raise ValueError(f"Error: Current dest_address ({current_dest_address}) is less than previous dest_address ({previous_dest_address}).")

                previous_dest_address = current_dest_address
                previous_length = concat_length

            previous_concat_value = concat_value

            # 确保 concat 相关的输入也有 slice 字段
            if 'slice_data_dest_str' not in parent_task:
                parent_task['slice_data_dest_str'] = parent_task['dest_address']
            if 'slice_data_type' not in parent_task:
                parent_task['slice_data_type'] = "0"
            if 'slice_length' not in parent_task:
                parent_task['slice_length'] = "0"


        for parent_task in task.get('parentTasks', []):
            output_var = parent_task['outputVar']
            lookup_var = parent_task.get('consumerInputVar', output_var)
            not_concat_var_dest_addr = parent_task['dest_address']
            dest_task_id = task['taskId']

            # 先从 dest_data 查找该变量，判断是否为指针类型
            is_pointer_type = False
            for dest_param, dest_param_value in dest_data[dest_task_id]['input'].items():
                if dest_param == lookup_var:
                    var_type = dest_param_value.get('type', '')
                    is_pointer_flag = dest_param_value.get('is_pointer', False)
                    if var_type in ('0b100', '0b101', '0b110') or is_pointer_flag:
                        is_pointer_type = True
                        parent_task['dest_address'] = dest_param_value.get('dest_address', "null")
                        parent_task['is_pointer'] = is_pointer_flag
                        parent_task['type'] = var_type
                        if 'parentTasksPort' in dest_param_value:
                            parent_task['parentTasksPort'] = dest_param_value['parentTasksPort']
                    break

            if is_pointer_type:
                continue

            # temp 变量不应该从 temp_alloc_map 取 dest_address
            # dest_address 是 tile 目标地址，已经由 input_dest_addr.py 正确设置
            # temp_offset 是临时存储区偏移（源地址），不是目标地址
            if not_concat_var_dest_addr != "null":
                continue
            dest_address = "null"
            for dest_param, dest_param_value in dest_data[dest_task_id]['input'].items():
                # if dest_param == output_var:
                if dest_param == lookup_var:
                    dest_address = dest_param_value.get('dest_address', "null")
                    is_pointer = dest_param_value.get('is_pointer', False)
                    parent_task['is_pointer'] = is_pointer
                    # 保留 type 字段（如果存在）
                    if 'type' in dest_param_value:
                        parent_task['type'] = dest_param_value['type']
                    # 保留 parentTasksPort 字段（如果存在，对指针类型很重要）
                    if 'parentTasksPort' in dest_param_value:
                        parent_task['parentTasksPort'] = dest_param_value['parentTasksPort']
                    break
            parent_task['dest_address'] = dest_address

            # 确保所有输入都有 slice 相关字段（包括指针类型），避免后续统计遗漏
            if 'slice_data_dest_str' not in parent_task:
                parent_task['slice_data_dest_str'] = dest_address
            if 'slice_data_type' not in parent_task:
                parent_task['slice_data_type'] = "0"
            if 'slice_length' not in parent_task:
                parent_task['slice_length'] = "0"

        for parent_task in task.get('global_Input', []):
            var = parent_task['name']
            dest_address = "null"  # default dest_address
            dest_task_id = task['taskId']
            for dest_param, dest_param_value in dest_data[dest_task_id]['input'].items():
                if dest_param == var:
                    dest_address = dest_param_value.get('dest_address', "null")  # Assign the value directly
                    # 保留 type 和 is_pointer 字段（如果存在）
                    is_pointer = dest_param_value.get('is_pointer', False)
                    parent_task['is_pointer'] = is_pointer
                    if 'type' in dest_param_value:
                        parent_task['type'] = dest_param_value['type']
                    if 'parentTasksPort' in dest_param_value:
                        parent_task['parentTasksPort'] = dest_param_value['parentTasksPort']
                    break
            # Assign dest_address value to parent_task['dest_address']
            parent_task['dest_address'] = dest_address

            # 确保所有输入都有 slice 相关字段，避免后续统计遗漏
            if 'slice_data_dest_str' not in parent_task:
                parent_task['slice_data_dest_str'] = dest_address
            if 'slice_data_type' not in parent_task:
                parent_task['slice_data_type'] = "0"
            if 'slice_length' not in parent_task:
                parent_task['slice_length'] = "0"

        for parent_task in task.get('para_Input', []):
            var = parent_task['name']
            dest_address = "null"  # default dest_address
            dest_task_id = task['taskId']
            for dest_param, dest_param_value in dest_data[dest_task_id]['input'].items():
                if dest_param == var:
                    dest_address = dest_param_value.get('dest_address', "null")
                    is_pointer = dest_param_value.get('is_pointer', False)
                    parent_task['is_pointer'] = is_pointer
                    # 保留 type 字段（如果存在）
                    if 'type' in dest_param_value:
                        parent_task['type'] = dest_param_value['type']
                    # 保留 parentTasksPort 字段（如果存在，对指针类型很重要）
                    if 'parentTasksPort' in dest_param_value:
                        parent_task['parentTasksPort'] = dest_param_value['parentTasksPort']
                    break
            # temp 变量不应该从 temp_alloc_map 取 dest_address
            # dest_address 是 tile 目标地址，已经由 input_dest_addr.py 正确设置
            parent_task['dest_address'] = dest_address

            # 确保所有输入都有 slice 相关字段（包括指针类型），避免后续统计遗漏
            if 'slice_data_dest_str' not in parent_task:
                parent_task['slice_data_dest_str'] = dest_address
            if 'slice_data_type' not in parent_task:
                parent_task['slice_data_type'] = "0"
            if 'slice_length' not in parent_task:
                parent_task['slice_length'] = "0"

    with open(input_tasks_spec_path, 'w') as f:
        json.dump(task_data, f, indent=4)

def process_ij_folder(ij_folder_path):
    for dag_folder in os.listdir(ij_folder_path):
        dag_name = dag_folder.split(".json")[0]
        dag_folder_path = os.path.join(ij_folder_path, dag_name)
        if os.path.isdir(dag_folder_path):
            process_dag_folder(dag_folder_path)

if __name__ == "__main__":
    ij_folder_path = './IJ'
    process_ij_folder(ij_folder_path)