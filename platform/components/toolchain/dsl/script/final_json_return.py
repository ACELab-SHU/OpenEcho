import json
import os
import shutil

def fill(final_json, map_json_path, output_json):
    # Check if map_json_path file is empty
    if not os.path.exists(map_json_path):
        print(f"!!Warning: {map_json_path} is no exist. Skipping processing.")
        with open(map_json_path, 'w') as f:
            pass  # 创建空文件，不写任何内容

    if os.path.getsize(map_json_path) == 0:
        shutil.copyfile(output_json, final_json)
        print(f"!!Warning: {map_json_path} is empty. Skipping processing.")
        print(f"========================== WORK COMPLETED ==========================")
        print(f"\n")
        return
    
    with open(output_json, 'r') as f:
        output_data = json.load(f)
    
    with open(map_json_path, 'r') as f:
        data = json.load(f)

    input_key = "return_output"
    
    for item in output_data:
        if input_key in item and item[input_key] is not None:
            for var in item[input_key]:
                if 'name' in var and var['name'] in data:
                    info = data[var['name']]
                    var.update(info)

        # 透传 all_output 字段（如果存在）
        if 'all_output' in item:
            pass  # all_output 已存在，保持不变

    check_slice_length = "all_input"

    for item2 in output_data:
        if check_slice_length in item2 and item2[check_slice_length] is not None:
            for var2 in item2[check_slice_length]:
                if 'slice_length' in var2 and 'length' in var2:
                    slice_length = int(var2['slice_length'])
                    length = int(var2['length'])
                    if slice_length > length:
                        raise ValueError("slice_length cannot be greater than length")

    # 重新计算所有 task 的 Input_Num 和 Output_Num（确保最终输出正确）
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

    print(f"========================== WORK COMPLETED ==========================")
    print(f"\n")
def process_dag_files(map_folder_path, output_base_path, final_base_path):
    for dag_folder in os.listdir(output_base_path):
        dag_base= dag_folder.split(".json")[0]
        dag_path = os.path.join(output_base_path, dag_base)
        var_json_path = os.path.join(map_folder_path, dag_base, "return_value.json")
        if os.path.isdir(dag_path):
            final_json_path = os.path.join(final_base_path, f"{dag_base}.json")
            output_json_path = os.path.join(output_base_path, dag_base, "final_all_input.json")
            fill(final_json_path, var_json_path, output_json_path)

output_base_path = "./IJ"
map_folder_path = "./IJ"
final_base_path = "./final_output"
process_dag_files(map_folder_path, output_base_path, final_base_path)