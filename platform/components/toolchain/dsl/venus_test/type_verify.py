import os
import re
import json
from scalar_o import strip_comments
from venus_ext_vector_normalize import normalize_venus_ext_vector_source

def convert_type(data_type):
    """
    将参数类型做最小必要的归一化，但必须保留向量类型语义。

    重要：__vNNNiM（或 ext_vector_type typedef alias）属于向量类型，
    不能折叠成 char/short/int，否则会导致 DSL 把向量输入误识别为标量。
    """
    t = (data_type or "").strip()

    # Preserve Venus vector types such as "__v4096i8", "__v4100i16", etc.
    if re.fullmatch(r'__v\d+i\d+', t):
        return t

    # Preserve common struct-style scalar wrappers (they are scalar semantics).
    if t.endswith("_struct"):
        return t

    # 定义基础标量类型转换规则
    if t in ("short", "int", "char", "double", "float"):
        return t

    # Fallback: keep original text (e.g., user-defined struct types like nrPDCCHConfig)
    return t  # 未知类型保持不变

def extract_function_arguments(file_path, function_name):
    # 正则表达式匹配函数定义
    function_pattern = re.compile(rf'\b(\w+)\s+{function_name}\s*\((.*?)\)\s*{{', re.S)
    
    arg_pattern = re.compile(r'\s*(\w+)\s+(\w+)(?:\s*,\s*)?')

    function_info = {'args': []}

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        content_no_comments = normalize_venus_ext_vector_source(strip_comments(content))
        match = function_pattern.search(content_no_comments)
        if match:
            args = match.group(2)
            arg_types = arg_pattern.findall(args)
            for arg_type, arg_name in arg_types:
                converted_type = convert_type(arg_type)  # 转换类型
                function_info['args'].append({'name': arg_name, 'type': converted_type})

    return function_info

def analyze_directory(directory):
    result = {}

    for filename in os.listdir(directory):
        if filename.endswith('.c'):
            file_path = os.path.join(directory, filename)
            function_name = os.path.splitext(filename)[0]  # 去掉扩展名，得到函数名
            function_info = extract_function_arguments(file_path, function_name)
            print(f"function_name:{function_name}")
            print(f"function_info:{function_info}")
            if function_info['args']:  # 仅当函数有参数时才添加到结果中
                result[function_name] = function_info

    return result

def save_to_json(data, output_file):
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=4)

# 使用示例
if __name__ == '__main__':
    function_info = analyze_directory(os.getcwd())
    save_to_json(function_info, 'input_type.json')
    print('Function information has been saved to input_type.json.')
