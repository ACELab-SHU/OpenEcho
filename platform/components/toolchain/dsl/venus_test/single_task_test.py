import re
import json
import os

def find_variable_lengths(map_filename):
    text_length = None
    data_length = None
    
    with open(map_filename, 'r') as map_file:
        map_content = map_file.read()

        text_pattern = re.compile(r'\.text\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)')
        data_pattern = re.compile(r'combined_data\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)')
        
        text_match = text_pattern.search(map_content)
        data_match = data_pattern.search(map_content)
        
        if text_match:
            address = text_match.group(1)
            length = int(text_match.group(2), 16)
            text_length = length
            
        if data_match:
            address = data_match.group(1)
            length = int(data_match.group(2), 16)
            data_length = length
    
    return text_length, data_length

def update_task_json_with_lengths(task_json_path, venus_path):
    task_data = {
        "single_task_test": {
            "text_length": "1",
            "data_length": "1",
            "data_offset": "1"
        }
    }

    variable_name = "single_task_test"
    map_filename = os.path.join(venus_path, variable_name + '.map')
    if os.path.exists(map_filename):
        text_lengths, data_lengths = find_variable_lengths(map_filename)
        task_data[variable_name]["text_length"] = text_lengths
        task_data[variable_name]["data_length"] = data_lengths
        task_data[variable_name]["data_offset"] = text_lengths

    else:
        print(f"Map file not found for variable '{variable_name}'")

    with open(task_json_path, 'w') as task_json_file:
        json.dump(task_data, task_json_file, indent=4)

if __name__ == "__main__":
    venus_path = os.getcwd()
    task_json_path = "single_task_test.json"
    update_task_json_with_lengths(task_json_path, venus_path)