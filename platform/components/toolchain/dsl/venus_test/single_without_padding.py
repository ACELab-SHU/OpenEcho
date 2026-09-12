import json
import os

def combine_binaries_and_update_json(output_bin, task_json,output_bin_path):

        with open(task_json, 'r+') as task_info_file:
            task_data = json.load(task_info_file)
            offset = 0
            with open(output_bin, 'rb') as input_bin_file:
                data = input_bin_file.read()
                text_length = task_data["single_task_test"]['text_length']
                text_end = offset + text_length
                data_length = task_data["single_task_test"]['data_length']
                data_end = 0x00020000 + data_length
                aligned_text_length = (text_length + 63) & ~63
                aligned_data_length = (data_length + 63) & ~63
                text_padding = aligned_text_length - text_length
                data_padding = aligned_data_length - data_length
                data = data[:text_end - offset] + b'\x00' * text_padding + \
                data[0x00020000:data_end] + b'\x00' * data_padding

                length = len(data)
                task_data["single_task_test"]['text_length'] = aligned_text_length
                task_data["single_task_test"]['data_offset'] = aligned_text_length
                task_data["single_task_test"]['data_length'] = aligned_data_length
            
            with open(output_bin_path, 'wb') as output_bin_file:
                output_bin_file.write(data)

            with open(task_json, 'w') as output_json_file:
                json.dump(task_data, output_json_file, indent=4)
if __name__ == "__main__":

    input_bin_path = "./ir/single_task_test.bin"
    output_bin_path = "../bin/after_padding_single_task_test.bin"
    task_json_path = "single_task_test.json"
    combine_binaries_and_update_json(input_bin_path, task_json_path,output_bin_path)
