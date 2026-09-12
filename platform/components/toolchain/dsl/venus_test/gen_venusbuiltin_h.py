
file = open("venusbuiltin.h", "w+")

venusbuiltin_normal = ["and", "or", "xor", "sll", "srl", "sra",
                       "add", "sadd", "saddu", "rsub", "sub", "ssub", "ssubu",
                       "mul", "mulh", "mulhu", "mulhsu",
                       "div", "rem", "divu", "remu", "signset", "min", "max"]
venusbuiltin_maskwrite = ["seq", "sne", "sltu", "slt", "sleu", "sle", "sgtu", "sgt"]
venusbuiltin_threesrcops = ["muladd", "mulsub", "addmul", "submul"]

for insn in venusbuiltin_normal:
    file.write("#define v" + insn + "(a, b, vmr, ...) __Venus_" + insn + "(a, b, vmr, ##__VA_ARGS__)\n")
file.write("\n")

for insn in venusbuiltin_maskwrite:
    file.write("#define v" + insn + "(a, b, vmr, vmw, ...) __Venus_" + insn + "(a, b, vmr, vmw, ##__VA_ARGS__)\n")
file.write("\n")

for insn in venusbuiltin_threesrcops:
    file.write("#define v" + insn + "(a, b, c, vmr, ...) __Venus_" + insn + "(a, b, c, vmr, ##__VA_ARGS__)\n")
file.write("\n")

file.close()