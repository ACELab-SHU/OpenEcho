import os

c_files = [f for f in os.listdir('./dags/cbin') if f.endswith('_bin.c')]



for c_file in c_files:
    dag_name = os.path.splitext(c_file)[0]
    bin_index = dag_name.find("_bin")
    print(dag_name,'dag info added to ./include/daginfo.h')
    print("--------------------------------------------------------------------------------------------------------------------------------------------------------------- ")

    if bin_index != -1:  # 如果找到了 "_bin"
        dag_name = dag_name[:bin_index]  # 使用切片获取 "_bin" 之前的部分
    # DAG_NAME = DAG_NAME = dag_name.upper()

    jacklight_string = f"// python start jacklight"

    insert_string =  f"\nextern unsigned int {dag_name}_bin;"
    insert_string += f"\nextern unsigned int {dag_name}_bin_size;"
    insert_string += f"\nextern unsigned int {dag_name}_task_container;"
    insert_string += f"\nextern unsigned int {dag_name}_task_container_size;"
    insert_string += f"\nextern unsigned int {dag_name}_global_para;"
    insert_string += f"\nextern unsigned int {dag_name}_global_para_size;"
    insert_string += f"\nextern unsigned int {dag_name}_output_num;"
    insert_string += f"\nextern unsigned int {dag_name}_output_num_size;"
    insert_string += f"\nextern unsigned int {dag_name}_task_num;"
    insert_string += f"\nextern unsigned int {dag_name}_task_num_size;"
    insert_string += f"\nextern unsigned int {dag_name}_return_value;"
    insert_string += f"\nextern unsigned int {dag_name}_return_value_size;"
    insert_string += f"\nextern unsigned int {dag_name}_output_addr;"               
    insert_string += f"\nextern unsigned int {dag_name}_output_addr_size;"
    insert_string += f"\nextern unsigned int {dag_name}_input_offset[];"
    insert_string += f"\nextern unsigned int {dag_name}_input_length[];"
    insert_string += f"\nextern unsigned int {dag_name}_binandjsonsize;\n"

    with open('./include/daginfo.h', 'r') as f:
        contents = f.read()

    if insert_string not in contents:
        insert_location = contents.find('// python end jacklight')

        new_contents = contents[:insert_location] + jacklight_string + insert_string + contents[insert_location:]

        with open('./include/daginfo.h', 'w') as f:
            f.write(new_contents)