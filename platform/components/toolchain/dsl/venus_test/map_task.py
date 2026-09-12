import re
import json
import os

def find_variable_lengths(map_filename):
    text_length = None
    data_length = None
    dfe_address = 0
    dfe_data_length = None
    dag_input_address = 0
    dag_input_data_length = None
    with open(map_filename, 'r') as map_file:
        map_content = map_file.read()

        text_pattern = re.compile(r'\.text\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)')
        data_pattern = re.compile(r'combined_data\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)')
        dfedata_pattern = re.compile(r'venus_dfedata\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)')
        dag_input_pattern = re.compile(r'venus_dag_input\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)')
        text_match = text_pattern.search(map_content)
        data_match = data_pattern.search(map_content)
        dfedata_match = dfedata_pattern.search(map_content)
        dag_input_match = dag_input_pattern.search(map_content)

        if text_match:
            address = text_match.group(1)
            length = int(text_match.group(2), 16)
            text_length = length
            
        if data_match:
            address = data_match.group(1)
            length = int(data_match.group(2), 16)
            data_length = length
        
        if dfedata_match:
            dfe_address = dfedata_match.group(1)
            dfe_length = int(dfedata_match.group(2), 16)
            dfe_data_length = dfe_length
        
        if dag_input_match:
            dag_input_address = dag_input_match.group(1)
            dag_input_length = int(dag_input_match.group(2), 16)
            dag_input_data_length = dag_input_length

    return text_length, data_length, dfe_address, dag_input_address

def update_task_json_with_lengths(task_json_path, venus_path):
    with open(task_json_path, 'r') as task_json_file:
        task_data = json.load(task_json_file)
        
        for variable_name, _ in task_data.items():
            actual_variable_name = variable_name.split('*')[0] if '*' in variable_name else variable_name
            map_filename = os.path.join(venus_path, actual_variable_name + '.map')
            if os.path.exists(map_filename):
                text_lengths, data_lengths, _, _ = find_variable_lengths(map_filename)
                hash_file_path = os.path.join(venus_path, actual_variable_name + '.hash')
                if os.path.exists(hash_file_path):
                    with open(hash_file_path, 'r') as hash_file:
                        hash_value = hash_file.read().strip()
                        hash_value = '0x' + hash_value  
                else:
                    hash_value = None
                
                task_data[variable_name]["text_length"] = text_lengths
                task_data[variable_name]["data_length"] = data_lengths
                task_data[variable_name]["hash"] = hash_value
            else:
                print(f"Map file not found for variable '{variable_name}'")

    with open(task_json_path, 'w') as task_json_file:
        json.dump(task_data, task_json_file, indent=4)

def process_task_json_files(map_base_path, task_base_path, venus_path):
    for filename in os.listdir(task_base_path):
        if filename.endswith("_task.json"):
            task_json_path = os.path.join(task_base_path, filename)         
            variable_name = filename.replace('_task.json', '')
            update_task_json_with_lengths(task_json_path, venus_path)
            map_filename = os.path.join('../IJ/', variable_name, "variable.map")
            if os.path.exists(task_json_path):
                with open(task_json_path, 'r') as task_json_file:
                    existing_data = json.load(task_json_file)
                    text_lengths, data_lengths, dfe_origin_addr, dag_input_origin_addr = find_variable_lengths(map_filename)

                    task_data = {"variable": {"data_length": data_lengths,"dfedata_origin_address": dfe_origin_addr,"dag_input_origin_address": dag_input_origin_addr}}
            else:
                existing_data = {}

            existing_data.update(task_data)

            with open(task_json_path, 'w') as task_json_file:
                json.dump(existing_data, task_json_file, indent=4)

if __name__ == "__main__":
    map_base_path = '../variable'
    task_base_path = '../variable/map'
    venus_path = os.getcwd()

    process_task_json_files(map_base_path, task_base_path, venus_path)