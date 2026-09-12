import json
import os
import sys

_script_dir = os.path.dirname(os.path.abspath(__file__))
if _script_dir not in sys.path:
    sys.path.insert(0, _script_dir)

from type_codes import TYPE_PARAM_GLOBAL, POINTER_TYPES

def replace_slice_task(tasks, slice_data, tasks_io, temp_alloc_map):
    # Create a lookup dictionary for slice data
    slice_lookup = {key: value for key, value in slice_data.items()}

    for task in tasks:
        # 保留 all_output 字段（如果存在）
        original_all_output = task.get('all_output', None)
        # Check if task has parentTasks and iterate through them
        if 'parentTasks' in task:
            new_parent_tasks = []
            for parent_task in task['parentTasks']:
                if 'outputVar' in parent_task and parent_task['taskId'] == 'slice':
                    output_var = parent_task['outputVar']
                    if output_var in slice_lookup:
                        slice_var = slice_lookup[output_var]['slice_var']
                        # 若 dest_address 仍是 null 且 slice_var 是临时变量，用分配表地址
                        resolved_dest = parent_task['dest_address']
                        if resolved_dest == "null" and slice_var in temp_alloc_map:
                            resolved_dest = temp_alloc_map[slice_var]['dest_address']

                        # Find the corresponding entry in para_Input
                        for task2 in tasks:
                            for para in task2['para_Input']:
                                if para['name'] == slice_var:
                                    # Preserve dest_address
                                    para_copy = para.copy()
                                    para_copy['dest_address'] = resolved_dest
                                    para_copy['slice_length'] = slice_lookup[output_var]['slice_length']
                                    para_copy['slice_data_type'] = slice_lookup[output_var]['data_type']
                                    # 保留原有的 type 和 is_pointer 字段，不覆盖
                                    if 'type' not in para_copy:
                                        para_copy['type'] = TYPE_PARAM_GLOBAL
                                    if 'is_pointer' not in para_copy:
                                        t = para_copy.get('type')
                                        para_copy['is_pointer'] = (t in POINTER_TYPES or t == '0b10')
                                    # 确保有 slice_data_dest_str 字段
                                    if 'slice_data_dest_str' not in para_copy:
                                        para_copy['slice_data_dest_str'] = resolved_dest
                                    # Add the copied entry to para_Input
                                    task['para_Input'].append(para_copy)
                                    break
                            break

                        for task_name, io_data in tasks_io.items():
                            for output_key, output_value in io_data.get('output', {}).items():
                                if output_key == slice_var:
                                    # Preserve dest_address and add slice details
                                    new_slice = {
                                        'taskId':task_name,
                                        'outputVar': slice_var,
                                        'outputIndex': output_value,
                                        'dest_address': resolved_dest,
                                        'concat_value': 0,
                                        'slice_length': slice_lookup[output_var]['slice_length'],
                                        'slice_data_type': slice_lookup[output_var]['data_type']
                                        }
                                    task['parentTasks'].append(new_slice)
                                    break
                            break
                    # Do not add this parent task to new_parent_tasks to effectively remove it
                else:
                    new_parent_tasks.append(parent_task)
            task['parentTasks'] = new_parent_tasks

        if 'childTasks' in task:
            new_child_tasks = []
            for child_task in task['childTasks']:
                if child_task['taskId'] == 'slice':
                    slice_map_var = child_task['inputVar']
                    for slice_var, slice_details in slice_lookup.items():
                        if slice_map_var in slice_lookup[slice_var]['slice_var']:
                            child_task['taskId'] = slice_lookup[slice_var]['taskid']
                            new_child_tasks.append(child_task)
                            break
                else:
                    new_child_tasks.append(child_task)
            task['childTasks'] = new_child_tasks

        # 恢复 all_output 字段
        if original_all_output is not None:
            task['all_output'] = original_all_output

    return tasks

def process_ij_folder(ij_folder_path):
    for dag_folder in os.listdir(ij_folder_path):
        dag_name = dag_folder.split(".json")[0]
        dag_folder_path = os.path.join(ij_folder_path, dag_name)
        dag_input_tasks_spec = os.path.join(ij_folder_path, dag_name,"input_tasks_spec.json")
        dag_task_input_output = os.path.join(ij_folder_path, dag_name,"task_input_output.json")
        dag_slice = os.path.join('./variable/map/', f"{dag_name}_slice.json")
        dag_slice_updated_tasks = os.path.join(ij_folder_path, dag_name,"slice_updated_tasks.json")
        with open(dag_input_tasks_spec, 'r') as f:
            tasks_data = json.load(f)

        with open(dag_task_input_output, 'r') as f:
            tasks_io = json.load(f)

        with open(dag_slice, 'r') as f:
            slice_data = json.load(f)

        # 加载临时变量分配表（若存在），key = 变量名
        temp_alloc_map = {}
        temp_alloc_path = os.path.join('./variable/map/', f"{dag_name}_temp_alloc.json")
        if os.path.exists(temp_alloc_path):
            with open(temp_alloc_path, 'r') as f:
                temp_alloc_list = json.load(f)
            temp_alloc_map = {entry['name']: entry for entry in temp_alloc_list}

        if os.path.isdir(dag_folder_path):
            updated_tasks = replace_slice_task(tasks_data, slice_data, tasks_io, temp_alloc_map)

        with open(dag_slice_updated_tasks, 'w') as f:
            json.dump(updated_tasks, f, indent=4)

if __name__ == "__main__":
    ij_folder_path = './IJ'
    process_ij_folder(ij_folder_path)