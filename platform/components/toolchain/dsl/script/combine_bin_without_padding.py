import json
import os
import sys

# parameter / dag_input 若原本位于 0x8000 以下，则从 0x8000 开始，
# 避免数据对象跨越硬件 LSU 0x8000 边界。task 镜像本身可以超过该边界。
VARIABLE_DATA_BASE_OFFSET = 0x8000

def combine_binaries_and_update_json(output_bin, dag_name, task_json, output_json, venus_test_path, variable_path, VenusInputStructAddr):
    with open(output_bin, 'wb') as output_bin_file:
        with open(task_json, 'r+') as task_info_file:
            task_info = json.load(task_info_file)
            offset = 0
            processed_tasks = {}  # Store task_data for each actual_task_name

            for task_name, task_data in task_info.items():
                if task_name != "variable":
                    actual_task_name = task_name.split('*')[0] if '*' in task_name else task_name
                    if actual_task_name not in processed_tasks:
                        bin_file_path = os.path.join(venus_test_path, actual_task_name + '.bin')
                        if bin_file_path != './venus_test/concat.bin' and bin_file_path != './venus_test/slice.bin' and actual_task_name not in processed_tasks:
                            with open(bin_file_path, 'rb') as input_bin_file:
                                data = input_bin_file.read()

                                text_length = task_data['text_length']
                                data_length = task_data['data_length']

                                text_end = text_length
                                data_end = 0x00020000 + data_length

                                aligned_text_length = (text_length + 63) & ~63
                                if 65473 <= data_length <= 65535:
                                    aligned_data_length = 65535
                                else:
                                    aligned_data_length = (data_length + 63) & ~63
                                if(aligned_data_length > VenusInputStructAddr):
                                    print("VenusInputStructAddr Error: aligned_data_length exceeds the VenusInputStructAddr!!!")
                                    sys.exit(1)
                                    
                                text_padding = aligned_text_length - text_length
                                data_padding = aligned_data_length - data_length
                                
                                text_segment = data[:text_length]
                                data_segment = data[0x00020000:data_end]

                                if len(data_segment) < data_length:
                                    data_segment += b'\x00' * (data_length - len(data_segment))
                                    print(f"!!!!!!!!!!padding zero!!!!!!!!!")

                                data1 = text_segment + b'\x00' * text_padding
                                data2 = data_segment + b'\x00' * data_padding
                                data = data1 +data2

                                output_bin_file.write(data)
                                length = len(data)
                                print(f"task_id: {actual_task_name}")

                                print(f"text_length: {text_length}, data_length: {data_length}, text_padding: {text_padding}, data_padding: {data_padding}")

                                task_data['text_offset'] = offset
                                task_data['data_offset'] = offset + aligned_text_length
                                task_data['total_length'] = length
                                task_data['text_length'] = aligned_text_length
                                task_data['data_length'] = aligned_data_length
                                processed_tasks[actual_task_name] = task_data  
                                task_info[task_name] = task_data
                                
                                offset += length

                                if length != (aligned_text_length + aligned_data_length):
                                    raise ValueError("Length does not equal the sum of aligned_text_length and aligned_data_length")
                                
                    elif actual_task_name in processed_tasks:
                        task_data = processed_tasks[actual_task_name]
                        task_info[task_name] = task_data


                else:
                    if offset < VARIABLE_DATA_BASE_OFFSET:
                        padding_size = VARIABLE_DATA_BASE_OFFSET - offset
                        output_bin_file.write(b'\x00' * padding_size)
                        offset = VARIABLE_DATA_BASE_OFFSET

                    variable_bin_path = os.path.join('./IJ', dag_name, 'variable.bin')
                    with open(variable_bin_path, 'rb') as input_bin_file:
                        data = input_bin_file.read()
                        
                        data_length = task_data['data_length']

                        # if task_data['dag_input_origin_address'] != 0:
                        #     dfedata_dead_addr = int(task_data['dag_input_origin_address'], 16)-int("00020000", 16)
                        # if (task_data['dfedata_origin_address'] != 0
                                # and task_data['dag_input_origin_address'] != 0):
                            # dfedata_dead_addr = int(task_data['dfedata_origin_address'], 16) - ...
                        if task_data['dfedata_origin_address'] != 0:
                            dfedata_dead_addr = int(task_data['dfedata_origin_address'], 16) - 0x00020000
                            data_no_dfe = data[0: dfedata_dead_addr]
                            if 65473 <= len(data_no_dfe) <= 65535:
                                aligned_data_length = 65535
                            else:
                                aligned_data_length = (len(data_no_dfe) + 63) & ~63
                            data_padding = aligned_data_length - len(data_no_dfe)
                            data_no_dfe = data_no_dfe + b'\x00' * data_padding
                        else:
                            if 65473 <= data_length <= 65535:
                                aligned_data_length = 65535
                            else:
                                aligned_data_length = (data_length + 63) & ~63
                            data_padding = aligned_data_length - data_length
                            data = data + b'\x00' * data_padding
                            data_no_dfe = data

                        output_bin_file.write(data_no_dfe)
                        length = len(data_no_dfe)
                        length_origin = len(data)

                        task_data['data_offset'] = offset
                        task_data['total_length'] = length_origin
                        task_data['data_length'] = length

                        offset += length

        with open(output_json, 'w') as output_json_file:
            json.dump(task_info, output_json_file, indent=4)

def process_dag_folders(base_path, venus_test_base_path, variable_base_path, output_bin_base_path, VenusInputStructAddr):
    os.makedirs(output_bin_base_path, exist_ok=True)

    for dag_folder in os.listdir(base_path):
        dag_path = os.path.join(base_path, dag_folder)
        if os.path.isdir(dag_path):
            dag_name = dag_folder.split(".json")[0]
            output_bin_path = os.path.join(output_bin_base_path, f"{dag_folder}.bin")
            task_json_path = os.path.join(variable_base_path, 'map', f"{dag_name}_task.json")
            output_json_path = os.path.join(dag_path, 'combined_task_without_padding.json')
            venus_test_path = os.path.join(venus_test_base_path)
            variable_path = os.path.join(variable_base_path)
            combine_binaries_and_update_json(output_bin_path, dag_name, task_json_path, output_json_path, venus_test_path, variable_path, VenusInputStructAddr)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python combine_bin_without_padding.py <VenusInputStructAddr>")
        sys.exit(1)
    VenusInputStructAddr = int(int(sys.argv[1], 16) - 0x80020000)  
    base_path = './IJ'
    venus_test_base_path = './venus_test/ir'
    variable_base_path = './variable'
    output_bin_base_path = './bin'

    process_dag_folders(base_path, venus_test_base_path, variable_base_path, output_bin_base_path, VenusInputStructAddr)
