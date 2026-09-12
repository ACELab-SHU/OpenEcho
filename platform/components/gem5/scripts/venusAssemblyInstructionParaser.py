#!/usr/bin/env python3
"""
自定义RISC-V指令解析脚本 (增强版)
支持解析 vns_brdcst.ivx 和 vns_sll.ivv 等格式的指令
"""

import re

class RiscvCustomInstructionParser:
    def __init__(self):
        # 标量寄存器名称到编号的映射 
        self.reg_map = {
            'zero': 0, 'ra': 1, 'sp': 2, 'gp': 3, 'tp': 4,
            't0': 5, 't1': 6, 't2': 7, 's0': 8, 's1': 9,
            'a0': 10, 'a1': 11, 'a2': 12, 'a3': 13,
            'a4': 14, 'a5': 15, 'a6': 16, 'a7': 17,
            's2': 18, 's3': 19, 's4': 20, 's5': 21, 's6': 22, 's7': 23,
            's8': 24, 's9': 25, 's10': 26, 's11': 27,
            't3': 28, 't4': 29, 't5': 30, 't6': 31
        }
        
        # 指令名称到操作码的映射
        self.opcode_map = {
            'vns_brdcst': 'VBRDCST',
            'vns_sll': 'VSLL',
            # 可以继续添加其他指令映射
        }
        
        # vew值到名称的映射
        self.vew_map = {'0': 'EW8', '1': 'EW16', '2': 'EW32', '3': 'EW64'}
        
        # vm_r值到布尔值的映射
        self.vm_map = {'0': 'false', '1': 'true'}

    def extract_vector_reg_number(self, reg_name):
        """从向量寄存器名称中提取数字编号"""
        # 支持 vns0, vns132, vns164 等格式
        match = re.search(r'vns(\d+)', reg_name)
        if match:
            return int(match.group(1))
        return 0

    def parse_li_instruction(self, instruction):
        """解析li指令，提取寄存器值和立即数"""
        result = {}
        li_pattern = r'li\s+([a-zA-Z0-9]+)\s*,\s*([0-9-]+)'
        match = re.match(li_pattern, instruction.strip())
        
        if match:
            reg = match.group(1)
            imm = int(match.group(2))
            result[reg] = imm
        return result

    def parse_instruction_operands(self, instruction):
        """通用指令操作数解析"""
        instruction = instruction.split('#')[0].strip()
        parts = [part.strip() for part in instruction.split(None, 1)]
        
        if len(parts) < 2:
            raise ValueError("指令格式错误：缺少操作数部分")
        
        # 解析操作码和func3
        opcode_part = parts[0]
        if '.' in opcode_part:
            opcode, func3 = opcode_part.split('.', 1)
            func3 = func3.upper()
        else:
            opcode = opcode_part
            func3 = "UNKNOWN"
        
        # 解析操作数部分
        operands_part = parts[1]
        operands = []
        current_operand = ""
        in_parenthesis = 0
        
        for char in operands_part:
            if char == '(':
                in_parenthesis += 1
            elif char == ')':
                in_parenthesis -= 1
            
            if char == ',' and in_parenthesis == 0:
                operands.append(current_operand.strip())
                current_operand = ""
            else:
                current_operand += char
        
        if current_operand:
            operands.append(current_operand.strip())
        
        return opcode, func3, [op.strip('()') for op in operands]

# vns_brdcst.ivx  vns171, a0, vns171, 1, 0, a1, 1
# vns_and.ivx     vns32, a0, vns68, 1, 0, t2, 0
    def parse_vns_basic(self, instruction, reg_values):
        """解析vns_brdcst指令"""
        opcode, func3, operands = self.parse_instruction_operands(instruction)
        
        if len(operands) < 7:
            raise ValueError(f"操作数数量不足，期望7个，得到{len(operands)}个")
        
        try:
            vd1_head = self.extract_vector_reg_number(operands[0])
            vs1_head = self.reg_map.get(operands[1].strip(), 0) if func3 == 'IVX' else self.extract_vector_reg_number(operands[1])
            vs2_head = self.extract_vector_reg_number(operands[2])
            vew = self.vew_map.get(operands[3].strip(), 'EW8')
            vm_r = self.vm_map.get(operands[4].strip(), 'false')
            vl_reg = operands[5].strip()
            scalar_reg = operands[1].strip()
        except IndexError as e:
            raise ValueError(f"操作数解析错误: {e}")
        
        # 获取vl值
        vl = reg_values.get(vl_reg, 0)
        
        # 获取scalar_op值（从a0寄存器）
        scalar_op = reg_values.get(scalar_reg, 0)
        
        opcode = 'v'+opcode.split('_')[1]
        return {
            'op': opcode.upper(),
            'vew': vew,
            'func3': func3,
            'vl': vl,
            'vs1_head': vs1_head,
            'vs2_head': vs2_head,
            'vd1_head': vd1_head,
            'vd2_head': 0,  # 固定为0
            'vm_r': vm_r,
            'scalar_op': scalar_op
        }

    def parse_vns_mw(self, instruction, reg_values):
        """解析vns_mw指令"""
        opcode, func3, operands = self.parse_instruction_operands(instruction)
        
        if len(operands) < 6:
            raise ValueError(f"操作数数量不足，期望6个，得到{len(operands)}个")
        
        try:
            vs1_head = self.reg_map.get(operands[0].strip(), 0) if func3 == 'MVX' else self.extract_vector_reg_number(operands[0])
            vs2_head = self.extract_vector_reg_number(operands[1])
            vew = self.vew_map.get(operands[2].strip(), 'EW8')
            vm_r = self.vm_map.get(operands[3].strip(), 'false')
            vl_reg = operands[4].strip()
            scalar_reg = operands[0].strip()
        except IndexError as e:
            raise ValueError(f"操作数解析错误: {e}")
        
        # 获取vl值
        vl = reg_values.get(vl_reg, 0)
        
        # 获取scalar_op值（从a0寄存器）
        scalar_op = reg_values.get(scalar_reg, 0)
        
        opcode = 'v'+opcode.split('_')[1]
        return {
            'op': opcode.upper(),
            'vew': vew,
            'func3': func3,
            'vl': vl,
            'vs1_head': vs1_head,
            'vs2_head': vs2_head,
            'vd1_head': 0,
            'vd2_head': 0,  # 固定为0
            'vm_r': vm_r,
            'scalar_op': scalar_op
        }

        
    # vns_range.misc  vns32, 1, 0, 0, t2, 0
    def parse_vns_range(self, instruction, reg_values):
        """解析vns_range指令"""
        opcode, func3, operands = self.parse_instruction_operands(instruction)
        
        if len(operands) < 6:
            raise ValueError(f"操作数数量不足，期望6个，得到{len(operands)}个")
        
        try:
            vd1_head = self.extract_vector_reg_number(operands[0])
            vew = self.vew_map.get(operands[1].strip(), 'EW8')
            vm_r = self.vm_map.get(operands[2].strip(), 'false')
            vl_reg = operands[4].strip()
        except IndexError as e:
            raise ValueError(f"操作数解析错误: {e}")
        
        # 获取vl值
        vl = reg_values.get(vl_reg, 0)
        
        # 获取scalar_op值（从a0寄存器）
        
        opcode = 'v'+opcode.split('_')[1]
        return {
            'op': opcode.upper(),
            'vew': vew,
            'func3': "OPMISC",
            'vl': vl,
            'vs1_head': 0,
            'vs2_head': 0,
            'vd1_head': vd1_head,
            'vd2_head': 0,
            'vm_r': vm_r,
            'scalar_op': 0
        }

    # vns_muladd.ivv	vns16, vns32, vns64, vns64, 0, 0, t2, 0
    def parse_vns_composite(self, instruction, reg_values):
        """解析vns_brdcst指令"""
        opcode, func3, operands = self.parse_instruction_operands(instruction)
        
        if len(operands) < 8:
            raise ValueError(f"操作数数量不足，期望8个，得到{len(operands)}个")
        
        try:
            vd1_head = self.extract_vector_reg_number(operands[0])
            vd2_head = self.extract_vector_reg_number(operands[3])
            vs1_head = self.reg_map.get(operands[1].strip(), 0) if func3 == 'IVX' else self.extract_vector_reg_number(operands[1])
            vs2_head = self.extract_vector_reg_number(operands[2])
            vew = self.vew_map.get(operands[4].strip(), 'EW8')
            vm_r = self.vm_map.get(operands[5].strip(), 'false')
            vl_reg = operands[6].strip()
            scalar_reg = operands[1].strip()
        except IndexError as e:
            raise ValueError(f"操作数解析错误: {e}")
        
        # 获取vl值
        vl = reg_values.get(vl_reg, 0)
        
        # 获取scalar_op值（从a0寄存器）
        scalar_op = reg_values.get(scalar_reg, 0)
        
        opcode = 'v'+opcode.split('_')[1]
        return {
            'op': opcode.upper(),
            'vew': vew,
            'func3': func3,
            'vl': vl,
            'vs1_head': vs1_head,
            'vs2_head': vs2_head,
            'vd1_head': vd1_head,
            'vd2_head': vd2_head,
            'vm_r': vm_r,
            'scalar_op': scalar_op
        }
    #                   vd2    vd1    vd2op  vd1op  vs2    vs1   vew vmr vl
    # vns_cmxmul.ivv	vns16, vns32, vns16, vns32, vns64, vns68, 1, 0, t2, 0
    def parse_vns_complex(self, instruction, reg_values):
        """解析vns_brdcst指令"""
        opcode, func3, operands = self.parse_instruction_operands(instruction)
        
        if len(operands) < 10:
            raise ValueError(f"操作数数量不足，期望10个，得到{len(operands)}个")
        
        try:
            vd1_head = self.extract_vector_reg_number(operands[1])
            vd2_head = self.extract_vector_reg_number(operands[0])
            vs1_head = self.extract_vector_reg_number(operands[5])
            vs2_head = self.extract_vector_reg_number(operands[4])
            vew = self.vew_map.get(operands[6].strip(), 'EW8')
            vm_r = self.vm_map.get(operands[7].strip(), 'false')
            vl_reg = operands[8].strip()
        except IndexError as e:
            raise ValueError(f"操作数解析错误: {e}")
        
        # 获取vl值
        vl = reg_values.get(vl_reg, 0)
        
        
        opcode = 'v'+opcode.split('_')[1]
        return {
            'op': opcode.upper(),
            'vew': vew,
            'func3': func3,
            'vl': vl,
            'vs1_head': vs1_head,
            'vs2_head': vs2_head,
            'vd1_head': vd1_head,
            'vd2_head': vd2_head,
            'vm_r': vm_r,
            'scalar_op': 0
        }


    def parse_custom_instruction(self, instruction, reg_values):
        """根据指令类型和func3分派到对应的解析函数"""
        # 解析指令操作码和func3
        opcode, func3, _ = self.parse_instruction_operands(instruction)
        
        # 将指令名称转换为小写，便于比较
        opcode_lower = opcode.lower()
        
        # 0. 首先检查range指令（使用parse_vns_range）
        range_instructions = [
            'vns_range'
        ]
        
        for range_instr in range_instructions:
            if range_instr in opcode_lower:
                return self.parse_vns_range(instruction, reg_values)
        
        # 1. 首先检查复合指令（使用parse_vns_composite）
        composite_instructions = [
            'vns_muladd', 'vns_mulsub', 'vns_addmul', 'vns_submul'
        ]
        
        for comp_instr in composite_instructions:
            if comp_instr in opcode_lower:
                return self.parse_vns_composite(instruction, reg_values)
        
        # 2. 检查复杂指令（使用parse_vns_complex）
        complex_instructions = [
            'vns_cmxmul'
        ]
        
        for complex_instr in complex_instructions:
            if complex_instr in opcode_lower:
                return self.parse_vns_complex(instruction, reg_values)
        
        # 3. 特殊指令列表（需要根据func3选择解析方法）
        special_instructions = [
            'vns_seq', 'vns_sne', 'vns_sltu', 'vns_slt', 
            'vns_sleu', 'vns_sle', 'vns_sgtu', 'vns_sgt'
        ]
        
        # 检查是否是特殊指令
        for special_instr in special_instructions:
            if special_instr in opcode_lower:
                # 对于特殊指令，根据func3选择解析方法
                if func3 and func3.upper() in ['IVV', 'IVX']:
                    return self.parse_vns_basic(instruction, reg_values)
                else:
                    return self.parse_vns_mw(instruction, reg_values)
        
        # 4. 普通指令的分派字典
        instruction_handlers = {
            'vns_and': self.parse_vns_basic,
            'vns_or': self.parse_vns_basic,
            'vns_xor': self.parse_vns_basic,
            'vns_brdcst': self.parse_vns_basic,
            'vns_sll': self.parse_vns_basic,
            'vns_srl': self.parse_vns_basic,
            'vns_sra': self.parse_vns_basic,
            'vns_add': self.parse_vns_basic,
            'vns_sadd': self.parse_vns_basic,
            'vns_saddu': self.parse_vns_basic,
            'vns_rsub': self.parse_vns_basic,
            'vns_sub': self.parse_vns_basic,
            'vns_ssub': self.parse_vns_basic,
            'vns_ssubu': self.parse_vns_basic,
            'vns_mul': self.parse_vns_basic,
            'vns_mulh': self.parse_vns_basic,
            'vns_mulhu': self.parse_vns_basic,
            'vns_mulhsu': self.parse_vns_basic,
            'vns_div': self.parse_vns_basic,
            'vns_rem': self.parse_vns_basic,
            'vns_divu': self.parse_vns_basic,
            'vns_remu': self.parse_vns_basic,
        }
        
        # 查找匹配的指令处理器
        for instr_prefix, handler in instruction_handlers.items():
            if instr_prefix in opcode_lower:
                return handler(instruction, reg_values)
        
        # 5. 如果没有找到匹配的处理器，尝试使用默认解析器
        print(f"警告: 未找到指令 {opcode} 的专用处理器，尝试使用通用解析器")
        try:
            # 尝试使用basic解析器作为后备
            return self.parse_vns_basic(instruction, reg_values)
        except Exception as e:
            raise ValueError(f"不支持的指令格式: {opcode}.{func3}, 错误: {e}")
    
    def format_output(self, parsed_data):
        """格式化输出为要求的格式"""
        return (f"{parsed_data['op']}, {parsed_data['vew']}, {parsed_data['func3']}, "
                f"{parsed_data['vl']}, {parsed_data['vs1_head']}, {parsed_data['vs2_head']}, "
                f"{parsed_data['vd1_head']}, {parsed_data['vd2_head']}, {parsed_data['vm_r']}, "
                f"{parsed_data['scalar_op']}")

def parse_instruction_sequence(instruction_text):
    """
    解析多行指令序列的主函数
    """
    parser = RiscvCustomInstructionParser()
    reg_values = {}
    results = []
    
    lines = instruction_text.strip().split('\n')
    
    # 解析所有指令
    for line in lines:
        line_clean = line.split('#')[0].strip()
        if not line_clean:
            continue
            
        if line_clean.startswith('li'):
            # 解析li指令，收集寄存器值
            reg_vals = parser.parse_li_instruction(line_clean)
            reg_values.update(reg_vals)
            print(f"解析: {line_clean} -> 寄存器值: {reg_vals}")
        elif 'vns_' in line_clean:
            # 解析自定义指令
            try:
                result = parser.parse_custom_instruction(line_clean, reg_values)
                formatted_output = parser.format_output(result)
                results.append(formatted_output)
                print(f"解析: {line_clean} -> {formatted_output}")
            except Exception as e:
                error_msg = f"错误解析指令 '{line_clean}': {e}"
                results.append(error_msg)
                print(error_msg)
        else:
            print(f"跳过未知指令: {line_clean}")
    
    return results

# 测试代码
if __name__ == "__main__":
    # 测试指令序列
    test_instructions = """\
li      a1, 256
li      a0, 16
vns_brdcst.ivx  vns16, a0, vns16, 1, 0, a1, 1
li      a0, 17
vns_brdcst.ivx  vns17, a0, vns17, 1, 0, a1, 1
li      a0, 18
vns_brdcst.ivx  vns18, a0, vns18, 1, 0, a1, 1
li      a0, 19
vns_brdcst.ivx  vns19, a0, vns19, 1, 0, a1, 1
li      a0, 20
vns_brdcst.ivx  vns20, a0, vns20, 1, 0, a1, 1
li      a0, 21
vns_brdcst.ivx  vns21, a0, vns21, 1, 0, a1, 1
li      a0, 22
vns_brdcst.ivx  vns22, a0, vns22, 1, 0, a1, 1
li      a0, 23
vns_brdcst.ivx  vns23, a0, vns23, 1, 0, a1, 1
li      a0, 24
vns_brdcst.ivx  vns24, a0, vns24, 1, 0, a1, 1
li      a0, 25
vns_brdcst.ivx  vns25, a0, vns25, 1, 0, a1, 1
li      a0, 26
vns_brdcst.ivx  vns26, a0, vns26, 1, 0, a1, 1
li      a0, 27
vns_brdcst.ivx  vns27, a0, vns27, 1, 0, a1, 1
li      a0, 28
vns_brdcst.ivx  vns28, a0, vns28, 1, 0, a1, 1
li      a0, 29
vns_brdcst.ivx  vns29, a0, vns29, 1, 0, a1, 1
li      a0, 30
vns_brdcst.ivx  vns30, a0, vns30, 1, 0, a1, 1
li      a0, 31
vns_brdcst.ivx  vns31, a0, vns31, 1, 0, a1, 1
li      a0, 32
vns_brdcst.ivx  vns32, a0, vns32, 1, 0, a1, 1
li      a0, 33
vns_brdcst.ivx  vns33, a0, vns33, 1, 0, a1, 1
li      a0, 34
vns_brdcst.ivx  vns34, a0, vns34, 1, 0, a1, 1
li      a0, 35
vns_brdcst.ivx  vns35, a0, vns35, 1, 0, a1, 1
li      a0, 36
vns_brdcst.ivx  vns36, a0, vns36, 1, 0, a1, 1
li      a0, 37
vns_brdcst.ivx  vns37, a0, vns37, 1, 0, a1, 1
li      a0, 38
vns_brdcst.ivx  vns38, a0, vns38, 1, 0, a1, 1
li      a0, 39
vns_brdcst.ivx  vns39, a0, vns39, 1, 0, a1, 1
li      a0, 40
vns_brdcst.ivx  vns40, a0, vns40, 1, 0, a1, 1
li      a0, 41
vns_brdcst.ivx  vns41, a0, vns41, 1, 0, a1, 1
li      a0, 42
vns_brdcst.ivx  vns42, a0, vns42, 1, 0, a1, 1
li      a0, 43
vns_brdcst.ivx  vns43, a0, vns43, 1, 0, a1, 1
li      a0, 44
vns_brdcst.ivx  vns44, a0, vns44, 1, 0, a1, 1
li      a0, 45
vns_brdcst.ivx  vns45, a0, vns45, 1, 0, a1, 1
li      a0, 46
vns_brdcst.ivx  vns46, a0, vns46, 1, 0, a1, 1
li      a0, 47
vns_brdcst.ivx  vns47, a0, vns47, 1, 0, a1, 1
li      a0, 48
vns_brdcst.ivx  vns48, a0, vns48, 1, 0, a1, 1
li      a0, 49
vns_brdcst.ivx  vns49, a0, vns49, 1, 0, a1, 1
li      a0, 50
vns_brdcst.ivx  vns50, a0, vns50, 1, 0, a1, 1
li      a0, 51
vns_brdcst.ivx  vns51, a0, vns51, 1, 0, a1, 1
li      a0, 52
vns_brdcst.ivx  vns52, a0, vns52, 1, 0, a1, 1
li      a0, 53
vns_brdcst.ivx  vns53, a0, vns53, 1, 0, a1, 1
li      a0, 54
vns_brdcst.ivx  vns54, a0, vns54, 1, 0, a1, 1
li      a0, 55
vns_brdcst.ivx  vns55, a0, vns55, 1, 0, a1, 1
li      a0, 56
vns_brdcst.ivx  vns56, a0, vns56, 1, 0, a1, 1
li      a0, 57
vns_brdcst.ivx  vns57, a0, vns57, 1, 0, a1, 1
li      a0, 58
vns_brdcst.ivx  vns58, a0, vns58, 1, 0, a1, 1
li      a0, 59
vns_brdcst.ivx  vns59, a0, vns59, 1, 0, a1, 1
li      a0, 60
vns_brdcst.ivx  vns60, a0, vns60, 1, 0, a1, 1
li      a0, 61
vns_brdcst.ivx  vns61, a0, vns61, 1, 0, a1, 1
li      a0, 62
vns_brdcst.ivx  vns62, a0, vns62, 1, 0, a1, 1
li      a0, 63
vns_brdcst.ivx  vns63, a0, vns63, 1, 0, a1, 1
li      a0, 64
vns_brdcst.ivx  vns64, a0, vns64, 1, 0, a1, 1
li      a0, 65
vns_brdcst.ivx  vns65, a0, vns65, 1, 0, a1, 1
li      a0, 66
vns_brdcst.ivx  vns66, a0, vns66, 1, 0, a1, 1
li      a0, 67
vns_brdcst.ivx  vns67, a0, vns67, 1, 0, a1, 1
li      a0, 68
vns_brdcst.ivx  vns68, a0, vns68, 1, 0, a1, 1
li      a0, 69
vns_brdcst.ivx  vns69, a0, vns69, 1, 0, a1, 1
li      a0, 70
vns_brdcst.ivx  vns70, a0, vns70, 1, 0, a1, 1
li      a0, 71
vns_brdcst.ivx  vns71, a0, vns71, 1, 0, a1, 1
li      a0, 116
vns_brdcst.ivx  vns116, a0, vns116, 1, 0, a1, 1
li      a0, 117
vns_brdcst.ivx  vns117, a0, vns117, 1, 0, a1, 1
li      a0, 118
vns_brdcst.ivx  vns118, a0, vns118, 1, 0, a1, 1
li      a0, 119
vns_brdcst.ivx  vns119, a0, vns119, 1, 0, a1, 1
li      a0, 120
vns_brdcst.ivx  vns120, a0, vns120, 1, 0, a1, 1
li      a0, 121
vns_brdcst.ivx  vns121, a0, vns121, 1, 0, a1, 1
li      a0, 122
vns_brdcst.ivx  vns122, a0, vns122, 1, 0, a1, 1
li      a0, 123
vns_brdcst.ivx  vns123, a0, vns123, 1, 0, a1, 1
li      a0, 124
vns_brdcst.ivx  vns124, a0, vns124, 1, 0, a1, 1
li      a0, 125
vns_brdcst.ivx  vns125, a0, vns125, 1, 0, a1, 1
li      a0, 126
vns_brdcst.ivx  vns126, a0, vns126, 1, 0, a1, 1
li      a0, 127
vns_brdcst.ivx  vns127, a0, vns127, 1, 0, a1, 1
li      a0, 128
vns_brdcst.ivx  vns128, a0, vns128, 1, 0, a1, 1
li      a0, 129
vns_brdcst.ivx  vns129, a0, vns129, 1, 0, a1, 1
li      a0, 130
vns_brdcst.ivx  vns130, a0, vns130, 1, 0, a1, 1
li      a0, 131
vns_brdcst.ivx  vns131, a0, vns131, 1, 0, a1, 1
li      a0, 132
vns_brdcst.ivx  vns132, a0, vns132, 1, 0, a1, 1
li      a0, 133
vns_brdcst.ivx  vns133, a0, vns133, 1, 0, a1, 1
li      a0, 134
vns_brdcst.ivx  vns134, a0, vns134, 1, 0, a1, 1
li      a0, 135
vns_brdcst.ivx  vns135, a0, vns135, 1, 0, a1, 1
li      a0, 136
vns_brdcst.ivx  vns136, a0, vns136, 1, 0, a1, 1
li      a0, 137
vns_brdcst.ivx  vns137, a0, vns137, 1, 0, a1, 1
li      a0, 138
vns_brdcst.ivx  vns138, a0, vns138, 1, 0, a1, 1
li      a0, 139
vns_brdcst.ivx  vns139, a0, vns139, 1, 0, a1, 1
li      a0, 140
vns_brdcst.ivx  vns140, a0, vns140, 1, 0, a1, 1
li      a0, 141
vns_brdcst.ivx  vns141, a0, vns141, 1, 0, a1, 1
li      a0, 142
vns_brdcst.ivx  vns142, a0, vns142, 1, 0, a1, 1
li      a0, 143
vns_brdcst.ivx  vns143, a0, vns143, 1, 0, a1, 1
li      a0, 144
vns_brdcst.ivx  vns144, a0, vns144, 1, 0, a1, 1
li      a0, 145
vns_brdcst.ivx  vns145, a0, vns145, 1, 0, a1, 1
li      a0, 146
vns_brdcst.ivx  vns146, a0, vns146, 1, 0, a1, 1
li      a0, 147
vns_brdcst.ivx  vns147, a0, vns147, 1, 0, a1, 1
li      a0, 148
vns_brdcst.ivx  vns148, a0, vns148, 1, 0, a1, 1
li      a0, 149
vns_brdcst.ivx  vns149, a0, vns149, 1, 0, a1, 1
li      a0, 150
vns_brdcst.ivx  vns150, a0, vns150, 1, 0, a1, 1
li      a0, 151
vns_brdcst.ivx  vns151, a0, vns151, 1, 0, a1, 1
li      a0, 152
vns_brdcst.ivx  vns152, a0, vns152, 1, 0, a1, 1
li      a0, 153
vns_brdcst.ivx  vns153, a0, vns153, 1, 0, a1, 1
li      a0, 154
vns_brdcst.ivx  vns154, a0, vns154, 1, 0, a1, 1
li      a0, 155
vns_brdcst.ivx  vns155, a0, vns155, 1, 0, a1, 1
li      a0, 156
vns_brdcst.ivx  vns156, a0, vns156, 1, 0, a1, 1
li      a0, 157
vns_brdcst.ivx  vns157, a0, vns157, 1, 0, a1, 1
li      a0, 158
vns_brdcst.ivx  vns158, a0, vns158, 1, 0, a1, 1
li      a0, 159
vns_brdcst.ivx  vns159, a0, vns159, 1, 0, a1, 1
li      a0, 160
vns_brdcst.ivx  vns160, a0, vns160, 1, 0, a1, 1
li      a0, 161
vns_brdcst.ivx  vns161, a0, vns161, 1, 0, a1, 1
li      a0, 162
vns_brdcst.ivx  vns162, a0, vns162, 1, 0, a1, 1
li      a0, 163
vns_brdcst.ivx  vns163, a0, vns163, 1, 0, a1, 1
li      a0, 164
vns_brdcst.ivx  vns164, a0, vns164, 1, 0, a1, 1
li      a0, 165
vns_brdcst.ivx  vns165, a0, vns165, 1, 0, a1, 1
li      a0, 166
vns_brdcst.ivx  vns166, a0, vns166, 1, 0, a1, 1
li      a0, 167
vns_brdcst.ivx  vns167, a0, vns167, 1, 0, a1, 1
li      a0, 168
vns_brdcst.ivx  vns168, a0, vns168, 1, 0, a1, 1
li      a0, 169
vns_brdcst.ivx  vns169, a0, vns169, 1, 0, a1, 1
li      a0, 170
vns_brdcst.ivx  vns170, a0, vns170, 1, 0, a1, 1
li      a0, 171
vns_brdcst.ivx  vns171, a0, vns171, 1, 0, a1, 1
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_and.ivx     vns32, t1, vns68, 1, 0, t2, 0
vns_and.ivx     vns32, t1, vns68, 1, 0, t2, 0
vns_and.ivx     vns132, t1, vns168, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_and.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_and.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_and.ivv     vns132, vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_brdcst.ivx  vns64, t1, vns64, 0, 0, t2, 0
vns_brdcst.ivx  vns64, t1, vns64, 0, 0, t2, 0
vns_brdcst.ivx  vns164, t1, vns164, 0, 0, t2, 0
li      a1, 999
li      a0, 0
vns_brdcst.ivx  vns64, t1, vns64, 1, 0, t2, 0
vns_brdcst.ivx  vns64, t1, vns64, 1, 0, t2, 0
vns_brdcst.ivx  vns164, t1, vns164, 1, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_sll.ivx     vns32, t1, vns64, 1, 0, t2, 0
vns_sll.ivx     vns32, t1, vns64, 1, 0, t2, 0
vns_sll.ivx     vns132, t1, vns164, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_sll.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_sll.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_sll.ivv     vns132, vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_seq.ivx     vns32, t1, vns64, 1, 0, t2, 0
vns_seq.ivx     vns32, t1, vns64, 1, 0, t2, 0
vns_seq.ivx     vns132, t1, vns164, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_seq.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_seq.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_seq.ivv     vns132, vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_seq.mvx     t1, vns64, 1, 0, t2, 0
vns_seq.mvx     t1, vns64, 1, 0, t2, 0
vns_seq.mvx     t1, vns164, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_seq.mvv     vns64, vns68, 0, 0, t2, 0
vns_seq.mvv     vns64, vns68, 0, 0, t2, 0
vns_seq.mvv     vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_seq.mvx     t1, vns64, 1, 1, t2, 0
vns_seq.mvx     t1, vns64, 1, 1, t2, 0
vns_seq.mvx     t1, vns164, 1, 1, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_seq.mvv     vns64, vns68, 0, 1, t2, 0
vns_seq.mvv     vns64, vns68, 0, 1, t2, 0
vns_seq.mvv     vns164, vns168, 0, 1, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_range.misc  vns32, 1, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_sub.ivx     vns32, t1, vns68, 1, 0, t2, 0
vns_sub.ivx     vns32, t1, vns68, 1, 0, t2, 0
vns_sub.ivx     vns132, t1, vns168, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_sub.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_sub.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_sub.ivv     vns132, vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_mul.ivx     vns32, t1, vns68, 1, 0, t2, 0
vns_mul.ivx     vns32, t1, vns68, 1, 0, t2, 0
vns_mul.ivx     vns132, t1, vns168, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_mul.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_mul.ivv     vns32, vns64, vns68, 0, 0, t2, 0
vns_mul.ivv     vns132, vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      t1, 180
li      a1, 999
li      a0, 0
vns_muladd.ivx  vns16, t1, vns32, vns64, 1, 0, t2, 0
vns_muladd.ivx  vns16, t1, vns32, vns64, 1, 0, t2, 0
vns_muladd.ivx  vns116, t1, vns132, vns164, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_muladd.ivv  vns16, vns32, vns64, vns68, 0, 0, t2, 0
vns_muladd.ivv  vns16, vns32, vns64, vns68, 0, 0, t2, 0
vns_muladd.ivv  vns116, vns132, vns164, vns168, 0, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a0, 0
vns_cmxmul.ivv  vns16, vns32, vns16, vns32, vns64, vns68, 1, 0, t2, 0
vns_cmxmul.ivv  vns16, vns32, vns16, vns32, vns64, vns38, 1, 0, t2, 0
vns_cmxmul.ivv  vns116, vns132, vns116, vns132, vns164, vns138, 1, 0, t2, 0
li      t2, 1024
li      a1, 999
li      a1, 0
vns_cmxmul.ivv  vns16, vns32, vns16, vns32, vns64, vns68, 0, 0, t2, 0
vns_cmxmul.ivv  vns16, vns32, vns16, vns32, vns64, vns38, 0, 0, t2, 0
vns_cmxmul.ivv  vns116, vns132, vns116, vns132, vns164, vns138, 0, 0, t2, 0
li      t2, 1024
li      a1, 0
li      t1, 30
vns_brdcst.ivx  vns190, t1, vns190, 0, 0, t2, 0
vns_brdcst.ivx  vns290, t1, vns290, 0, 0, t2, 0
li      t1, 45
vns_brdcst.ivx  vns200, t1, vns200, 0, 0, t2, 0
vns_brdcst.ivx  vns300, t1, vns300, 0, 0, t2, 0
li      t1, 90
vns_brdcst.ivx  vns210, t1, vns210, 0, 0, t2, 0
vns_brdcst.ivx  vns310, t1, vns310, 0, 0, t2, 0
li      t1, 45
li      a1, 0
vns_div.ivx     vns32, t1, vns190, 1, 0, t2, 0
vns_div.ivx     vns32, t1, vns190, 1, 0, t2, 0
vns_div.ivx     vns132, t1, vns290, 1, 0, t2, 0
li      a1, 0
vns_div.ivx     vns32, t1, vns200, 1, 0, t2, 0
vns_div.ivx     vns32, t1, vns200, 1, 0, t2, 0
vns_div.ivx     vns132, t1, vns300, 1, 0, t2, 0
li      a1, 0
vns_div.ivx     vns32, t1, vns210, 1, 0, t2, 0
vns_div.ivx     vns32, t1, vns210, 1, 0, t2, 0
vns_div.ivx     vns132, t1, vns310, 1, 0, t2, 0
li      t2, 1024
li      a1, 0
vns_div.ivv     vns32, vns200, vns190, 0, 0, t2, 0
vns_div.ivv     vns32, vns200, vns190, 0, 0, t2, 0
vns_div.ivv     vns132, vns300, vns290, 0, 0, t2, 0
li      a1, 0
vns_div.ivv     vns32, vns200, vns200, 0, 0, t2, 0
vns_div.ivv     vns32, vns200, vns200, 0, 0, t2, 0
vns_div.ivv     vns132, vns300, vns300, 0, 0, t2, 0
vns_div.ivv     vns32, vns200, vns210, 0, 0, t2, 0
vns_div.ivv     vns32, vns200, vns210, 0, 0, t2, 0
vns_div.ivv     vns132, vns300, vns310, 0, 0, t2, 0"""
    
    print("测试指令序列解析:")
    print("=" * 60)
    
    results = parse_instruction_sequence(test_instructions)
    
    print("\n最终输出结果:")
    print("=" * 60)
    for index, result in enumerate(results):
        print("else if(VenusInstrPkt::vns_instr_gencounter == %d) this->venus_instr_pkt = new VenusInstrPkt("%index, end="")
        print(result, end="")
        print(");")
    
    # # 验证输出是否符合预期
    # expected_results = [
    #     "VBRDCST, EW16, IVX, 1024, 10 , 0 , 0 , 0 , false, 180",
    #     "VSLL, EW8, IVV, 512, 164 , 164 , 132 , 0 , false, 0"
    # ]
    
    # print("\n验证结果:")
    # print("=" * 60)
    # for i, (actual, expected) in enumerate(zip(results, expected_results)):
    #     status = "✓ 通过" if actual == expected else "✗ 失败"
    #     print(f"指令 {i+1}: {status}")
    #     print(f"  实际: {actual}")
    #     print(f"  期望: {expected}")