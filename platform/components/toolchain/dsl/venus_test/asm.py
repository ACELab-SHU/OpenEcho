import os
import sys

VRF_BASE_ADDR = 0x100000
VRF_BANKS_PER_LANE = 4
VRF_BYTES_PER_BANK = 2


def split_lui_addi(value):
    """把 32-bit 地址拆成 RISC-V lui/addi 可表达的立即数。"""
    upper = (value + 0x800) >> 12
    lower = value - (upper << 12)
    return upper, lower


def generate_S_file(task, mode, venus_row, venus_lane):
    template_content = """.section .text
.global {}

mv x1, x0
mv x2, x0
mv x3, x0
mv x4, x0
mv x5, x0
mv x6, x0
mv x7, x0
mv x8, x0
mv x9, x0
mv x10, x0
mv x11, x0
mv x12, x0
mv x13, x0
mv x14, x0
mv x15, x0
mv x16, x0
mv x17, x0
mv x18, x0
mv x19, x0
mv x20, x0
mv x21, x0
mv x22, x0
mv x23, x0
mv x24, x0
mv x25, x0
mv x26, x0
mv x27, x0
mv x28, x0
mv x29, x0
mv x30, x0
mv x31, x0

lui sp, 0x24
jal ra, {}
/* break */
ebreak //系统自陷
"""

    stackmvto_vrf_special_in_release_content = """.section .text
.global {}

mv x1, x0
mv x2, x0
mv x3, x0
mv x4, x0
mv x5, x0
mv x6, x0
mv x7, x0
mv x8, x0
mv x9, x0
mv x10, x0
mv x11, x0
mv x12, x0
mv x13, x0
mv x14, x0
mv x15, x0
mv x16, x0
mv x17, x0
mv x18, x0
mv x19, x0
mv x20, x0
mv x21, x0
mv x22, x0
mv x23, x0
mv x24, x0
mv x25, x0
mv x26, x0
mv x27, x0
mv x28, x0
mv x29, x0
mv x30, x0
mv x31, x0

lui	a1, 524799
addi	s5, zero, 1
sw	s5, 4(a1)
mv a1, x0
mv s5, x0
lui sp, {vrf_stack_upper}
addi sp, sp, {vrf_stack_lower}
jal ra, {}
/* break */
ebreak //系统自陷
"""

    stackmvto_datasection_special_in_release_content = """.section .text
.global {}

mv x1, x0
mv x2, x0
mv x3, x0
mv x4, x0
mv x5, x0
mv x6, x0
mv x7, x0
mv x8, x0
mv x9, x0
mv x10, x0
mv x11, x0
mv x12, x0
mv x13, x0
mv x14, x0
mv x15, x0
mv x16, x0
mv x17, x0
mv x18, x0
mv x19, x0
mv x20, x0
mv x21, x0
mv x22, x0
mv x23, x0
mv x24, x0
mv x25, x0
mv x26, x0
mv x27, x0
mv x28, x0
mv x29, x0
mv x30, x0
mv x31, x0

lui sp, 0x23
jal ra, {}
/* break */
ebreak //系统自陷
"""

    stackmvto_vrf_special_tasks = {
        "Task_getPolarInfo",
        "Task_ltePDCCHIndices",
    }
    stackmvto_datasection_special_tasks = {"Task_ltePDCCHDeinterleave"} ######### Don't use this #########
    # use_special = mode == "release" and task in special_tasks
    use_stackmvto_vrf_special  = task in stackmvto_vrf_special_tasks
    use_stackmvto_datasection_special = task in stackmvto_datasection_special_tasks
    if use_stackmvto_vrf_special:
        vrf_bytes = (
            venus_row * venus_lane *
            VRF_BANKS_PER_LANE * VRF_BYTES_PER_BANK
        )
        vrf_stack_top = VRF_BASE_ADDR + vrf_bytes
        vrf_stack_upper, vrf_stack_lower = split_lui_addi(vrf_stack_top)
        content = stackmvto_vrf_special_in_release_content.format(
            task,
            task,
            vrf_stack_upper=hex(vrf_stack_upper),
            vrf_stack_lower=vrf_stack_lower,
        )
    elif use_stackmvto_datasection_special:
        content = stackmvto_datasection_special_in_release_content.format(
            task, task
        )
    else:
        content = template_content.format(task, task)

    folder_path = "asm"
    os.makedirs(folder_path, exist_ok=True)
    file_path = os.path.join(folder_path, f"{task}.S")

    with open(file_path, "w") as file:
        file.write(content)

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(
            "Usage: python asm.py [debug|release] "
            "<venus_row> <venus_lane> task1 [task2 ...]"
        )
        sys.exit(1)

    mode = sys.argv[1]
    venus_row = int(sys.argv[2], 0)
    venus_lane = int(sys.argv[3], 0)
    tasks = sys.argv[4:]
    if mode not in ("debug", "release"):
        print("Error: Mode must be 'debug' or 'release'.")
        sys.exit(1)
    for task in tasks:
        generate_S_file(task, mode, venus_row, venus_lane)
