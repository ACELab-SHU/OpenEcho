import re
import json
import os

def find_variable_info(map_filename, variable_name):
    with open(map_filename, 'r') as map_file:
        map_content = map_file.read()

        pattern = re.compile(rf'0x([0-9a-fA-F]+)\s+{variable_name}\s+0x([0-9a-fA-F]+)\s')
        match = pattern.search(map_content)
        if match:
            address = '0x' + match.group(1)
            base_address = 0x00020000
            address_int = int(address, 16)
            offset = address_int - base_address
            length = int(match.group(2), 16) - int(match.group(1), 16)
            return offset, length

    return None

def update_json_with_info(json_filenames, map_filename, json_file_to_extract_offset_from, output_dag_folder):
    try:
        with open(json_file_to_extract_offset_from, 'r') as json_file:
            extracted_data = json.load(json_file)
            base = extracted_data['variable']['data_offset']
    except (FileNotFoundError, json.decoder.JSONDecodeError, KeyError):
        print(f"Error: Unable to extract base offset from file: {json_file_to_extract_offset_from}")
        return

    for json_filename in json_filenames:
        if os.path.exists(json_filename):
            try:
                with open(json_filename, 'r') as json_file:
                    data = json.load(json_file)

                    for variable_name, variable_info in data.items():
                        result = find_variable_info(map_filename, variable_name)
                        if result is not None:
                            offset, length = result
                            address = base + offset
                            variable_info["offset"] = offset
                            if variable_info.get('type') != '0b010':
                                variable_info["length"] = length
                            # print(f"Variable '{variable_name}' offset updated to '{address}' in JSON file: {json_filename}")
                        else:
                            print(f"Variable '{variable_name}' not found in the {json_filename} map file.")
                with open(json_filename, 'w') as json_file:
                    json.dump(data, json_file, indent=4)
            except json.decoder.JSONDecodeError:
                print(f"JSONDecodeError: Unable to parse JSON data from file: {json_filename}")
        else:
            print(f"JSON file not found: {json_filename}")

def process_dag_folders(map_base_path, task_base_path, output_base_path):
    for dag_folder in os.listdir(output_base_path):
        dag_path = os.path.join(output_base_path, dag_folder)
        if os.path.isdir(dag_path):
            
            # 每个 DAG 文件夹有自己独立的 variable.map 文件
            map_filename = os.path.join(dag_path, "variable.map")
            json_file_to_extract_offset_from = os.path.join(dag_path, "combined_task_without_padding.json")
            
            json_filenames = [
                os.path.join(dag_path, "global.json"),
                os.path.join(dag_path, "return_value.json"),
                os.path.join(dag_path, "parameter.json"),
                os.path.join(dag_path, "dag_input.json"),
                os.path.join(dag_path, "dfedata.json"),
                os.path.join(dag_path, "all_data.json")
            ]

            # 更新 JSON 文件，传递当前 DAG 文件夹路径
            update_json_with_info(json_filenames, map_filename, json_file_to_extract_offset_from, dag_path)

if __name__ == "__main__":
    map_base_path = os.getcwd()
    task_base_path = './map'
    output_base_path = '../IJ'
    
    process_dag_folders(map_base_path, task_base_path, output_base_path)