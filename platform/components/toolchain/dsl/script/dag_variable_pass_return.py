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
def process_dag_folders(ij_folder, return_value_file):
    # Load return_value.json
    return_value = load_json_first(return_value_file)
    if not return_value:
        print("Error: return_value.json is invalid or missing")
        return

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
                if "output" in task_content:
                    task_ouput = task_content["output"]

                    # Compare ouputs with return_value.json and extract full details
                    for key in task_ouput.keys():
                        if key in return_value and key not in matched_data:  
                            matched_data[key] = return_value[key]  # Store full metadata
                            print(f"{dag_folder}, key: {key}")

            # Save reordered data to matched_data.json
            if matched_data:
                save_reordered_data(dag_path, matched_data, return_value, "return_value.json")


# Define paths
ij_folder = "../IJ"  # Replace with the actual IJ folder path
return_value_file = "../variable/map/return_value.json"

# Run processing
process_dag_folders(ij_folder, return_value_file)
