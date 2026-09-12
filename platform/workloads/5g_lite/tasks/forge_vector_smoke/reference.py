"""Independent host arithmetic; provisional golden, never an RTL authority."""
import struct

N = 32


def cases():
    return {
        "signed_ramp": ([i - 16 for i in range(N)], [31 - 2*i for i in range(N)]),
        "zero": ([0] * N, [0] * N),
        "bounds": ([1024, -1024, 1024, -1024] * 8, [1024, -1024, -1024, 1024] * 8),
    }


def golden(a, b):
    if len(a) != N or len(b) != N or any(type(v) is not int or abs(v) > 1024 for v in a + b):
        raise ValueError("Contract requires two signed i16[32] arrays in [-1024,1024]")
    summed = [x + y for x, y in zip(a, b)]
    restored = [x - y for x, y in zip(summed, b)]
    return {"sum": struct.pack("<32h", *summed), "restored": struct.pack("<32h", *restored)}


def bas(a, b):
    golden(a, b)
    return ("return_value short sum[32]\nreturn_value short restored[32]\n"
            + "parameter short a = {" + ",".join(map(str, a)) + "}\n"
            + "parameter short b = {" + ",".join(map(str, b)) + "}\n"
            + "dag dag1 = {\n [sum] = Task_forgeAdd(a, b)\n"
            + " [restored] = Task_forgeRestore(sum, b)\n}\nEND\n")
