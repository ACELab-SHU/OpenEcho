import os
import json
import re

def load_json_first(file_path):
    try:
        with open(file_path, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading {file_path}: {e}")
        return None
# Function to process a C file (parameter.c or dag_input.c)
def process_c_file(c_file_path, reordered_data, new_file_path):
    if os.path.exists(c_file_path):
        with open(c_file_path, 'r') as f:
            lines = f.readlines()

        updated_lines = []
        for line in lines:
            match = re.match(r'(char|short|int)\s+(\w+)\[\]', line)
            if match:
                var_name = match.group(2)
                if var_name in reordered_data:
                    # Add this line to the updated_lines as it matches
                    updated_lines.append(line)
            else:
                updated_lines.append(line)

        # Write the updated content to the new C file
        with open(new_file_path, 'w') as f:
            f.writelines(updated_lines)
        print(f"Updated {new_file_path} with matched variables")


def load_json(file_path, new_json_file_path):
    if os.path.getsize(file_path) == 0:
        # 如果文件为空，创建一个新的空 JSON 文件
        print(f"{file_path} is empty. Creating a new empty JSON file.")
        with open(new_json_file_path, 'w') as f:
            json.dump({}, f, indent=4)  # 创建一个空的 JSON 文件
        return {}, file_path  # 返回空字典和文件路径

    try:
        # 尝试加载 JSON 数据
        with open(file_path, 'r') as f:
            data = json.load(f)
            return data, file_path
    except json.JSONDecodeError as e:
        # 如果文件内容无效，捕获 JSONDecodeError 异常
        print(f"Error loading {file_path}: Invalid JSON format. {e}")
        # 创建空的 JSON 文件
        with open(new_json_file_path, 'w') as f:
            json.dump({}, f, indent=4)  # 创建一个空的 JSON 文件
        return {}, file_path  # 返回空字典和文件路径
    except Exception as e:
        print(f"Error loading {file_path}: {e}")
        return {}, file_path  # 处理其他错误


# Function to save reordered data to a JSON file with consistent indices, checking original data
def save_reordered_data(dag_path, reordered_data, original_data, filename):
    # Load original data from the corresponding JSON file
    new_json_file_path = os.path.join(dag_path, filename)
    original_file_path = os.path.join("../variable/map", filename)
    # Load original data from the corresponding JSON file
    original_data_content, file_path = load_json(original_file_path, new_json_file_path)
    
    print(f"original_file_path:{original_file_path}")
    print(f"original_data_content:{original_data_content}")
    if not original_data_content:
        print(f"Original data is empty or not found in {original_file_path}. Skipping saving for {filename}.")
        return
    # Filter the reordered data to only include entries that exist in the original data
    filtered_data = {}
    for key, value in reordered_data.items():
        if key in original_data_content:
            filtered_data[key] = value

    # If filtered data exists, save it to the new JSON file
    if filtered_data:        
        with open(new_json_file_path, 'w') as json_file:
            json.dump(filtered_data, json_file, indent=4)
        print(f"Matched data saved to {new_json_file_path}")
    else:
        print(f"No matching data found in {filename} to save.")

# Function to process each DAG folder and update parameter.c, dag_input.c
def process_dag_folders(ij_folder, all_data_file):
    # Load all_data.json
    all_data = load_json_first(all_data_file)
    if not all_data:
        print("Error: all_data.json is invalid or missing")
        return

    # Define the paths for the original .c files
    parameter_c_file = "../variable/parameter.c"
    dag_input_c_file = "../variable/dag_input.c"
    dfedata_c_file = "../variable/dfedata.c"
    global_c_file = "../variable/global.c"
    
    # Traverse each DAG folder in IJ
    for dag_folder in os.listdir(ij_folder):
        dag_path = os.path.join(ij_folder, dag_folder)
        task_file = os.path.join(dag_path, "task_input_output.json")

        # Check if it's a folder and contains task_input_output.json
        if os.path.isdir(dag_path) and os.path.exists(task_file):
            task_data = load_json_first(task_file)
            if not task_data:
                continue  # Skip if the file is invalid
            
            matched_data = {}

            # Iterate through all tasks in task_input_output.json
            for task_content in task_data.values():
                if "input" in task_content:
                    task_input = task_content["input"]

                    # Compare inputs with all_data.json and extract full details
                    for key in task_input.keys():
                        if key in all_data and key not in matched_data:  
                            matched_data[key] = all_data[key]  # Store full metadata
                            print(f"{dag_folder}, key: {key}")

            # **Reorder indices sequentially from 1**
            reordered_data = {}
            new_index = 1
            for key in matched_data:
                reordered_data[key] = matched_data[key]
                reordered_data[key]["index"] = new_index
                new_index += 1  # Increment index

            # Save reordered data to matched_data.json
            if reordered_data:
                save_reordered_data(dag_path, reordered_data, all_data, "all_data.json")

                # New paths for the updated C files in the DAG folder
                new_parameter_c_file = os.path.join(dag_path, "parameter.c")
                new_dag_input_c_file = os.path.join(dag_path, "dag_input.c")
                new_dfedata_c_file = os.path.join(dag_path, "dfedata.c")
                new_global_c_file = os.path.join(dag_path, "global.c")

                # Process both files using the helper function
                process_c_file(parameter_c_file, reordered_data, new_parameter_c_file)
                process_c_file(dag_input_c_file, reordered_data, new_dag_input_c_file)
                process_c_file(dfedata_c_file, reordered_data, new_dfedata_c_file)
                process_c_file(global_c_file, reordered_data, new_global_c_file)

                # Ensure other new JSON files also have consistent indices and data in original JSON
                save_reordered_data(dag_path, reordered_data, "../variable/map/parameter.json", "parameter.json")
                save_reordered_data(dag_path, reordered_data, "../variable/map/dag_input.json", "dag_input.json")
                save_reordered_data(dag_path, reordered_data, "../variable/map/dfedata.json", "dfedata.json")
                
# Define paths
ij_folder = "../IJ"  # Replace with the actual IJ folder path
all_data_file = "../variable/map/all_data.json"

# Run processing
process_dag_folders(ij_folder, all_data_file)
