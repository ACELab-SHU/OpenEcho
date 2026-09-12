
def RoundToCeil(num, base):
    div = num // base
    rem = num % base
    if(rem):
        div = div + 1
    return (div * base)

file = open("venustype.h", "w+")

# in byte
ElementsPerRowInVenusArch = 32 

RowsInVenusArch = 64

# typedef char
for e in range(1, ElementsPerRowInVenusArch * RowsInVenusArch + 1):
    # file.write("typedef char __v" + str(e) + "i8 __attribute__((ext_vector_type(" + str(RoundToCeil(e, 32)) + ")));\n")
    file.write("typedef char __v" + str(e) + "i8 __attribute__((ext_vector_type(" + str(e) + ")));\n")

# typedef short
for e in range(1, ElementsPerRowInVenusArch * RowsInVenusArch // 2 + 1):
    # file.write("typedef short __v" + str(e) + "i16 __attribute__((ext_vector_type(" + str(RoundToCeil(e, 16)) + ")));\n")
    file.write("typedef short __v" + str(e) + "i16 __attribute__((ext_vector_type(" + str(e) + ")));\n")

file.close()