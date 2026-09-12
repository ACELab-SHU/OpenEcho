import os
import json

def extract_c_file_names(directory):
    c_files = [file[:-2] for file in os.listdir(directory) if file.endswith('.c')]
    return c_files

def update_json_with_dest_address(venus_test_path):
    function_names = extract_c_file_names(os.getcwd())
    data = {}  # Dictionary to store function names and vns_delimit_counts

    for function_name in function_names:
        print(f"function_name:{function_name}")
        asm_file_path = os.path.join(function_name + '_input_scalar_num.txt')
        with open(asm_file_path, 'r') as asm_file:
            data[function_name] = int(asm_file.read().strip())

    json_file_path = os.path.join(os.getcwd(),'task_input_num.json')
    with open(json_file_path, 'w') as outfile:
        json.dump(data, outfile, indent=4)

if __name__ == "__main__":

    venus_test_base_path = '../venus_test'
    
    update_json_with_dest_address(venus_test_base_path)
