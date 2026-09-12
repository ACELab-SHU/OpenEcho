# This file provides the runtime support for running a basic program
# Assumes the program has been parsed using basparse.py
# from dependency_graph import draw_dependency_graph
# from dependency import FunctionNode, DependencyGraph
# from resource_analysis import analyze_resource
import sys
import math
import random
import re
import json
import os

_DSL_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _DSL_ROOT not in sys.path:
    sys.path.insert(0, _DSL_ROOT)
from script.type_codes import (
    TYPE_PARAM_GLOBAL,
    TYPE_DAG_DFE_STATIC,
    TYPE_PTR_TEMP,
    TYPE_PTR_PARAM_GLOBAL,
    TYPE_PTR_DAG_DFE,
    TYPE_RETURN_VALUE,
)


TYPE_BYTE_WIDTH = {
    "char": 1,
    "short": 2,
    "int": 4,
    "float": 4,
    "double": 8,
}


def byte_length(datatype, element_count):
    if datatype not in TYPE_BYTE_WIDTH:
        raise ValueError(f"Unsupported datatype '{datatype}' for byte length calculation")
    return int(element_count) * TYPE_BYTE_WIDTH[datatype]


class BasicInterpreter:
    # graph = DependencyGraph()
    node_names = []
    data_table = {} 
    # Initialize the interpreter. prog is a dictionary
    # containing (line,statement) mappings
    def __init__(self, prog):
        self.prog = prog

        self.functions = {           # Built-in function table
            'SIN': lambda z: math.sin(self.eval(z)),
            'COS': lambda z: math.cos(self.eval(z)),
            'TAN': lambda z: math.tan(self.eval(z)),
            'ATN': lambda z: math.atan(self.eval(z)),
            'EXP': lambda z: math.exp(self.eval(z)),
            'ABS': lambda z: abs(self.eval(z)),
            'LOG': lambda z: math.log(self.eval(z)),
            'SQR': lambda z: math.sqrt(self.eval(z)),
            'INT': lambda z: int(self.eval(z)),
            'RND': lambda z: random.random()
        }


    # Collect all data statements
    def collect_data(self):
        self.data = []
        for lineno in self.stat:
            if self.prog[lineno][0] == 'DATA':
                self.data = self.data + self.prog[lineno][1]              
        self.dc = 0                  # Initialize the data counter

    # Check for end statements
    def check_end(self):
        has_end = 0
        for lineno in self.stat:
            if self.prog[lineno][0] == 'END' and not has_end:
                has_end = lineno
        if not has_end:
            print("NO END INSTRUCTION")
            self.error = 1
            return
        if has_end != lineno:
            print("END IS NOT LAST")
            self.error = 1

    # Check loops
    def check_loops(self):
        for pc in range(len(self.stat)):
            lineno = self.stat[pc]
            if self.prog[lineno][0] == 'FOR':
                forinst = self.prog[lineno]
                loopvar = forinst[1]
                for i in range(pc + 1, len(self.stat)):
                    if self.prog[self.stat[i]][0] == 'NEXT':
                        nextvar = self.prog[self.stat[i]][1]
                        if nextvar != loopvar:
                            continue
                        self.loopend[pc] = i
                        break
                else:
                    print("FOR WITHOUT NEXT AT LINE %s" % self.stat[pc])
                    self.error = 1

    # Evaluate an expression
    def eval(self, expr):
        etype = expr[0]
        if etype == 'NUM':
            return expr[1]            
        elif etype == 'GROUP':
            return self.eval(expr[1])
        elif etype == 'UNARY':
            if expr[1] == '-':
                return -self.eval(expr[2])
        elif etype == 'BINOP':
            if expr[1] == '+':
                return self.eval(expr[2]) + self.eval(expr[3])
            elif expr[1] == '-':
                return self.eval(expr[2]) - self.eval(expr[3])
            elif expr[1] == '*':
                return self.eval(expr[2]) * self.eval(expr[3])
            elif expr[1] == '/':
                return float(self.eval(expr[2])) / self.eval(expr[3])
            elif expr[1] == '^':
                return abs(self.eval(expr[2]))**self.eval(expr[3])
        elif etype == 'VAR':
            var, dim1, dim2 = expr[1]
            if not dim1 and not dim2:
                if var in self.vars:
                    return self.vars[var]
                else:
                    print("UNDEFINED VARIABLE %s AT LINE %s" %
                          (var, self.stat[self.pc]))
                    raise RuntimeError
            # May be a list lookup or a function evaluation
            if dim1 and not dim2:
                if var in self.functions:
                    # A function
                    return self.functions[var](dim1)
                else:
                    # A list evaluation
                    if var in self.lists:
                        dim1val = self.eval(dim1)
                        if dim1val < 1 or dim1val > len(self.lists[var]):
                            print("LIST INDEX OUT OF BOUNDS AT LINE %s" %
                                  self.stat[self.pc])
                            raise RuntimeError
                        return self.lists[var][dim1val - 1]
            if dim1 and dim2:
                if var in self.tables:
                    dim1val = self.eval(dim1)
                    dim2val = self.eval(dim2)
                    if dim1val < 1 or dim1val > len(self.tables[var]) or dim2val < 1 or dim2val > len(self.tables[var][0]):
                        print("TABLE INDEX OUT OUT BOUNDS AT LINE %s" %
                              self.stat[self.pc])
                        raise RuntimeError
                    return self.tables[var][dim1val - 1][dim2val - 1]
            print("UNDEFINED VARIABLE %s AT LINE %s" %
                  (var, self.stat[self.pc]))
            raise RuntimeError

    # Evaluate a relational expression
    def releval(self, expr):
        etype = expr[1]
        lhs = self.eval(expr[2])
        rhs = self.eval(expr[3])
        if etype == '<':
            if lhs < rhs:
                return 1
            else:
                return 0

        elif etype == '<=':
            if lhs <= rhs:
                return 1
            else:
                return 0

        elif etype == '>':
            if lhs > rhs:
                return 1
            else:
                return 0

        elif etype == '>=':
            if lhs >= rhs:
                return 1
            else:
                return 0

        elif etype == '=':
            if lhs == rhs:
                return 1
            else:
                return 0

        elif etype == '<>':
            if lhs != rhs:
                return 1
            else:
                return 0

    # Assignment
    def assign(self, target, value):
        var, dim1, dim2 = target
        if not dim1 and not dim2:
            self.vars[var] = self.eval(value)
        elif dim1 and not dim2:
            # List assignment
            dim1val = self.eval(dim1)
            if not var in self.lists:
                self.lists[var] = [0] * 10

            if dim1val > len(self.lists[var]):
                print ("DIMENSION TOO LARGE AT LINE %s" % self.stat[self.pc])
                raise RuntimeError
            self.lists[var][dim1val - 1] = self.eval(value)
        elif dim1 and dim2:
            dim1val = self.eval(dim1)
            dim2val = self.eval(dim2)
            if not var in self.tables:
                temp = [0] * 10
                v = []
                for i in range(10):
                    v.append(temp[:])
                self.tables[var] = v
            # Variable already exists
            if dim1val > len(self.tables[var]) or dim2val > len(self.tables[var][0]):
                print("DIMENSION TOO LARGE AT LINE %s" % self.stat[self.pc])
                raise RuntimeError
            self.tables[var][dim1val - 1][dim2val - 1] = self.eval(value)

    # Change the current line number
    def goto(self, linenum):
        if not linenum in self.prog:
            print("UNDEFINED LINE NUMBER %d AT LINE %d" %
                  (linenum, self.stat[self.pc]))
            raise RuntimeError
        self.pc = self.stat.index(linenum)

    def process_c_files(self, folder_path):
        files = os.listdir(folder_path)
        for file_name in files:
            if file_name.endswith('.c'):
                file_path = os.path.join(folder_path, file_name)
                file_base_name = os.path.splitext(file_name)[0]
                if os.path.getsize(file_path) > 0:
                    with open(file_path, 'a') as f:
                        f.write('char end_' + file_base_name + ' __attribute__((aligned(64))) __attribute__((section("venus_' + file_base_name + '"))) = {0};')


    def add_data_type(self,data_id, data_type):
        self.data_table[data_id] = data_type
    # Run it
    def run(self):
        self.vars = {}            # All variables
        self.lists = {}            # List variables
        self.tables = {}            # Tables
        self.loops = []            # Currently active loops
        self.loopend = {}            # Mapping saying where loops end
        self.gosub = None           # Gosub return point (if any)
        self.error = 0              # Indicates program error

        self.stat = list(self.prog)  # Ordered list of all line numbers
        self.stat.sort()
        self.pc = 0                  # Current program counter

        # Processing prior to running

        self.collect_data()          # Collect all of the data statements
        self.check_end()
        self.check_loops()

        
        if self.error:
            raise RuntimeError
        # output=""
        loaded_mapping_global = {}
        loaded_mapping_para = {}
        loaded_mapping_dag_input = {}
        loaded_mapping_return = {} 
        loaded_mapping_dfe = {} 
        loaded_mapping_task = {}       
        loaded_mapping = {}      
        task_dict = {}  
        task_dict_dest_addr = {}
        variable_dict = {}
        variable_dict_without_return_value = {}
        dag_info = {}
        all_dag_info ={}
        
        concat_count = 0 
        data_concat = {}  # Concat input and output
        which_concat = {}

        while 1:
            line = self.stat[self.pc]
            instr = self.prog[line] 
            op = instr[0]

            # END and STOP statements
            if op == 'END' or op == 'STOP':
                used_variables = set()
                for dag_name, dag_data in dag_info.items():
                    for task_id, task_info in dag_data["task_dict"].items():
                        used_variables.update(task_info.get('input', []))
                        used_variables.update(task_info.get('output', []))
                unused_variables = set(variable_dict.keys()) - used_variables
                unused_variables_without_return_value = set(variable_dict_without_return_value.keys()) - used_variables

                for var_name in unused_variables:
                    del variable_dict[var_name]

                for var_name in unused_variables_without_return_value:
                    del variable_dict_without_return_value[var_name]

                for dag_name, dag_data in dag_info.items():
                    task_dict = dag_data["task_dict"]
                    task_dict_dest_addr = dag_data["task_dict_dest_addr"]
                    which_concat= dag_data["which_concat"]
                    data_concat= dag_data["data_concat"]
                    dag_folder = f"./IJ/{dag_name}"
                    os.makedirs(dag_folder, exist_ok=True)
                    with open(f"{dag_folder}/task_input_output.json", 'w') as json_file:
                        json.dump(task_dict_dest_addr, json_file, indent=4)
                    tasks = []
                    # Iterate through each task
                    for task_id, task_info in task_dict.items():
                        actual_task_name = task_id.split('*')[0] if '*' in task_id else task_id
                        if (actual_task_name == "concat" or actual_task_name == "slice"):
                            continue

                        output_num = task_info.get("output", [])

                        task = {
                            "taskId": task_id,
                            "computationCost": 0.0,  # Assuming computation cost is 0 for simplicity
                            "spm_size": 0,  # Assuming SPM size is 0 for simplicity
                            "num_lane": 0,  # Assuming num_lane is 0 for simplicity
                            "has_bitalu": False,  # Assuming no bit ALU for simplicity
                            "has_serdiv": False,  # Assuming no serial divider for simplicity
                            "has_complexunit": False,  # Assuming no complex unit for simplicity
                            "parentTasks": [],
                            "childTasks": [],
                            "global_Input": [],
                            "para_Input": [],
                            "return_output": [],
                            "output_num": len(output_num),
                            "is_spmd": task_info.get("is_spmd", 0),             # bas不写则默认为0
                            "min_core_num": task_info.get("min_core_num", 1),   # bas不写则默认为1
                        }
                        with open('venus_test/scalar_o.json', 'r') as f:
                            vreturn_data = json.load(f)
                        # ParentTasks with var name and index. This index is ParentTask's var index
                        for other_task_id, other_task_info in task_dict.items():
                            if 'output' in other_task_info:
                                for output_var, output_index in other_task_info['output'].items():
                                    if output_var in task_info.get('input', {}):
                                        # Find the index of the output variable in the parent task's output
                                        parent_task_output_index = other_task_info['output'][output_var]
                                         # Check if the parent task is "concat"
                                        actual_task_name = other_task_id.split('*')[0] if '*' in other_task_id else other_task_id
                                        if actual_task_name == "concat" and other_task_id in which_concat[task_id]:
                                            print(f'actual_task_name:{actual_task_name}')
                                            # Replace with concat's all parent inputs
                                            concat_parent_inputs = []
                                            num_params = sum(isinstance(item, str) for item in list(data_concat[other_task_id].values())[0])
                                            for index in range(num_params):
                                                # Get the value of the parameter
                                                concat_input_value = list(data_concat[other_task_id].values())[0]
                                                concat_input_value_onlystr = [item for item in list(data_concat[other_task_id].values())[0] if isinstance(item, str)]
                                                concat_input_value_onlystr_index = concat_input_value_onlystr[index]
                                                # Check if the parameter value is in the output of this task
                                                for parent_task_id, parent_task_info in task_dict.items():
                                                    if 'output' in parent_task_info:
                                                        for parent_output_var, parent_output_index in parent_task_info['output'].items():
                                                            actual_parent_task_id = parent_task_id.split('*')[0] if '*' in parent_task_id else parent_task_id
                                                            print(f'actual_parent_task_id:{actual_parent_task_id}')
                                                            if actual_parent_task_id != "concat" :
                                                                concat_output = list(data_concat[other_task_id].keys())[0]
                                                                concat_length = 0
                                                                if concat_input_value_onlystr_index == parent_output_var :
                                                                    print(f"concat_input_value_onlystr_index:{concat_input_value_onlystr_index}")
                                                                    for item in vreturn_data:
                                                                        if item['function'] == actual_parent_task_id:
                                                                            vreturn_o = item.get('vreturn_o', [])
                                                                            # 跳过字符串（指针参数），只取 dict 输出端口条目
                                                                            dict_entries = [v for v in vreturn_o if isinstance(v, dict)]
                                                                            print(f"parent_output_index '{parent_output_index}'")
                                                                            if 0 <= int(parent_output_index) < len(dict_entries):
                                                                                vec = dict_entries[int(parent_output_index)]
                                                                                vec_size = list(vec.values())[0]

                                                                                print(f"vreturn_o '{vreturn_o}'")
                                                                                print(f"vec '{vec}'")
                                                                                print(f"vec_size '{vec_size}'")
                                                                                concat_length = vec_size

                                                                            else:
                                                                                raise ValueError(f"concat value need fixed length, vector {concat_input_value_onlystr_index} not found in scalar_o.json")
  
                                                                    concat_parent_inputs.append({
                                                                        "taskId": parent_task_id,
                                                                        "outputVar": parent_output_var,
                                                                        "outputIndex": parent_output_index,
                                                                        "dest_address": concat_output, # Choose the concat output as dest_address to match
                                                                        "concat_value": max([value for value in concat_input_value if isinstance(value, int)]),
                                                                        "concat_length": concat_length
                                                                    })
                                                                    task["parentTasks"].extend(concat_parent_inputs)
                                                                    concat_parent_inputs = []

                                        elif actual_task_name != "concat":
                                            consumer_input = task_info.get('input', {})
                                            matching_input_keys = sorted(
                                                [k for k in consumer_input if k.split('*')[0] == output_var],
                                                key=lambda k: consumer_input[k]
                                            )
                                            for input_key in matching_input_keys:
                                                is_pointer_flag = False
                                                if task_id in task_dict_dest_addr and 'input' in task_dict_dest_addr[task_id]:
                                                    if input_key in task_dict_dest_addr[task_id]['input']:
                                                        is_pointer_flag = task_dict_dest_addr[task_id]['input'][input_key].get('is_pointer', False)

                                                pt = {
                                                    "taskId": other_task_id,
                                                    "outputVar": output_var,
                                                    "outputIndex": parent_task_output_index,
                                                    "dest_address": "null",
                                                    "concat_value": 0,
                                                    "concat_length": 0,
                                                    "is_pointer": is_pointer_flag,
                                                    "consumerInputVar": input_key,
                                                }
                                                if is_pointer_flag:
                                                    pt["type"] = TYPE_PTR_TEMP
                                                task["parentTasks"].append(pt)
                            Found = False
                            if 'input' in other_task_info:
                                # Iterate over the input variables and their corresponding indices of the current task
                                for input_var, input_index in other_task_info['input'].items():
                                    # Check if the current input variable is present in the output of this task
                                    if input_var in task_info.get('output', {}):
                                        # Get the index of the current input variable in this task
                                        child_task_output_index = other_task_info['input'][input_var]
                                        # Get the actual task name, if it contains '*', use the part before '*' as the actual name
                                        actual_task_name = other_task_id.split('*')[0] if '*' in other_task_id else other_task_id
                                        # If the actual task name is "concat"
                                        if actual_task_name == "concat":
                                            # Store all child inputs of concat
                                            concat_child_inputs = []  
                                            # Loop through all tasks to find the child tasks of concat
                                            for child_task_id, child_task_info in task_dict.items():
                                                # Check if the child task has inputs
                                                if 'input' in child_task_info:
                                                    # Iterate over the input variables and their corresponding indices of the child task
                                                    for child_input_var, child_input_index in child_task_info['input'].items(): # Concat's input
                                                        # Get the actual child task name, if it contains '*', use the part before '*' as the actual name
                                                        actual_child_task_id = child_task_id.split('*')[0] if '*' in child_task_id else child_task_id
                                                        # Check if the actual name of the child task is not "concat" and the input variable of the child task is in the output of the concat task
                                                        
                                                        if actual_child_task_id != "slice" and actual_child_task_id != "concat" and other_task_id in which_concat[child_task_id] and Found == False and child_input_var in other_task_info['output'] :
                                                            # Get the number of parameters for the current child task input variable
                                                            num_params = sum(isinstance(item, str) for item in data_concat[other_task_id][child_input_var])

                                                            # Loop through the parameters
                                                            Found = True
                                                            
                                                            for index in range(num_params):
                                                                
                                                                # Get the value of the parameter
                                                                concat_input_value = data_concat[other_task_id][child_input_var]
                                                                concat_input_value_onlystr = [item for item in data_concat[other_task_id][child_input_var] if isinstance(item, str)]
                                                                concat_input_value_onlystr_index = concat_input_value_onlystr[index]
                                                                # Check if the parameter value is in the output of this task
                                                                if concat_input_value_onlystr_index in task_info.get('output', {}):
                                                                    # for item in vreturn_data:
                                                                    #     if item['function'] == actual_parent_task_id:
                                                                    #         vreturn_o = item.get('vreturn_o', [])
                                                                    #         vec = vreturn_o[int(parent_output_index)]
                                                                    #         vec_size = list(vec.values())[0]
                                                                    #         concat_length = vec_size
                                                                    # Append the child task
                                                                    concat_child_inputs.append({
                                                                        "taskId": child_task_id,
                                                                        "inputVar": concat_input_value_onlystr_index,
                                                                        "inputIndex": child_task_output_index,
                                                                        "dest_address": child_input_var,
                                                                        "concat_value": max([value for value in concat_input_value if isinstance(value, int)]),
                                                                        "concat_length": 0
                                                                    })
                                                            task["childTasks"].extend(concat_child_inputs)
                                        else:
                                            # If it's not a concat task, append the child task to the list of child tasks of the current task
                                            task["childTasks"].append({
                                                "taskId": other_task_id,
                                                "inputVar": input_var,
                                                "inputIndex": child_task_output_index, 
                                                "dest_address": "null",
                                                "concat_value": 0,
                                                "concat_length": 0
                                            })

                        for input_var in task_info['input']:
                            input_var_original = input_var
                            input_var = input_var.split('*', 1)[0]

                            # 获取指针标记
                            is_pointer = False
                            if task_id in task_dict_dest_addr and input_var_original in task_dict_dest_addr[task_id]['input']:
                                is_pointer = task_dict_dest_addr[task_id]['input'][input_var_original].get('is_pointer', False)

                            # 步骤10：pointer 类型：temp 取址在 input_dest_addr；此处区分 param/global 与 dag/dfe 指针
                            if input_var in variable_dict_without_return_value:
                                input_index = list(variable_dict_without_return_value.keys()).index(input_var) + 1
                                input_type = variable_dict_without_return_value[input_var]['type']
                                input_num = variable_dict_without_return_value[input_var]['num']

                                if is_pointer:
                                    vsrc = variable_dict_without_return_value[input_var]['var']
                                    if vsrc in ('global', 'parameter'):
                                        input_type = TYPE_PTR_PARAM_GLOBAL
                                    elif vsrc in ('dfedata', 'dag_input'):
                                        input_type = TYPE_PTR_DAG_DFE

                                # 根据变量来源分类
                                if variable_dict_without_return_value[input_var]['var'] == 'global':
                                    task['global_Input'].append({"name": input_var, "index": input_index, "type": input_type, "dest_address": "null", "input_num":input_num, "is_pointer": is_pointer})
                                elif variable_dict_without_return_value[input_var]['var'] == 'parameter':
                                    task['para_Input'].append({"name": input_var, "index": input_index, "type": input_type, "dest_address": "null", "input_num":input_num, "is_pointer": is_pointer})
                                elif variable_dict_without_return_value[input_var]['var'] == 'dfedata':
                                    task['para_Input'].append({"name": input_var, "index": input_index, "type": input_type, "dest_address": "null", "input_num":input_num, "is_pointer": is_pointer})
                                elif variable_dict_without_return_value[input_var]['var'] == 'dag_input':
                                    task['para_Input'].append({"name": input_var, "index": input_index, "type": input_type, "dest_address": "null", "input_num":input_num, "is_pointer": is_pointer})
                            elif input_var not in variable_dict_without_return_value :
                                is_in_task_dict_output = False
                                for task_info_value in task_dict.values():
                                    if input_var in task_info_value['output']:
                                        is_in_task_dict_output = True
                                        break
                                if is_in_task_dict_output == False:
                                    raise ValueError(f"Input variable '{input_var}' is not defined as an output or a valid input.")

                        for output_index, output_var in enumerate(task_info['output']):
                            if output_var in variable_dict:
                                if variable_dict[output_var]['type'] == TYPE_RETURN_VALUE:
                                    output_type = variable_dict[output_var]['type']
                                    output_length = variable_dict[output_var]['length']
                                    task['return_output'].append({"name": output_var, "index": output_index, "type": output_type, "length": output_length})       
                                    filename = "./variable/map/return_value.json"
                                    loaded_mapping_return = {}
                                    try:
                                        with open(filename, "r") as json_file:
                                            loaded_mapping_return = json.load(json_file)
                                    except FileNotFoundError:
                                        pass
                                    except json.decoder.JSONDecodeError:
                                        pass
                                    
                                    loaded_mapping_return[output_var] = {"taskId": task_id, "index": output_index, "type": output_type, "length": output_length}
                                    with open(filename, "w") as json_file:
                                        json.dump(loaded_mapping_return, json_file, indent=4)        

                        tasks.append(task)

                    with open(f"{dag_folder}/input_tasks.json", "w") as json_file:
                        json.dump(tasks, json_file, indent=4)

                    # -------------------------------------------------------
                    # 阶段一步骤1：产出临时变量生命周期表
                    # 临时变量 = task 输出变量中，未声明为 return_value 的变量。
                    # -------------------------------------------------------
                    self._generate_temp_variables(
                        dag_name, dag_folder, task_dict, variable_dict,
                        pinned_vars=dag_data.get("pinned_vars", []),
                    )

                for var in variable_dict_without_return_value:
                    if variable_dict_without_return_value[var]['var'] == 'global':
                        datatype = variable_dict_without_return_value[var]['datatype']
                        value = variable_dict_without_return_value[var]['value']
                        length = variable_dict_without_return_value[var]['length']
                        input_index = list(variable_dict_without_return_value.keys()).index(var) + 1
                        c_code = f'{datatype} {var} __attribute__((aligned(64))) __attribute__((section("venus_global"))) = {value};\n'
                        with open('./variable/global.c', 'a') as file:
                            file.write(c_code)
                        filename = "./variable/map/global.json"
                        filename_all = "./variable/map/all_data.json"
                        loaded_mapping_global[var] = {"type": TYPE_PARAM_GLOBAL, "index": input_index,"length":length}
                        loaded_mapping[var] = {"type": TYPE_PARAM_GLOBAL, "index": input_index,"length":length}
                        with open(filename, "w") as json_file:
                            json.dump(loaded_mapping_global, json_file, indent=4)          

                    elif variable_dict_without_return_value[var]['var'] == 'parameter':
                        datatype = variable_dict_without_return_value[var]['datatype']
                        value = variable_dict_without_return_value[var]['value']
                        length = variable_dict_without_return_value[var]['length']
                        input_index = list(variable_dict_without_return_value.keys()).index(var) + 1
                        c_code = f'''{datatype} {var}[] __attribute__((aligned(64))) __attribute__((section("venus_parameter"))) = {{{", ".join(str(val) for val in value)}}};\n'''
                        with open('./variable/parameter.c', 'a') as file:
                            file.write(c_code)
                        filename = "./variable/map/parameter.json"
                        filename_all = "./variable/map/all_data.json"
                        loaded_mapping_para[var] = {"type": TYPE_PARAM_GLOBAL, "index": input_index,"length":length,"datatype": datatype}
                        loaded_mapping[var] = {"type": TYPE_PARAM_GLOBAL, "index": input_index,"length":length,"datatype": datatype}
                        with open(filename, "w") as json_file:
                            json.dump(loaded_mapping_para, json_file, indent=4)
                        with open(filename_all, "w") as json_file_all:
                            json.dump(loaded_mapping, json_file_all,  indent=4)

                    elif variable_dict_without_return_value[var]['var'] == 'dfedata':
                        datatype = variable_dict_without_return_value[var]['datatype']
                        element_count = variable_dict_without_return_value[var]['num']
                        length = variable_dict_without_return_value[var]['length']
                        input_index = list(variable_dict_without_return_value.keys()).index(var) + 1
                        c_code = f'''{datatype} {var}[{element_count}] __attribute__((aligned(64))) __attribute__((section("venus_dfedata"))) = {{}};\n'''
                        with open('./variable/dfedata.c', 'a') as file:
                            file.write(c_code)
                        filename = "./variable/map/dfedata.json"
                        filename_all = "./variable/map/all_data.json"
                        loaded_mapping_dfe[var] = {"type": TYPE_DAG_DFE_STATIC, "index": input_index,"length":length,"datatype": datatype}
                        loaded_mapping[var] = {"type": TYPE_DAG_DFE_STATIC, "index": input_index,"length":length,"datatype": datatype}
                        with open(filename, "w") as json_file:
                            json.dump(loaded_mapping_dfe, json_file, indent=4)
                        with open(filename_all, "w") as json_file_all:
                            json.dump(loaded_mapping, json_file_all,  indent=4)
                    
                    elif variable_dict_without_return_value[var]['var'] == 'dag_input':
                        datatype = variable_dict_without_return_value[var]['datatype']
                        element_count = variable_dict_without_return_value[var]['num']
                        length = variable_dict_without_return_value[var]['length']
                        input_index = list(variable_dict_without_return_value.keys()).index(var) + 1
                        value = [0xffff] * int(element_count)
                        # c_code = f'''{datatype} {var}[{element_count}] __attribute__((aligned(64))) __attribute__((section("venus_dag_input"))) = {{{", ".join(hex(val) for val in value)}}};\n'''
                        c_code = f'''{datatype} {var}[{element_count}] __attribute__((aligned(64))) __attribute__((section("venus_dag_input"))) = {{}};\n'''
                        with open('./variable/dag_input.c', 'a') as file:
                            file.write(c_code)
                        filename = "./variable/map/dag_input.json"
                        filename_all = "./variable/map/all_data.json"
                        # loaded_mapping_dag_input[var] = {"type": TYPE_DAG_DFE_STATIC, "index": input_index,"length":length,"datatype": datatype}
                        # loaded_mapping[var] = {"type": TYPE_DAG_DFE_STATIC, "index": input_index,"length":length,"datatype": datatype}
                        loaded_mapping_dag_input[var] = {"type": TYPE_DAG_DFE_STATIC, "index": input_index,"length": length,"datatype": datatype}
                        loaded_mapping[var] = {"type": TYPE_DAG_DFE_STATIC, "index": input_index,"length": length,"datatype": datatype}
                        with open(filename, "w") as json_file:
                            json.dump(loaded_mapping_dag_input, json_file, indent=4)
                        with open(filename_all, "w") as json_file_all:
                            json.dump(loaded_mapping, json_file_all,  indent=4)

                folder_path = "./variable"
                self.process_c_files(folder_path)          
                break           # We're done



            # GOTO statement
            elif op == 'GOTO':
                newline = instr[1]
                self.goto(newline)
                continue

            # PRINT statement
            elif op == 'PRINT':
                plist = instr[1]
                out = ""
                for label, val in plist:
                    if out:
                        out += ' ' * (15 - (len(out) % 15))
                    out += label
                    if val:
                        if label:
                            out += " "
                        eval = self.eval(val)
                        out += str(eval)
                sys.stdout.write(out)
                end = instr[2]
                if not (end == ',' or end == ';'):
                    sys.stdout.write("\n")
                if end == ',':
                    sys.stdout.write(" " * (15 - (len(out) % 15)))
                if end == ';':
                    sys.stdout.write(" " * (3 - (len(out) % 3)))

            # LET statement
            elif op == 'LET':
                target = instr[1]
                value = instr[2]
                self.assign(target, value)

            elif op == 'GLOBAL':
                global_id = instr[1][1]
                global_type = instr[1][0]
                original_binary_value = 0
                if(instr[2] != "NULL"):
                    original_binary_value = instr[2]
                if global_id in variable_dict:
                    raise ValueError(f"Variable '{global_id}' already exists in variable_dict!")

                global_length = byte_length(global_type, 1)
                variable_dict[str(global_id)] = {"var":"global","type": TYPE_PARAM_GLOBAL,"datatype":global_type,"value":original_binary_value,"num":1,"length":global_length}
                variable_dict_without_return_value[str(global_id)] = {"var":"global","type": TYPE_PARAM_GLOBAL,"datatype":global_type,"value":original_binary_value,"num":1,"length":global_length}
                

            elif op == 'PARAMETER' :
                parameter_id = instr[1][1]
                parameter_type = instr[1][0]
                original_array_value = instr[2]

                if parameter_id in variable_dict:
                    raise ValueError(f"Variable '{parameter_id}' already exists in variable_dict!")

                parameter_num = len(original_array_value)
                parameter_length = byte_length(parameter_type, parameter_num)
                variable_dict[str(parameter_id)] = {"var":"parameter","type": TYPE_PARAM_GLOBAL,"datatype":parameter_type,"value":original_array_value,"num":parameter_num,"length":parameter_length}
                variable_dict_without_return_value[str(parameter_id)] = {"var":"parameter","type": TYPE_PARAM_GLOBAL,"datatype":parameter_type,"value":original_array_value,"num":parameter_num,"length":parameter_length}
                
            elif op == 'DFEDATA' :
                dfedata_id = instr[1][1]
                dfedata_type = instr[1][0]
                dfedata_length = instr[2]

                if dfedata_id in variable_dict:
                    raise ValueError(f"Variable '{dfedata_id}' already exists in variable_dict!")

                dfedata_byte_length = byte_length(dfedata_type, dfedata_length)
                variable_dict[str(dfedata_id)] = {"var":"dfedata","type": TYPE_DAG_DFE_STATIC,"datatype":dfedata_type,"num":dfedata_length,"length":dfedata_byte_length}
                variable_dict_without_return_value[str(dfedata_id)] = {"var":"dfedata","type": TYPE_DAG_DFE_STATIC,"datatype":dfedata_type,"num":dfedata_length,"length":dfedata_byte_length}
            
            elif op == 'DAG_INPUT' :
                dag_input_id = instr[1][1]
                dag_input_type = instr[1][0]
                dag_input_length = instr[2]

                if dag_input_id in variable_dict:
                    raise ValueError(f"Variable '{dag_input_id}' already exists in variable_dict!")

                dag_input_byte_length = byte_length(dag_input_type, dag_input_length)
                variable_dict[str(dag_input_id)] = {"var":"dag_input","type": TYPE_DAG_DFE_STATIC,"datatype":dag_input_type,"num":dag_input_length,"length":dag_input_byte_length}
                variable_dict_without_return_value[str(dag_input_id)] = {"var":"dag_input","type": TYPE_DAG_DFE_STATIC,"datatype":dag_input_type,"num":dag_input_length,"length":dag_input_byte_length}
                  
            elif op == 'RETURN_VALUE':
                return_value_id = instr[1][1]
                return_value_type = instr[1][0]
                return_value_length = instr[2]
                return_index = 0
                return_value_byte = 0

                return_value_byte = byte_length(return_value_type, return_value_length)

                if return_value_id in loaded_mapping:
                    raise ValueError(f"Variable '{return_value_id}' already exists in loaded_mapping!")
                variable_dict[str(return_value_id)] = {"var":"return_value","index": return_index, "datatype":return_value_type ,"type": TYPE_RETURN_VALUE, "length": return_value_byte,"num":return_value_length}
                
            elif op == 'DAG':
                input_num = 0 
                input_vars = {}
                input_vars_dest_addr = {}
                output_vars = {}
                
                dag_object = instr[1]
                dag_name = dag_object['id']

                schedule_interval = dag_object.get('schedule_interval')
                start_date = dag_object.get('start_date')
                end_date = dag_object.get('end_date')

                tasks = dag_object['tasks']
                task_dict = {}
                task_dict_dest_addr = {}
                task_counts = {}
                data_concat = {}
                which_concat = {}
                slice_map = {}
                spmd_stack = []     # 用于跟踪SPMD作用域
                pinned_vars = []    # #pragma temp pin 声明顺序（去重保序）
                pinned_seen = set()
                with open("./venus_test/task_input_num.json", "r") as file:
                    data = json.load(file)

                with open("./venus_test/scalar_o.json", "r") as file:
                    task_scalar_data = json.load(file)

                for task in tasks:
                    task_id = task['name']

                    # SPMD 指令处理
                    # spmd_stack用列表模拟栈：遇到SPMD_BEGIN压入核心数，遇到SPMD_END弹出
                    if task_id == 'SPMD_BEGIN':
                        spmd_stack.append(task.get('min_core_num', 1))
                        continue
                    if task_id == 'SPMD_END':
                        if spmd_stack:
                            spmd_stack.pop()
                        continue
                    # TEMP_PIN：只收集变量名，不进 task_dict，不碰 spmd_stack
                    if task_id == 'TEMP_PIN':
                        for vname in task.get('vars', []):
                            if vname not in pinned_seen:
                                pinned_seen.add(vname)
                                pinned_vars.append(vname)
                        continue

                    # 当前 task 的 SPMD 属性
                    # 栈空时is_spmd=0, min_core_num=1，不写pragma的task自然就是非SPMD模式
                    if spmd_stack:
                        task_is_spmd = 1
                        task_min_core_num = spmd_stack[-1]
                    else:
                        task_is_spmd = 0
                        task_min_core_num = 1
                    
                    # 输入输出数检查
                    input_count = len(task.get("input", []))
                    output_count = len(task.get("output", []))
                    if input_count > 63:
                        raise ValueError(f"Task {task_id} has too many inputs: {input_count}")
                    if output_count > 16:
                        raise ValueError(f"Task {task_id} has too many outputs: {output_count}")
                                         
                    if task_id == "variable":
                        raise ValueError(f"Task Id couldn't be called {task_id}.")

                    if '*' in task_id:
                        raise ValueError(f"Task Id {task_id} contains invalid character '*'.")

                    # SPMD 与 slice/concat 互斥检查
                    if task_is_spmd == 1 and (task_id == "concat" or task_id == "slice"):
                        raise ValueError(f"Task '{task_id}' cannot be used with SPMD (slice/concat are not SPMD-compatible).")

                    if task_id != "concat" and task_id != "slice":
                        expected_input_num = data[task_id]
                        
                    # Check if there are duplicate task IDs, if so, append '*' and the number of repetitions after the task ID.
                    if task_id in task_counts:
                        task_counts[task_id] += 1
                        task_id = f"{task_id}*{task_counts[task_id]}"
                    else:
                        task_counts[task_id] = 0

                    actual_task_name = task_id.split('*')[0] if '*' in task_id else task_id
                    if actual_task_name == "concat":      
                        if isinstance(task['input'], list) and len(task['input']) == 1:
                            raise ValueError(f"The input to the concat function must be greater than {len(task['input'])}.")

                        if task['output'] is not None:
                            for member in task['output']:
                                out_name = member[0] if isinstance(member, tuple) else member
                                if out_name in variable_dict and variable_dict[out_name].get('type') == TYPE_RETURN_VALUE:
                                    raise ValueError(
                                        f"Illegal usage: concat output variable '{out_name}' cannot be declared as return_value. "
                                        f"Bind return_value to a normal task output instead of concat."
                                    )
                        
                        concat_count = concat_count + 1
                        if task['input'] is not None:
                            for member in task['input']:
                                if isinstance(member, tuple):
                                    var_name = member[0]
                                else:
                                    var_name = member
                                if task_id not in data_concat:
                                        data_concat[task_id] = {task['output'][0][0]: []}  # Create an empty list if the key doesn't exist
                                found_var = False
                                index = False
                                for task_concat, inner_dict in reversed(data_concat.items()):
                                    if found_var:
                                        break
                                    if index:
                                        for var, value_list in inner_dict.items():
                                            if var_name == var:
                                                found_var = True 
                                                for value in value_list:
                                                    data_concat[task_id][task['output'][0][0]].append(value)
                                                break
                                    index = True
                                if not found_var:
                                    data_concat[task_id][task['output'][0][0]].append(var_name)  
                                data_concat[task_id][task['output'][0][0]].append(concat_count)                         

                        if task['input'] is not None:
                            for member in task['input']:
                                if isinstance(member, tuple):
                                    var_name = member[0]
                                else:
                                    var_name = member

                    elif actual_task_name != "slice": 
                         which_concat[task_id] = []
                         if task['input'] is not None:
                            for member in task['input']:
                                if isinstance(member, tuple):
                                    var_name = member[0]
                                else:
                                    var_name = member
                                found_var = False    
                                index = True
                                for task_concat, inner_dict in reversed(data_concat.items()):
                                    if index:
                                        for var, value_list in inner_dict.items():
                                            if var == var_name:        
                                                which_concat[task_id].append(task_concat)
                                                index = False
                                                found_var = True
                                if not found_var:
                                    which_concat[task_id].append(1)        

                    input_num = 0 
                    input_num2 = 0 
                    input_vars = {}
                    input_vars_dest_addr = {}
                    var_count = {}

                    if actual_task_name != "slice" and task['input'] is not None:
                        for member in task['input']:
                            input_num2= input_num2 + 1

                        if actual_task_name != "concat" and input_num2 != expected_input_num:
                            raise ValueError(f"Input2 number for task '{task_id}' is {input_num2}, expected {expected_input_num}")
                        
                        for member in task['input']:
                            input_num= input_num + 1
                            index = input_num -1
                            is_pointer = False
                            if isinstance(member, tuple):
                                if member[0] == 'POINTER':      # 识别指针标记
                                    is_pointer = True
                                    var_name = member[1]        # 取实际变量名（去掉&）

                                    # 阶段一步骤4：野指针前置报错检查
                                    # 检查被取地址的变量是否已定义
                                    var_defined = False
                                    # 检查是否在 variable_dict 中（global/parameter/dfedata/dag_input/return_value）
                                    if var_name in variable_dict:
                                        var_defined = True
                                    # 检查是否在之前的 task 输出中（temp 变量）
                                    if not var_defined:
                                        for prev_task_info in task_dict.values():
                                            if 'output' in prev_task_info and var_name in prev_task_info['output']:
                                                var_defined = True
                                                break

                                    if not var_defined:
                                        raise ValueError(
                                            f"野指针错误：在 task '{task_id}' 中尝试对未定义的变量 '{var_name}' 取地址（&{var_name}）。"
                                            f"变量必须先定义（声明为 global/parameter/dfedata/dag_input/return_value 或作为前序 task 的输出）才能取地址。"
                                        )
                                else:
                                    var_name = member[0]
                            else:
                                var_name = member
                            # var count
                            if var_name in var_count:
                                var_count[var_name] += 1
                                var_name_with_count = f"{var_name}*{var_count[var_name]}"
                            else:
                                var_count[var_name] = 0
                                var_name_with_count = var_name
                            input_vars[var_name_with_count] = index

                            scalar_o_set = set()  

                            for item in task_scalar_data:
                                scalar_o_list = item.get("scalar_o", [])
                                scalar_o_set.update(scalar_o_list)

                            # 首先检查是否在 variable_dict 中（parameter/global/dfedata/dag_input）
                            if (str(var_name_with_count) in variable_dict ) :
                                input_len = variable_dict[str(var_name_with_count)]['num']
                                print(f"DEBUG: {var_name_with_count} found in variable_dict, num={input_len}")
                            else:
                                # temp 变量作为输入时，优先从生产者输出端口解析真实长度
                                base_var_name = var_name.split('*')[0] if '*' in var_name else var_name
                                print(f"DEBUG: Checking if {base_var_name} is temp var, task_dict keys: {list(task_dict.keys())}")
                                input_len = 0
                                producer_task_id_for_var = None
                                producer_output_port_for_var = 0
                                for prev_task_id, prev_task_info in task_dict.items():
                                    if 'output' not in prev_task_info:
                                        continue
                                    # 1) 精确同名匹配（历史行为）
                                    if base_var_name in prev_task_info['output']:
                                        producer_task_id_for_var = prev_task_id
                                        producer_output_port_for_var = int(prev_task_info['output'][base_var_name])
                                        break
                                    # 2) 别名匹配：允许 consumer 使用 rxData_real，
                                    #    producer 输出为 rxData_real_0 / rxData_real_1 ... 的情况。
                                    #    选择最小后缀（通常 *_0），保持确定性。
                                    candidates = []
                                    for out_name, out_port in prev_task_info['output'].items():
                                        if re.fullmatch(re.escape(base_var_name) + r'_\d+', out_name):
                                            try:
                                                suffix = int(out_name.rsplit('_', 1)[1])
                                            except Exception:
                                                continue
                                            candidates.append((suffix, out_name, out_port))
                                    if candidates:
                                        candidates.sort(key=lambda x: x[0])
                                        _suffix, resolved_name, resolved_port = candidates[0]
                                        producer_task_id_for_var = prev_task_id
                                        producer_output_port_for_var = int(resolved_port)
                                        # 用 resolved_name 去 scalar_o.json 取值（字典 key 可能是 resolved_name）
                                        base_var_name = resolved_name
                                        break

                                if producer_task_id_for_var is not None:
                                    actual_task_name_producer = producer_task_id_for_var.split('*')[0] if '*' in producer_task_id_for_var else producer_task_id_for_var
                                    for item in task_scalar_data:
                                        if item.get("function") == actual_task_name_producer:
                                            vreturn_o = item.get("vreturn_o", [])
                                            dict_entries = [v for v in vreturn_o if isinstance(v, dict)]
                                            if dict_entries:
                                                port = producer_output_port_for_var if producer_output_port_for_var < len(dict_entries) else 0
                                                vret = dict_entries[port]
                                                if "output" in vret:
                                                    input_len = vret["output"]
                                                elif base_var_name in vret:
                                                    input_len = vret[base_var_name]
                                                elif len(vret) == 1:
                                                    input_len = list(vret.values())[0]
                                                if input_len:
                                                    print(f"DEBUG: Found {base_var_name} output length in scalar_o.json (port {port}): {input_len}")
                                            break

                                # 非 temp 输入再按 scalar_o_set 走标量语义
                                if input_len == 0 and (str(var_name_with_count) in scalar_o_set):
                                    input_len = 1
                                    print(f"DEBUG: {var_name_with_count} found in scalar_o_set, num=1")

                                if input_len == 0:
                                    input_len = 1
                                    print(f"DEBUG: {var_name_with_count} not found anywhere, defaulting num=1")
                            # 阶段一步骤2：保留指针语义；parentTasks 侧已在上面写入 TYPE_PTR_TEMP；静态量指针在 input_dest_addr 中细分为 0b101/0b110
                            input_vars_dest_addr[str(var_name_with_count)] = {"index": index, "dest_address": "null", "num": input_len, "is_pointer": is_pointer}
                            
                            
                            with open("./venus_test/input_type.json", 'r', encoding='utf-8') as f:
                                input_type_data = json.load(f)
                            task_input_type_data = input_type_data.get(actual_task_name)
                            print(f"type verify: {actual_task_name}")
                            print(f"! var_name_with_count: {var_name_with_count}")
                            # print(f"variable_dict:{variable_dict}")
                            if task_input_type_data is not None:
                                args = task_input_type_data.get("args", [])
                                arg = args[index]
                                name = arg.get("name")
                                arg_type = arg.get("type")
                                # print(f" - task_input_type_data: {task_input_type_data}")
                                print(f" - name: {name}")
                                print(f" - arg_type: {arg_type}")
                                print(f" - var_name_with_count: {var_name_with_count}")
                                if (arg_type == "short" or arg_type == "int" or arg_type == "char") and var_name_with_count in variable_dict:
                                    if variable_dict[str(var_name_with_count)]['datatype'] != arg_type:
                                        raise ValueError(f"{actual_task_name}: Type mismatch for bas: {str(var_name_with_count)} c: {name}: expected {arg_type}, found {(variable_dict[str(var_name_with_count)]['datatype'])}")
                                elif (arg_type == "short" or arg_type == "int" or arg_type == "char") and var_name_with_count not in variable_dict:
                                    # temp 变量作为标量结构体输入，保持 num=1，不要改成 0
                                    # input_vars_dest_addr[str(var_name_with_count)]["num"] = 0
                                    pass
                                else:
                                    # 非 short/int/char 参数不应在此处强制覆盖 num=1。
                                    # temp 依赖输入需要保留前面推导出的真实长度语义。
                                    pass
                                # print(f"input_vars_dest_addr:{input_vars_dest_addr[str(var_name_with_count)]}")
                            else:
                                # print(f"input_type_data:{input_type_data}")
                                print(f"actual_task_name:{actual_task_name}")
                                print(f" - var_name_with_count: {var_name_with_count}")
                                # print(f"task_input_type_data:{task_input_type_data}")
                                args = []        
     
                    output_vars = {}
                    if actual_task_name != "slice" and task['output'] is not None:
                        for member in task['output']:
                            if isinstance(member, tuple):
                                var_name = member[0]
                            else:
                                var_name = member
                            output_vars[var_name] = task['output'].index(member)
  
                    if actual_task_name == "slice": 
                        if task['input'] is not None:
                            first_input_member = task['input'][0]
                            slice_length_member = task['input'][1]
                            slice_data_type_member = task['input'][2]
                            if isinstance(first_input_member, tuple):
                                first_input_name = first_input_member[0]
                                slice_length = slice_length_member[0]
                                slice_data_type = slice_data_type_member[0]
                            else:
                                first_input_name = first_input_member
                                slice_length = slice_length_member
                                slice_data_type = slice_data_type_member

                            input_vars[first_input_name] = 0
                            input_vars_dest_addr[str(first_input_name)] = {"index": 0, "dest_address": "null", "num":0}
                        
                        slice_parent_taskid = None
                        if task['output'] is not None:
                            first_output_member_var = task['output'][0]
                            if isinstance(first_output_member_var, tuple):
                                first_output_var = first_output_member_var[0]
                            else:
                                first_output_var = first_output_member_var
                            output_vars[first_output_var] = 0
                            
                            for slice_parent_task_id, slice_parent_task_id_details in task_dict.items():
                                if 'output' in slice_parent_task_id_details:
                                    for var_name in slice_parent_task_id_details['output']:
                                        if var_name == first_input_name:
                                            slice_parent_taskid = slice_parent_task_id
                                            break
                                else:
                                    slice_parent_taskid = "noparent"

                            slice_map[str(first_output_var)] = {"taskid":slice_parent_taskid, "slice_var": first_input_name, "slice_length": slice_length, "data_type": slice_data_type}

                    task_dict[str(task_id)] = {"input": input_vars, "output": output_vars, "is_spmd": task_is_spmd, "min_core_num": task_min_core_num}
                    output_vars_dest_addr = {str(key): value for key, value in output_vars.items()}
                    task_dict_dest_addr[str(task_id)] = {"input": input_vars_dest_addr, "output": output_vars_dest_addr}
                
                
                
                dag_info[dag_name] = {
                    "task_dict": task_dict,
                    "task_dict_dest_addr": task_dict_dest_addr,
                    "data_concat": data_concat,
                    "which_concat":which_concat,
                    "pinned_vars": pinned_vars,
                }

                all_dag_info[dag_name] = {
                    "schedule_interval": schedule_interval,
                    "start_date": start_date,
                    "end_date": end_date
                }
                
                with open(f"./variable/map/all_dag_info.json", "w") as json_file:
                    json.dump(all_dag_info, json_file, indent=4)
                
                with open(f"./variable/map/{dag_name}_slice.json", "w") as json_file:
                    json.dump(slice_map, json_file, indent=4)
                
                filename = f"./variable/map/{dag_name}_task.json"
                os.makedirs(os.path.dirname(filename), exist_ok=True)

                loaded_mapping_task = {}
                for task_id in task_dict:
                    actual_task_name = task_id.split('*')[0] if '*' in task_id else task_id
                    if actual_task_name == "concat":
                        continue
                    loaded_mapping_task[task_id] = {"text_address": "0x00000000", "data_address": "0x00020000"}

                with open(filename, "w") as json_file:
                    json.dump(loaded_mapping_task, json_file, indent=4)

        # READ statement
            elif op == 'READ':
                for target in instr[1]:
                    if self.dc < len(self.data):
                        value = ('NUM', self.data[self.dc])
                        self.assign(target, value)
                        self.dc += 1
                    else:
                        # No more data.  Program ends
                        return
            elif op == 'IF':
                relop = instr[1]
                newline = instr[2]
                if (self.releval(relop)):
                    self.goto(newline)
                    continue

            elif op == 'FOR':
                loopvar = instr[1]
                initval = instr[2]
                finval = instr[3]
                stepval = instr[4]

                # Check to see if this is a new loop
                if not self.loops or self.loops[-1][0] != self.pc:
                    # Looks like a new loop. Make the initial assignment
                    newvalue = initval
                    self.assign((loopvar, None, None), initval)
                    if not stepval:
                        stepval = ('NUM', 1)
                    stepval = self.eval(stepval)    # Evaluate step here
                    self.loops.append((self.pc, stepval))
                else:
                    # It's a repeat of the previous loop
                    # Update the value of the loop variable according to the
                    # step
                    stepval = ('NUM', self.loops[-1][1])
                    newvalue = (
                        'BINOP', '+', ('VAR', (loopvar, None, None)), stepval)

                if self.loops[-1][1] < 0:
                    relop = '>='
                else:
                    relop = '<='
                if not self.releval(('RELOP', relop, newvalue, finval)):
                    # Loop is done. Jump to the NEXT
                    self.pc = self.loopend[self.pc]
                    self.loops.pop()
                else:
                    self.assign((loopvar, None, None), newvalue)

            elif op == 'NEXT':
                if not self.loops:
                    print("NEXT WITHOUT FOR AT LINE %s" % line)
                    return

                nextvar = instr[1]
                self.pc = self.loops[-1][0]
                loopinst = self.prog[self.stat[self.pc]]
                forvar = loopinst[1]
                if nextvar != forvar:
                    print("NEXT DOESN'T MATCH FOR AT LINE %s" % line)
                    return
                continue
            elif op == 'GOSUB':
                newline = instr[1]
                if self.gosub:
                    print("ALREADY IN A SUBROUTINE AT LINE %s" % line)
                    return
                self.gosub = self.stat[self.pc]
                self.goto(newline)
                continue

            elif op == 'RETURN':
                if not self.gosub:
                    print("RETURN WITHOUT A GOSUB AT LINE %s" % line)
                    return
                self.goto(self.gosub)
                self.gosub = None

            elif op == 'FUNC':
                fname = instr[1]
                pname = instr[2]
                expr = instr[3]

                def eval_func(pvalue, name=pname, self=self, expr=expr):
                    self.assign((pname, None, None), pvalue)
                    return self.eval(expr)
                self.functions[fname] = eval_func

            elif op == 'DIM':
                for vname, x, y in instr[1]:
                    if y == 0:
                        # Single dimension variable
                        self.lists[vname] = [0] * x
                    else:
                        # Double dimension variable
                        temp = [0] * y
                        v = []
                        for i in range(x):
                            v.append(temp[:])
                        self.tables[vname] = v

            self.pc += 1
        
        
    # Utility functions for program listing
    def expr_str(self, expr):
        etype = expr[0]
        if etype == 'NUM':
            return str(expr[1])
        elif etype == 'GROUP':
            return "(%s)" % self.expr_str(expr[1])
        elif etype == 'UNARY':
            if expr[1] == '-':
                return "-" + str(expr[2])
        elif etype == 'BINOP':
            return "%s %s %s" % (self.expr_str(expr[2]), expr[1], self.expr_str(expr[3]))
        elif etype == 'VAR':
            return self.var_str(expr[1])

    def relexpr_str(self, expr):
        return "%s %s %s" % (self.expr_str(expr[2]), expr[1], self.expr_str(expr[3]))

    def var_str(self, var):
        varname, dim1, dim2 = var
        if not dim1 and not dim2:
            return varname
        if dim1 and not dim2:
            return "%s(%s)" % (varname, self.expr_str(dim1))
        return "%s(%s,%s)" % (varname, self.expr_str(dim1), self.expr_str(dim2))

    # Create a program listing
    def list(self):
        stat = list(self.prog)      # Ordered list of all line numbers
        stat.sort()
        for line in stat:
            instr = self.prog[line]
            op = instr[0]
            if op in ['END', 'STOP', 'RETURN']:
                
                print("%s %s" % (line, op))
                continue
            elif op == 'REM':
                print("%s %s" % (line, instr[1]))
            elif op == 'PRINT':
                _out = "%s %s " % (line, op)
                first = 1
                for p in instr[1]:
                    if not first:
                        _out += ", "
                    if p[0] and p[1]:
                        _out += '"%s"%s' % (p[0], self.expr_str(p[1]))
                    elif p[1]:
                        _out += self.expr_str(p[1])
                    else:
                        _out += '"%s"' % (p[0],)
                    first = 0
                if instr[2]:
                    _out += instr[2]
                print(_out)
            elif op == 'LET':
                print("%s LET %s = %s" %
                      (line, self.var_str(instr[1]), self.expr_str(instr[2])))
            elif op == 'READ':
                _out = "%s READ " % line
                first = 1
                for r in instr[1]:
                    if not first:
                        _out += ","
                    _out += self.var_str(r)
                    first = 0
                print(_out)
            elif op == 'IF':
                print("%s IF %s THEN %d" %
                      (line, self.relexpr_str(instr[1]), instr[2]))
            elif op == 'GOTO' or op == 'GOSUB':
                print("%s %s %s" % (line, op, instr[1]))
            elif op == 'FOR':
                _out = "%s FOR %s = %s TO %s" % (
                    line, instr[1], self.expr_str(instr[2]), self.expr_str(instr[3]))
                if instr[4]:
                    _out += " STEP %s" % (self.expr_str(instr[4]))
                print(_out)
            elif op == 'NEXT':
                print("%s NEXT %s" % (line, instr[1]))
            elif op == 'FUNC':
                print("%s DEF %s(%s) = %s" %
                      (line, instr[1], instr[2], self.expr_str(instr[3])))
            elif op == 'DIM':
                _out = "%s DIM " % line
                first = 1
                for vname, x, y in instr[1]:
                    if not first:
                        _out += ","
                    first = 0
                    if y == 0:
                        _out += "%s(%d)" % (vname, x)
                    else:
                        _out += "%s(%d,%d)" % (vname, x, y)

                print(_out)
            elif op == 'DATA':
                _out = "%s DATA " % line
                first = 1
                for v in instr[1]:
                    if not first:
                        _out += ","
                    first = 0
                    _out += v
                print(_out)

    # -----------------------------------------------------------------------
    # 阶段一步骤1 & 步骤2：临时变量生命周期分析
    # 产出：./IJ/<dag>/temp_variables.json
    #       ./variable/map/<dag>_temp_lifetime.json
    # -----------------------------------------------------------------------
    def _get_task_output_size(self, producer_task_id, output_var_name, variable_dict, producer_output_port=0):
        """
        查询临时变量的 size_bytes（字节数）。
        优先从 variable_dict 中该变量的 length 字段获取。
        回退：从 scalar_o.json 查 actual_task_name 的 vreturn_o[producer_output_port]。
        找不到则报错。
        返回 int（字节数）。
        """
        if output_var_name in variable_dict:
            length = variable_dict[output_var_name].get('length')
            if length is not None:
                return int(length)

        # 回退：从 scalar_o.json 查 actual_task_name 的 vreturn_o
        actual_task_name = producer_task_id.split('*')[0] if '*' in producer_task_id else producer_task_id
        scalar_o_path = './venus_test/scalar_o.json'
        if os.path.exists(scalar_o_path):
            with open(scalar_o_path, 'r') as f:
                scalar_o_data = json.load(f)
            for item in scalar_o_data:
                if item.get('function') == actual_task_name:
                    vreturn_o = item.get('vreturn_o', [])
                    # vreturn_o 是混合列表：dict 元素是输出端口（{varname: size}），
                    # 字符串元素是指针参数（&varname），需要跳过字符串按端口序号索引
                    dict_entries = [v for v in vreturn_o if isinstance(v, dict)]
                    if dict_entries:
                        port = producer_output_port if producer_output_port < len(dict_entries) else 0
                        vec = dict_entries[port]
                        size = list(vec.values())[0]
                        if isinstance(size, str):
                            raise ValueError(
                                f"scalar_o.json has unresolved output size '{size}' for variable "
                                f"'{output_var_name}' from task '{producer_task_id}' (port {producer_output_port}). "
                                f"Fix dsl/venus_test/scalar_o.py sizeof parsing for the producer Task .c file."
                            )
                        return int(size)

        raise ValueError(
            f"Cannot determine size for temporary variable '{output_var_name}' "
            f"produced by task '{producer_task_id}' (output port {producer_output_port}). "
            f"Not found in variable_dict and not found in scalar_o.json."
        )

    def _generate_temp_variables(self, dag_name, dag_folder, task_dict, variable_dict, pinned_vars=None):
        """
        识别临时变量（task 输出中未声明为 return_value 的变量），
        做后向扫描确定生命周期，写出 temp_variables.json 和
        <dag>_temp_lifetime.json。
        pinned_vars: #pragma temp pin 声明的变量名列表（保序去重后）。
        """
        if pinned_vars is None:
            pinned_vars = []
        # 加载 scalar_o.json（保留，供其他逻辑使用；临时变量 size_bytes 不依赖此文件）
        # return_value 变量名集合
        return_value_names = {
            var for var, info in variable_dict.items()
            if info.get('type') == TYPE_RETURN_VALUE
        }

        # 按 task_dict 插入顺序建立有序 task 列表（跳过 concat/slice）
        ordered_tasks = []
        for task_id, task_info in task_dict.items():
            actual = task_id.split('*')[0] if '*' in task_id else task_id
            if actual in ('concat', 'slice'):
                continue
            ordered_tasks.append((task_id, task_info))

        # 收集临时变量：producer_task_id、producer_task_order、output_port
        # key = var_name，value = dict
        # 注意：return_value 也是 temp 变量，需要包含在内（双重身份）
        temp_vars = {}
        for task_order, (task_id, task_info) in enumerate(ordered_tasks):
            actual = task_id.split('*')[0] if '*' in task_id else task_id
            for output_var, output_port in task_info.get('output', {}).items():
                is_return = output_var in return_value_names
                temp_vars[output_var] = {
                    "name": output_var,
                    "producer_task_id": task_id,
                    "producer_task_order": task_order,
                    "producer_output_port": int(output_port),
                    "is_return_value": is_return,
                    "is_pinned": False,
                    "pin_order": -1,
                    "birth_order": task_order,
                    "last_use_order": task_order,   # 初始值，后向扫描会更新
                    "size_bytes": None,
                    "dest_address": "null",
                    "_actual_task_name": actual,
                }

        if not temp_vars:
            if pinned_vars:
                raise ValueError(
                    f"#pragma temp pin names {pinned_vars} but DAG '{dag_name}' has no temp outputs"
                )
            # 没有临时变量，写空文件
            with open(f"{dag_folder}/temp_variables.json", 'w') as f:
                json.dump([], f, indent=4)
            os.makedirs('./variable/map', exist_ok=True)
            with open(f"./variable/map/{dag_name}_temp_lifetime.json", 'w') as f:
                json.dump([], f, indent=4)
            return

        # 后向扫描：找每个临时变量最后一次出现在哪个 task 的 input 里
        # &var 和 var 等价：input key 可能带 '*' 后缀（重复变量计数），
        # 需要去掉 '*' 后缀再比较变量名。
        for task_order, (task_id, task_info) in enumerate(ordered_tasks):
            for input_key in task_info.get('input', {}):
                # 去掉重复计数后缀，再去掉可能的指针标记（指针在 is_pointer 字段，
                # 变量名本身不含 '&'，但可能含 '*N' 后缀）
                base_var = input_key.split('*')[0] if '*' in input_key else input_key
                if base_var in temp_vars:
                    if task_order > temp_vars[base_var]['last_use_order']:
                        temp_vars[base_var]['last_use_order'] = task_order

        # 无用值：从未被其它 task 作为输入；return_value：一律隐式专属区
        unused_vars = [
            vname for vname, info in temp_vars.items()
            if (not info['is_return_value']
                and info['last_use_order'] == info['birth_order'])
        ]
        return_vars = [
            vname for vname, info in temp_vars.items()
            if info['is_return_value']
        ]
        auto_pin_names = set(unused_vars) | set(return_vars)

        # Warning-1：存在无用值则提示（与是否显式 pin 无关）
        if unused_vars:
            print(
                f"Warning: {unused_vars} will never be used as input by any task; "
                f"they are unused values (DAG '{dag_name}')."
            )

        # 应用 #pragma temp pin：必须是已存在的 temp 输出名；显式 pin 仍对任意 temp 有效
        redundant_explicit = []
        for idx, vname in enumerate(pinned_vars):
            if vname not in temp_vars:
                raise ValueError(
                    f"#pragma temp pin('{vname}'): not a task output temp variable in DAG '{dag_name}'"
                )
            if vname in auto_pin_names:
                redundant_explicit.append(vname)
            temp_vars[vname]['is_pinned'] = True
            temp_vars[vname]['pin_order'] = idx

        # Warning-2：显式 pin 了无用值或 return_value → 提示后台已隐式专属区
        if redundant_explicit:
            detail_parts = []
            for vname in redundant_explicit:
                if vname in unused_vars:
                    detail_parts.append(f"'{vname}' is an unused value")
                else:
                    detail_parts.append(f"'{vname}' is a return_value")
            print(
                f"Warning: {', '.join(detail_parts)}; "
                f"the backend has already allocated exclusive storage for "
                f"{'it' if len(redundant_explicit) == 1 else 'them'} "
                f"(DAG '{dag_name}')."
            )

        # 隐式 pin：无用值 ∪ return_value 中尚未被显式 pin 的，追加 pin_order
        next_order = len(pinned_vars)
        for vname in sorted(
            auto_pin_names,
            key=lambda n: (temp_vars[n]['birth_order'], n),
        ):
            if not temp_vars[vname]['is_pinned']:
                temp_vars[vname]['is_pinned'] = True
                temp_vars[vname]['pin_order'] = next_order
                next_order += 1

        # 查询 size_bytes
        for var_name, info in temp_vars.items():
            info['size_bytes'] = self._get_task_output_size(
                info['producer_task_id'], var_name, variable_dict,
                info['producer_output_port']
            )

        # 整理输出，去掉内部辅助字段
        result = []
        for var_name, info in temp_vars.items():
            entry = {k: v for k, v in info.items() if k != '_actual_task_name'}
            result.append(entry)

        # 写出 ./IJ/<dag>/temp_variables.json
        with open(f"{dag_folder}/temp_variables.json", 'w') as f:
            json.dump(result, f, indent=4)

        # 写出 ./variable/map/<dag>_temp_lifetime.json
        os.makedirs('./variable/map', exist_ok=True)
        with open(f"./variable/map/{dag_name}_temp_lifetime.json", 'w') as f:
            json.dump(result, f, indent=4)

        print(f"[temp_vars] {dag_name}: {len(result)} temporary variable(s) found "
              f"(.bas preliminary order; final lifetime written by temp_alloc after HEFT).")

    # Erase the current program
    def new(self):
        self.prog = {}

    # Insert statements
    def add_statements(self, prog):
        for line, stat in prog.items():
            self.prog[line] = stat

    # Delete a statement
    def del_line(self, lineno):
        try:
            del self.prog[lineno]
        except KeyError:
            pass
