import sys
import os
import json
import argparse

TYPE_TEMP = 0b000
TYPE_PARAM_GLOBAL = 0b001
TYPE_DAG_DFE = 0b010
TYPE_RESERVED = 0b011
TYPE_PTR_TEMP = 0b100
TYPE_PTR_PARAM_GLOBAL = 0b101
TYPE_PTR_DAG_DFE = 0b110
TYPE_RETURN_VALUE = 0b111

DEFAULT_TASK_INPUT_TYPE_BITS = 2
TASK_INPUT_TYPE_BIT_CHOICES = (2, 3)

INDEX_INPUT_TYPES = {
  TYPE_PARAM_GLOBAL,
  TYPE_DAG_DFE,
  TYPE_PTR_PARAM_GLOBAL,
  TYPE_PTR_DAG_DFE,
}

PARENT_PORT_INPUT_TYPES = {
  TYPE_TEMP,
  TYPE_PTR_TEMP,
}

DAG_INPUT_TYPES = {
  TYPE_DAG_DFE,
  TYPE_PTR_DAG_DFE,
}


def _validate_task_input_type_bits(task_input_type_bits):
  task_input_type_bits = int(task_input_type_bits)
  if task_input_type_bits not in TASK_INPUT_TYPE_BIT_CHOICES:
    raise ValueError(
      f"TASK_INPUT_TYPE_BITS must be one of {TASK_INPUT_TYPE_BIT_CHOICES}, "
      f"got {task_input_type_bits}"
    )
  return task_input_type_bits

class DAG_parser(object):
  def __init__(self, dag_json_file, task_input_type_bits=DEFAULT_TASK_INPUT_TYPE_BITS):
    task_input_type_bits = _validate_task_input_type_bits(task_input_type_bits)
    self.parameters = {
        'TASK_CRC_BITS': 16,
        'TASK_CODE_ADDR_BITS': 22,
        'TASK_CODE_LENGTH_BITS': 22,
        'TASK_DATA_ADDR_BITS': 25,
        'TASK_DATA_LENGTH_BITS': 25,
        'HARDWARE_INFO_BITS': 11,
        'IS_SPMD_BITS': 1,
        'MIN_CORE_NUM_BITS': 6,
        'TASK_INPUT_NUM_BITS': 7,
        'TASK_OUTPUT_NUM_BITS': 4,
        'TASK_INPUT_TYPE_BITS': task_input_type_bits,
        'TASK_ID_BITS': 6,
        'TASK_OUTPUT_ID_BITS': 4,
        'TASK_INPUT_DES_ADDR_BITS': 32,
        'TASK_INPUT_SRC_ADDR_BITS': 28,
        'TASK_INPUT_LEN_BITS': 16,
        'MAX_TASK_INPUT_NUM': 64,
        'MAX_GLOBAL_PARA_NUM': 256,
        'MAX_TASK_CONTAINER_NUM': 256,
        'MAX_OUTPUT_NUM_PER_TASK': 16,
        'DATA_WIDTH': 512,
        'DATA_BYTES': 64
      }
    self.dag_json_file = dag_json_file
    self.return_value_info = '0'
    self.dag_output_file = ''
    self.dag_name = ''
    self.json_data = ''
    self.global_para_descriptor_mem = [0] * self.parameters['MAX_GLOBAL_PARA_NUM']

    # NOTE:
    # task descriptor 不再作为跨 task 复用的状态使用。
    # 原脚本这里的 self.task_descriptor_array 会导致上一个 task 的 input descriptor
    # 残留到下一个 task。现在保留这个成员仅兼容外部引用，真正解析时使用局部数组。
    self.task_descriptor_array = []

    self.task_container_mem = [0] * self.parameters['MAX_TASK_CONTAINER_NUM']
    self.output_num_value_array = ['0' * (self.parameters['TASK_OUTPUT_NUM_BITS'])] * self.parameters['MAX_TASK_CONTAINER_NUM']
    self.output_num_value = 0
    self.task_num_value = 0
    self.global_num = 0
    self.l1_input_num = 0
    self.task_num = 0
    self.input_name   = []
    self.input_offset = []
    self.input_length = []
    self.output_descriptor_mem = []
    self.global_data_end = 0

  def __field_location(self, task_idx=None, input_idx=None):
    if task_idx is None:
      return "DAG"
    if input_idx is None:
      return f"task[{task_idx}]"
    return f"task[{task_idx}].all_input[{input_idx}]"

  def __parse_int_field(self, value, field_name, task_idx=None, input_idx=None, bare_binary=False):
    if value is None:
      raise ValueError(f"{self.__field_location(task_idx, input_idx)} missing {field_name}")

    try:
      if isinstance(value, str):
        value = value.strip()
        if value.startswith(('0b', '0B')):
          return int(value, 2)
        if value.startswith(('0x', '0X')):
          return int(value, 16)
        if bare_binary and len(value) > 1 and all(ch in '01' for ch in value):
          return int(value, 2)
        return int(value)
      return int(value)
    except (TypeError, ValueError):
      raise ValueError(
        f"{self.__field_location(task_idx, input_idx)} has invalid {field_name}: {value}"
      )

  def __format_binary_field(
    self,
    value,
    bit_width,
    field_name,
    task_idx=None,
    input_idx=None,
    bare_binary=False
  ):
    value_int = self.__parse_int_field(
      value,
      field_name,
      task_idx,
      input_idx,
      bare_binary=bare_binary
    )
    if value_int < 0 or value_int >= (1 << bit_width):
      raise ValueError(
        f"{self.__field_location(task_idx, input_idx)} {field_name}={value} "
        f"does not fit in {bit_width} bits"
      )
    return bin(value_int)[2:].zfill(bit_width)

  def __input_type_bin(self, input_type, task_idx, input_idx):
    input_type_value = self.__parse_int_field(
      input_type,
      "type",
      task_idx,
      input_idx,
      bare_binary=True
    )
    type_bits = self.parameters['TASK_INPUT_TYPE_BITS']
    if input_type_value < 0 or input_type_value >= (1 << type_bits):
      raise ValueError(
        f"{self.__field_location(task_idx, input_idx)} type={input_type} "
        f"does not fit in {type_bits} bits; use --task-input-type-bits 3 "
        f"for pointer type descriptors"
      )
    return input_type_value, bin(input_type_value)[2:].zfill(type_bits)

  def __default_input_type_bin(self):
    if self.parameters['TASK_INPUT_TYPE_BITS'] == 3:
      return bin(TYPE_RESERVED)[2:].zfill(self.parameters['TASK_INPUT_TYPE_BITS'])
    return '1' * self.parameters['TASK_INPUT_TYPE_BITS']

  def __default_task_descriptor_array(self):
    """
    每个 task 都必须重新生成一份默认 input descriptor 数组。

    默认 descriptor:
      type = 11(2-bit legacy) / 011(3-bit reserved)
      port = 0
      dest_address = 0

    宽度:
      type[TASK_INPUT_TYPE_BITS] + port[10] + dest_address[32]
    """
    default_desc = (
      self.__default_input_type_bin() +
      '0' * (
        self.parameters['TASK_OUTPUT_ID_BITS'] +
        self.parameters['TASK_ID_BITS'] +
        self.parameters['TASK_INPUT_DES_ADDR_BITS']
      )
    )
    return [default_desc] * self.parameters['MAX_TASK_INPUT_NUM']

  def __dag_json_init(self):
    dag_json_path = os.path.basename(self.dag_json_file)
    self.dag_name, ext = dag_json_path.split('.')
    self.dag_output_file = './cjson/' + self.dag_name + '_' + ext + '.c'
    with open(self.dag_json_file, 'r') as f:
      self.json_data = json.load(f)
    print("Dag name:", self.dag_name)

  def __flatten_reverse(self, input_array):
    flattened_array = []
    flattened_array.extend(input_array)
    flattened_array.reverse()
    flattened_array = "".join(flattened_array)
    return flattened_array

  def __expand_bit(self, data, num, bit):
    return (num * (bit - len(data)) + data)

  def __bin2hex512(self, data):
    data_int = int(data, 2)
    hex_str = hex(data_int)[2:]
    hex_str = hex_str.zfill(int(self.parameters['DATA_WIDTH'] / 4))
    return hex_str

  def __bin2hexstr512(self, data):
    hex_str = self.__bin2hex512(data)
    split_hex = [hex_str[i:i+8] for i in range(0, len(hex_str), 8)]
    format_hex = [f"0x{item}" for item in split_hex]
    return format_hex

  def __bin2hex3072(self, data):
    data_int = int(data, 2)
    hex_str = hex(data_int)[2:]
    hex_str = hex_str.zfill(int(self.parameters['DATA_WIDTH'] / 4 * 6))
    return hex_str

  def __bin2hexstr3072(self, data):
    hex_str = self.__bin2hex3072(data)
    split_hex = [hex_str[i:i+8] for i in range(0, len(hex_str), 8)]
    format_hex = [f"0x{item}" for item in split_hex]
    return format_hex

  def __task_container_parser(self):
    for item in self.json_data:
      if "return_output" not in item:

        # FIX:
        # 每个 task 开始解析时，重新初始化 64 个 input descriptor。
        # 防止前一个 task 的 input[1..63] 残留到当前 task。
        task_descriptor_array = self.__default_task_descriptor_array()

        hash = item.get("hash")
        if hash is not None:
          hash = hash[-4:]
        hash = bin(int(hash, 16))[2:].zfill(self.parameters['TASK_CRC_BITS'])

        text_addr = item.get("text_offset")
        text_addr = bin(int(text_addr))[2:].zfill(self.parameters['TASK_CODE_ADDR_BITS'])

        text_length = item.get("text_length")
        text_length = bin(int(text_length))[2:].zfill(self.parameters['TASK_CODE_LENGTH_BITS'])

        data_addr = item.get("data_offset")
        data_addr = bin(int(data_addr))[2:].zfill(self.parameters['TASK_DATA_ADDR_BITS'])

        data_length = item.get("data_length")
        data_length = bin(int(data_length))[2:].zfill(self.parameters['TASK_DATA_LENGTH_BITS'])

        hardware_info = item.get("hardwareinfo")
        hardware_info = bin(int(hardware_info, 2))[2:].zfill(self.parameters['HARDWARE_INFO_BITS'])

        inputnum = item.get("Input_Num")
        inputnum = bin(int(inputnum))[2:].zfill(self.parameters['TASK_INPUT_NUM_BITS'])

        outputnum = item.get("Output_Num")
        outputnum = bin(int(outputnum))[2:].zfill(self.parameters['TASK_OUTPUT_NUM_BITS'])

        is_spmd = item.get("is_spmd", 0)
        is_spmd = bin(int(is_spmd))[2:].zfill(self.parameters['IS_SPMD_BITS'])

        min_core_num = item.get("min_core_num", 1)
        min_core_num = bin(int(min_core_num))[2:].zfill(self.parameters['MIN_CORE_NUM_BITS'])

        all_inputs = item.get('all_input', [])
        if all_inputs != 'None':
          for i, all_inputs_item in enumerate(all_inputs):
            if i >= self.parameters['MAX_TASK_INPUT_NUM']:
              raise ValueError(
                f"Task {self.task_num} has too many inputs: "
                f"{len(all_inputs)} > {self.parameters['MAX_TASK_INPUT_NUM']}"
              )

            input_type = all_inputs_item.get("type")
            input_type_value, input_type_bin = self.__input_type_bin(input_type, self.task_num, i)

            if input_type_value in DAG_INPUT_TYPES:
              self.l1_input_num = self.l1_input_num + 1

            if input_type_value in INDEX_INPUT_TYPES:
              port = all_inputs_item.get("index")
              port = self.__format_binary_field(
                port,
                self.parameters['TASK_ID_BITS'] + self.parameters['TASK_OUTPUT_ID_BITS'],
                "index",
                self.task_num,
                i
              )
            elif input_type_value in PARENT_PORT_INPUT_TYPES:
              port = all_inputs_item.get("parentTasksPort")
              port = self.__format_binary_field(
                port,
                self.parameters['TASK_ID_BITS'] + self.parameters['TASK_OUTPUT_ID_BITS'],
                "parentTasksPort",
                self.task_num,
                i,
                bare_binary=True
              )
            else:
              port = 0
              port = self.__format_binary_field(
                port,
                self.parameters['TASK_ID_BITS'] + self.parameters['TASK_OUTPUT_ID_BITS'],
                "unused_port",
                self.task_num,
                i
              )

            desaddr = all_inputs_item.get("dest_address")
            desaddr = self.__format_binary_field(
              desaddr,
              self.parameters['TASK_INPUT_DES_ADDR_BITS'],
              "dest_address",
              self.task_num,
              i
            )

            if input_type_bin == "110":
              one_task_descriptor = "101" + port + desaddr
            else:
              one_task_descriptor = input_type_bin + port + desaddr
            task_descriptor_array[i] = one_task_descriptor

            # record dest addr and src addr and length into global and parameter table according to index
            if input_type_value in INDEX_INPUT_TYPES:
              srcaddr = all_inputs_item.get("offset")
              srcaddr_int = self.__parse_int_field(srcaddr, "offset", self.task_num, i)
              srcaddr = self.__format_binary_field(
                srcaddr,
                self.parameters['TASK_INPUT_SRC_ADDR_BITS'],
                "offset",
                self.task_num,
                i
              )

              length = all_inputs_item.get("length")
              length_int = self.__parse_int_field(length, "length", self.task_num, i)
              length = self.__format_binary_field(
                length,
                self.parameters['TASK_INPUT_LEN_BITS'],
                "length",
                self.task_num,
                i
              )

              index = all_inputs_item.get("index")
              index = self.__parse_int_field(index, "index", self.task_num, i)
              if index <= 0 or index > self.parameters['MAX_GLOBAL_PARA_NUM']:
                raise ValueError(
                  f"{self.__field_location(self.task_num, i)} index={index} "
                  f"must be in [1, {self.parameters['MAX_GLOBAL_PARA_NUM']}]"
                )
              if index > self.global_num:
                self.global_num = index

              self.global_data_end = max(self.global_data_end, srcaddr_int + length_int)
              global_para_descriptor_mem = srcaddr + length
              self.global_para_descriptor_mem[index - 1] = self.__expand_bit(
                global_para_descriptor_mem,
                '0',
                self.parameters['DATA_WIDTH']
              )

        flattened_task_descriptor_array = self.__flatten_reverse(task_descriptor_array)

        self.task_container_mem[self.task_num] = (
          hash +
          text_addr +
          text_length +
          data_addr +
          data_length +
          hardware_info +
          is_spmd +
          min_core_num +
          inputnum +
          flattened_task_descriptor_array
        )

        self.output_num_value_array[self.task_num] = outputnum
        self.task_num += 1

    print('Total_task_num:', self.task_num)
    print('Total_global_num:', self.global_num)

    flattened_output_num_value_array = self.__flatten_reverse(self.output_num_value_array)
    self.output_num_value = self.__expand_bit(
      flattened_output_num_value_array,
      '0',
      self.parameters['DATA_WIDTH']
    )
    self.task_num_value = self.__expand_bit(
      str(bin(self.task_num)[2:]),
      '0',
      self.parameters['DATA_WIDTH']
    )

  def __return_output_parser(self):
    for item in self.json_data:
      if "return_output" in item:
        if item["return_output"] != 'None':
          return_value_num = len(item["return_output"])
          parentTasksPorts = [
            self.__format_binary_field(
              x.get("parentTasksPort"),
              self.parameters['TASK_ID_BITS'] + self.parameters['TASK_OUTPUT_ID_BITS'],
              "parentTasksPort",
              bare_binary=True
            )
            for x in item["return_output"]
          ]
          parentTasksPorts.reverse()
          self.return_value_info += "".join(parentTasksPorts)
          self.return_value_info = self.__expand_bit(self.return_value_info, '1', self.parameters['DATA_WIDTH'])
          print("Return_value_num:", return_value_num)

  def __output_addr_parser(self):
      if self.task_num == 0:
          self.output_descriptor_mem = []
          return

      outputs_per_task = 16
      outputs_per_512 = 10
      bits_per_output = 48
      bits_per_512 = 512

      total_outputs = self.task_num * outputs_per_task
      total_512_words = (total_outputs + outputs_per_512 - 1) // outputs_per_512
      total_bits = total_512_words * bits_per_512

      packed = 0

      for task_idx in range(self.task_num):
          item = self.json_data[task_idx]

          all_output = []
          if "return_output" not in item:
              all_output = item.get("all_output", [])

          for out_idx in range(outputs_per_task):
              global_out_idx = task_idx * outputs_per_task + out_idx

              word512_idx = global_out_idx // outputs_per_512
              slot_idx = global_out_idx % outputs_per_512
              bit_offset = word512_idx * bits_per_512 + slot_idx * bits_per_output

              if out_idx >= len(all_output):
                  data48 = 0
              else:
                  out = all_output[out_idx]
                  temp_offset = int(out["temp_offset"])
                  length = int(out["length"])
                  data48 = ((temp_offset & 0xFFFFFFFF) << 16) | (length & 0xFFFF)

              packed |= data48 << bit_offset

      output_buffer = packed.to_bytes(total_bits // 8, byteorder="little")

      self.output_descriptor_mem = []
      for i in range(0, len(output_buffer), 4):
          val = int.from_bytes(output_buffer[i:i+4], byteorder="little")
          self.output_descriptor_mem.append(f"0x{val:08x}")

  def __input_offset_parser(self):
    self.input_offset = [0] * self.l1_input_num
    self.input_length = [0] * self.l1_input_num
    self.input_name = [''] * self.l1_input_num

    cur_offset_id = 0
    for task_idx, item in enumerate(self.json_data):
      if "return_output" not in item:
        all_inputs = item.get('all_input', [])
        if all_inputs != 'None':
          for i, all_inputs_item in enumerate(all_inputs):
            input_type_value, _ = self.__input_type_bin(
              all_inputs_item.get("type"),
              task_idx,
              i
            )
            if input_type_value in DAG_INPUT_TYPES:
              offset = all_inputs_item.get("offset")
              length = all_inputs_item.get("length")
              name = all_inputs_item.get("name")
              self.input_offset[cur_offset_id] = offset
              self.input_length[cur_offset_id] = length
              self.input_name[cur_offset_id] = name
              cur_offset_id = cur_offset_id + 1

  def __remove_duplicates(self):
    seen = set()
    result_offsets = []
    result_lengths = []
    result_names = []
    for i in range(len(self.input_offset)):
      if self.input_offset[i] not in seen:
          seen.add(self.input_offset[i])
          result_offsets.append(self.input_offset[i])
          result_lengths.append(self.input_length[i])
          result_names.append(self.input_name[i])
    self.l1_input_num = len(result_offsets)
    self.input_offset = result_offsets
    self.input_length = result_lengths
    self.input_name = result_names

  def __write2cfile(self):
    os.makedirs(os.path.dirname(self.dag_output_file), exist_ok=True)

    with open(self.dag_output_file, 'w') as f:
      f.write('unsigned int ')
      f.write(self.dag_name)
      f.write('_task_container[] __attribute__((aligned(64), section(".dag"))) = {\n')
      for i in range(self.task_num):
        f.write(f"// task_container_mem[{i}]\n")
        for value in reversed(self.__bin2hexstr3072(self.task_container_mem[i])):
          f.write(hex(int(value, 16)) + ',\n')
      f.write('};\n\n')

      f.write('unsigned int ')
      f.write(self.dag_name)
      f.write('_global_para[] __attribute__((aligned(64), section(".dag"))) = {\n')
      for i in range(self.global_num):
        f.write(f"// global_para_descriptor_mem[{i}]\n")
        if self.global_para_descriptor_mem[i] == 0:
          print(f"no index: {i}")
          self.global_para_descriptor_mem[i] = "0" * 512
        for value in reversed(self.__bin2hexstr512(self.global_para_descriptor_mem[i])):
          f.write(hex(int(value, 16)) + ',\n')
      f.write('};\n\n')

      f.write('unsigned int ')
      f.write(self.dag_name)
      f.write('_output_addr[] __attribute__((aligned(64), section(".dag"))) = {\n')
      ints_per_task = (16 * 8) // 4
      for task_idx in range(self.task_num):
          f.write(f"// output_descriptor_mem[{task_idx}]\n")
          start = task_idx * ints_per_task
          end = start + ints_per_task
          for val in self.output_descriptor_mem[start:end]:
              f.write(val + ',\n')
      f.write('};\n\n')

      f.write('unsigned int ')
      f.write(self.dag_name)
      f.write('_output_num[] __attribute__((aligned(64), section(".dag"))) = {\n')
      for value in reversed(self.__bin2hexstr512(str(self.output_num_value))):
        f.write(hex(int(value, 16)) + ',\n')
      f.write('};\n\n')

      f.write('unsigned int ')
      f.write(self.dag_name)
      f.write('_task_num[] __attribute__((aligned(64), section(".dag"))) = {\n')
      for value in reversed(self.__bin2hexstr512(str(self.task_num_value))):
        f.write(hex(int(value, 16)) + ',\n')
      f.write('};\n\n')

      f.write('unsigned int ')
      f.write(self.dag_name)
      f.write('_return_value[] __attribute__((aligned(64), section(".dag"))) = {\n')
      for value in reversed(self.__bin2hexstr512(self.return_value_info)):
        f.write(hex(int(value, 16)) + ',\n')
      f.write('};\n\n')

      f.write(f'unsigned int {self.dag_name}_task_container_size = sizeof({self.dag_name}_task_container);\n')
      f.write(f'unsigned int {self.dag_name}_global_para_size = sizeof({self.dag_name}_global_para);\n')
      f.write(f'unsigned int {self.dag_name}_output_num_size = sizeof({self.dag_name}_output_num);\n')
      f.write(f'unsigned int {self.dag_name}_task_num_size = sizeof({self.dag_name}_task_num);\n')
      f.write(f'unsigned int {self.dag_name}_return_value_size = sizeof({self.dag_name}_return_value);\n')
      f.write(f'unsigned int {self.dag_name}_output_addr_size = sizeof({self.dag_name}_output_addr);\n')

      f.write(f"// {', '.join(self.input_name)}\n")
      f.write(f"unsigned int {self.dag_name}_input_offset[{self.l1_input_num}] = {{" + ", ".join(map(str, self.input_offset)) + "};\n")
      f.write(f"unsigned int {self.dag_name}_input_length[{self.l1_input_num}] = {{" + ", ".join(map(str, self.input_length)) + "};\n")
      if self.l1_input_num == 0:
        raise ValueError(f'The DAG "{self.dag_name}" has no input data!')

      input_data_end = 0
      for offset, length in zip(self.input_offset, self.input_length):
        offset_int = self.__parse_int_field(offset, "input_offset")
        length_int = self.__parse_int_field(length, "input_length")
        input_data_end = max(input_data_end, offset_int + length_int)

      bin_file = os.path.join(
        os.path.dirname(self.dag_json_file),
        '..',
        'bin',
        self.dag_name + '.bin'
      )
      bin_size = os.path.getsize(bin_file) if os.path.exists(bin_file) else 0
      binandjsonsize = max(bin_size, self.global_data_end, input_data_end)
      f.write(f"unsigned int {self.dag_name}_binandjsonsize = {binandjsonsize};\n")

  def read_json_file(self):
    self.__dag_json_init()
    self.__task_container_parser()
    self.__return_output_parser()
    self.__input_offset_parser()
    self.__output_addr_parser()
    self.__remove_duplicates()
    self.__write2cfile()

    print("----------------------------------------------------------------------- L2 config info ------------------------------------------------------------------------ ")
    print("Return value num             :", self.__bin2hex512(self.return_value_info))
    for i in range(self.global_num):
      print(f"Global para mem content[{i}]   :", self.__bin2hex512(self.global_para_descriptor_mem[i]))
    for i in range(self.task_num):
      print(f"Task container mem content[{i}]:", self.__bin2hex512(self.task_container_mem[i]))
    print("Output num value             :", self.__bin2hex512(str(self.output_num_value)))
    print("Task num value               :", self.__bin2hex512(str(self.task_num_value)))
    print("--------------------------------------------------------------------------------------------------------------------------------------------------------------- ")

    task_output = './cjson/' + self.dag_name + '_' + 'json.c'
    print(task_output + ' is generated')
    print("--------------------------------------------------------------------------------------------------------------------------------------------------------------- ")


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
    description="Generate DAG descriptor C files from DAG JSON files."
  )
  parser.add_argument(
    "--task-input-type-bits",
    type=_validate_task_input_type_bits,
    default=DEFAULT_TASK_INPUT_TYPE_BITS,
    choices=TASK_INPUT_TYPE_BIT_CHOICES,
    help="Task input typeid width. Use 3 for pointer-aware DSL JSON, or 2 for legacy JSON descriptors.",
  )
  parser.add_argument(
    "--legacy-task-input-type-bits",
    action="store_const",
    const=2,
    dest="task_input_type_bits",
    help="Generate legacy 2-bit task input type descriptors.",
  )
  parser.add_argument("dag_json_files", nargs="+")
  args = parser.parse_args(sys.argv[1:])

  for arg in args.dag_json_files:
    dag_parser = DAG_parser(arg, task_input_type_bits=args.task_input_type_bits)
    dag_parser.read_json_file()
