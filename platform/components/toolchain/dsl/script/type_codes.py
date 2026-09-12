# DSL JSON 变量 type 字段：3 位语义标签（字符串 "0b000"～"0b111"）。
# 0b011 为保留码，本轮不定义业务常量、不写入产物。
#--------------------------------
# 000temp ；
# 001param和global ；
# 010daginput和dfeinput ；
# 100指针temp ；
# 101param和global指针 ；
# 110daginput和dfeinput指针 ；
# 111return value；
#--------------------------------

TYPE_TEMP = "0b000"
TYPE_PARAM_GLOBAL = "0b001"
TYPE_DAG_DFE_STATIC = "0b010"
TYPE_PTR_TEMP = "0b100"
TYPE_PTR_PARAM_GLOBAL = "0b101"
TYPE_PTR_DAG_DFE = "0b110"
# TYPE_PTR_DAG_DFE = "0b101"
TYPE_RETURN_VALUE = "0b111"

POINTER_TYPES = (TYPE_PTR_TEMP, TYPE_PTR_PARAM_GLOBAL, TYPE_PTR_DAG_DFE)

# 与 source/main.cpp 中 POINTER_INPUT_STRUCT_BYTES 一致：标量输入区指针槽大小
POINTER_INPUT_STRUCT_BYTES = 64
