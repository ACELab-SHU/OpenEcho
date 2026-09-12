import json
import os

def fill_index(final_json, source, output_json, temp_alloc_path):
    with open(source, 'r') as f:
        data = json.load(f)

    # 读取output.json文件
    with open(output_json, 'r') as f:
        output_data = json.load(f)

    # 读取 temp_alloc.json（如果存在）
    temp_alloc_map = {}
    if os.path.exists(temp_alloc_path) and os.path.getsize(temp_alloc_path) > 0:
        with open(temp_alloc_path, 'r') as f:
            temp_alloc_list = json.load(f)
        temp_alloc_map = {entry['name']: entry for entry in temp_alloc_list}
    
    for item in output_data:
        if 'taskId' in item and item['taskId'] is not None :
            taskId = item['taskId']
            actual_task_name = taskId.split('*')[0] if '*' in taskId else taskId
            if actual_task_name == "concat" or actual_task_name == "slice":
                continue
            if taskId in data:
                text_offset = data[taskId]['text_offset']
                data_offset = data[taskId]['data_offset']
                total_length = data[taskId]['total_length']
                text_length = data[taskId]['text_length']
                data_length = data[taskId]['data_length']
                hash = data[taskId]['hash']
                item['text_offset'] = text_offset
                item['data_offset'] = data_offset
                item['total_length'] = total_length
                item['text_length'] = text_length
                item['data_length'] = data_length
                item['hash'] = hash
                item['is_spmd'] = item.get('is_spmd', 0)
                item['min_core_num'] = item.get('min_core_num', 1)

                # 生成 all_output 字段
                all_output = []
                for var_name, var_info in temp_alloc_map.items():
                    if var_info.get('producer_task_id') == taskId:
                        output_entry = {
                            "name": var_name,
                            "parentTasksPort": format(var_info.get('producer_output_port', 0), '#012b'),
                            "temp_offset": var_info.get('temp_offset', 0),
                            "length": var_info.get('size_bytes', 0)
                        }
                        all_output.append(output_entry)

                if all_output:
                    item['all_output'] = all_output

    with open(final_json, 'w') as f:
        json.dump(output_data, f, indent=4)

def process_dag_folders(base_path):
    for dag_folder in os.listdir(base_path):
        dag_base= dag_folder.split(".json")[0]
        dag_path = os.path.join(base_path, dag_base)
        if os.path.isdir(dag_path):
            final_json_path = os.path.join(dag_path, "input_tasks_spec.json")
            source = os.path.join(dag_path, "combined_task_without_padding.json")
            output_json_path = os.path.join(dag_path, "input_tasks.json")
            temp_alloc_path = os.path.join('./variable/map/', f"{dag_base}_temp_alloc.json")
            fill_index(final_json_path, source, output_json_path, temp_alloc_path)

if __name__ == "__main__":
    base_path = "./IJ"
    process_dag_folders(base_path)