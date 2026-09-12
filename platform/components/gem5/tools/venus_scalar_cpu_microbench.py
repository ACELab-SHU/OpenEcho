#!/usr/bin/env python3
"""Build deterministic scalar600 CPU timing/conformance microbenchmarks."""

import argparse
from pathlib import Path
from types import SimpleNamespace

import venus_lsu_microbench as base


CASES = {}


def case(name):
    def register(function):
        CASES[name] = function
        return function
    return register


def header():
    return [
        '    .section .text, "ax", @progbits',
        "    .option norvc",
        # RTL starts at PC 0 while the gem5 replay enters at PC 4.  The
        # existing scalar comparators align the common architectural stream.
        "    .4byte 0x00000093",
        "    .global _start",
        "_start:",
        "    li sp, 0x24000",
        # 0x24000 is the top (exclusive) of the 16-KiB tile-0 DSPM.  Match
        # the production task prologue and move the stack into the mapped
        # range before issuing scalar loads/stores.
        "    addi sp, sp, -64",
        "    .global measure_begin",
        "measure_begin:",
    ]


def finish(lines):
    lines += ["    .global measure_end", "measure_end:"]
    lines += base.emit_return()
    return lines


@case("alu_independent")
def alu_independent():
    return finish(header() + [
        "    addi t0, zero, 3",
        "    addi t1, zero, 5",
        "    addi t2, zero, 7",
        "    addi s0, zero, 9",
        "    add s1, t0, t1",
        "    sub s2, s0, t0",
        "    xor s3, t1, t2",
        "    or s4, t0, t2",
        "    and s5, t1, t2",
        "    sll s6, t1, t0",
        "    srl s7, s6, t0",
        "    sra s8, s6, t0",
        "    slt s9, t0, t1",
        "    sltu s10, t1, t0",
        "    lui s11, 0x12345",
        "    auipc a4, 0",
    ])


@case("alu_raw")
def alu_raw():
    return finish(header() + [
        # Zero-, one- and two-instruction producer/consumer distances.
        "    addi t0, zero, 1",
        "    add t1, t0, t0",
        "    add t2, t1, t0",
        "    xor s0, t2, t1",
        "    addi s1, zero, 11",
        "    addi a4, zero, 17",
        "    sub s2, s1, t0",
        "    addi a5, zero, 23",
        "    addi a6, zero, 29",
        "    or s3, s2, t1",
        "    slli s4, s3, 3",
        "    srli s5, s4, 1",
        "    srai s6, s5, 2",
    ])


@case("rv32i_alu_immediate")
def rv32i_alu_immediate():
    return finish(header() + [
        # Cover every OP-IMM function, both signed and unsigned compare
        # corners, and the architectural x0 write suppression boundary.
        "    addi t0, zero, -17",
        "    slti t1, t0, -16",
        "    sltiu t2, t0, 1",
        "    xori s0, t0, -1",
        "    ori s1, s0, 0x155",
        "    andi s2, s1, 0x2aa",
        "    slli s3, s2, 7",
        "    srli s4, s3, 3",
        "    srai s5, t0, 4",
        "    addi zero, s5, 31",
        "    addi s6, zero, 1",
        "    slti s7, t0, 0",
        "    sltiu s8, zero, -1",
        "    lui s9, 0x80000",
        "    auipc s10, 0",
        "    add s11, s9, s10",
    ])


@case("rv32i_branch_matrix")
def rv32i_branch_matrix():
    return finish(header() + [
        # Exercise all six branch functions in both directions.  The signed
        # and unsigned pairs deliberately see the same negative operand.
        "    addi t0, zero, -1",
        "    addi t1, zero, 1",
        "    addi t2, zero, 0",
        "    beq t0, t1, branch_matrix_bad0",
        "    addi t2, t2, 1",
        "    bne t0, t1, branch_matrix_bne",
        "branch_matrix_bad0:",
        "    addi s0, zero, 0x101",
        "branch_matrix_bne:",
        "    addi t2, t2, 2",
        "    blt t0, t1, branch_matrix_blt",
        "    addi s1, zero, 0x102",
        "branch_matrix_blt:",
        "    addi t2, t2, 4",
        "    bge t0, t1, branch_matrix_bad1",
        "    addi t2, t2, 8",
        "    bltu t0, t1, branch_matrix_bad2",
        "    addi t2, t2, 16",
        "    bgeu t0, t1, branch_matrix_bgeu",
        "branch_matrix_bad1:",
        "    addi s2, zero, 0x103",
        "branch_matrix_bad2:",
        "    addi s3, zero, 0x104",
        "branch_matrix_bgeu:",
        "    addi t2, t2, 32",
        # Reverse/equalize the operands to cover the complementary outcomes
        # and keep zero-distance ALU-to-branch forwarding dependencies.
        "    addi t1, t1, -2",
        "    beq t1, t0, branch_matrix_beq",
        "    addi s4, zero, 0x105",
        "branch_matrix_beq:",
        "    bne t1, t0, branch_matrix_bad3",
        "    blt t1, t0, branch_matrix_bad4",
        "    addi t1, zero, 0",
        "    bge t1, t0, branch_matrix_bge",
        "branch_matrix_bad3:",
        "    addi s5, zero, 0x106",
        "branch_matrix_bad4:",
        "    addi s6, zero, 0x107",
        "branch_matrix_bge:",
        "    bltu t1, t0, branch_matrix_bltu",
        "    addi s7, zero, 0x108",
        "branch_matrix_bltu:",
        "    bgeu t1, t0, branch_matrix_bad5",
        "    addi t2, t2, 64",
        "    j branch_matrix_done",
        "branch_matrix_bad5:",
        "    addi s8, zero, 0x109",
        "branch_matrix_done:",
        "    addi t2, t2, 128",
    ])


@case("branch_forward")
def branch_forward():
    return finish(header() + [
        "    addi t0, zero, 5",
        "    addi t1, zero, 5",
        "    bne t0, t1, branch_bad0",
        "    addi t2, zero, 1",
        "    beq t0, t1, branch_taken0",
        "branch_bad0:",
        "    addi s0, zero, 0x111",
        "branch_taken0:",
        "    addi t0, t0, 1",
        "    blt t1, t0, branch_taken1",
        "    addi s1, zero, 0x222",
        "branch_taken1:",
        "    bgeu t0, t1, branch_taken2",
        "    addi s2, zero, 0x333",
        "branch_taken2:",
        "    addi t2, t2, 3",
        "    j branch_done",
        "    addi s3, zero, 0x444",
        "branch_done:",
        "    addi t2, t2, 5",
    ])


@case("branch_loop")
def branch_loop():
    return finish(header() + [
        "    addi t0, zero, 6",
        "    addi t1, zero, 0",
        "branch_loop_body:",
        "    addi t1, t1, 3",
        "    addi t0, t0, -1",
        "    bne t0, zero, branch_loop_body",
        "    addi t2, t1, 1",
    ])


@case("jal_jalr")
def jal_jalr():
    return finish(header() + [
        "    addi t0, zero, 1",
        "    jal ra, jal_function",
        "jal_return:",
        "    addi t0, t0, 4",
        "    auipc t1, 0",
        "    addi t1, t1, 20",
        "    jalr zero, 0(t1)",
        "    addi s0, zero, 0x111",
        "    addi s1, zero, 0x222",
        "jalr_target:",
        "    addi t0, t0, 8",
        "    j jal_done",
        "jal_function:",
        "    addi t0, t0, 2",
        "    jalr zero, 0(ra)",
        "jal_done:",
        "    addi t2, t0, 1",
    ])


@case("control_target_edges")
def control_target_edges():
    return finish(header() + [
        # JAL with rd=x0, JALR with an odd source address (scalar600 preserves
        # bit zero instead of applying the standard RISC-V clear), and an
        # immediately consumed JAL link value.
        "    addi t0, zero, 1",
        "    jal zero, control_jal_target",
        "    addi s0, zero, 0x111",
        "control_jal_target:",
        "    auipc t1, 0",
        "    addi t1, t1, 21",
        "    jalr ra, 0(t1)",
        "    addi s1, zero, 0x222",
        "    addi s2, zero, 0x333",
        "control_jalr_target:",
        "    addi t2, ra, 20",
        "    jalr zero, 0(t2)",
        "    addi s3, zero, 0x444",
        "control_return_target:",
        "    addi t0, t0, 2",
    ])


@case("mul")
def mul_case():
    return finish(header() + [
        "    addi t0, zero, -7",
        "    addi t1, zero, 9",
        "    mul t2, t0, t1",
        "    mulh s0, t0, t1",
        "    mulhu s1, t0, t1",
        "    mulhsu s2, t0, t1",
        "    add s3, t2, t1",
        "    mul s4, s3, t0",
        "    mul s5, t1, t1",
    ])


@case("mul_transitions")
def mul_transitions():
    return finish(header() + [
        "    addi t0, zero, 7",
        "    addi t1, zero, 9",
        # Continuous requests, then one- and two-ALU separation.  Keep each
        # destination live so architectural writeback is also checked.
        "    mul t2, t0, t1",
        "    mul s0, t0, t1",
        "    addi s1, zero, 1",
        "    mul s2, t0, t1",
        "    addi s3, zero, 2",
        "    addi s4, zero, 3",
        "    mul s5, t0, t1",
        # Immediate ALU-result consumption by MUL and MUL-result consumption
        # by ALU distinguish operand admission from fixed multiplier latency.
        "    add s6, t0, t1",
        "    mul s7, s6, t0",
        "    add s8, s7, t1",
    ])


@case("muldiv_arch_edges")
def muldiv_arch_edges():
    return finish(header() + [
        # RV32M architectural corners: INT_MIN/-1, unsigned divide by zero,
        # high-half sign combinations, and direct result consumers.
        "    lui t0, 0x80000",
        "    addi t1, zero, -1",
        "    div t2, t0, t1",
        "    rem s0, t0, t1",
        "    divu s1, t0, zero",
        "    remu s2, t0, zero",
        "    mulh s3, t0, t1",
        "    mulhu s4, t0, t1",
        "    mulhsu s5, t0, t1",
        "    add s6, t2, s0",
        "    xor s7, s1, s2",
        "    mul s8, s6, s7",
    ])


@case("load_mul_forward")
def load_mul_forward():
    return finish(header() + [
        "    addi t0, zero, 7",
        "    addi t1, zero, 9",
        "    sw t0, 0(sp)",
        # Direct load-result consumption distinguishes the registered WB/RF
        # boundary from ordinary ALU-to-MUL admission.
        "    lw t2, 0(sp)",
        "    mul s0, t2, t1",
        "    add s1, s0, t0",
        # An independent instruction removes the direct load dependency.
        "    lw s2, 0(sp)",
        "    addi s3, zero, 3",
        "    mul s4, s2, t1",
    ])


@case("lsu_dependency_matrix")
def lsu_dependency_matrix():
    return finish(header() + [
        # Address/data forwarding into stores, direct and separated load-use
        # distances, load-to-store data, and load-to-control admission.
        "    addi t0, sp, 16",
        "    addi t1, zero, 37",
        "    sw t1, 0(t0)",
        "    lw t2, 0(t0)",
        "    add s0, t2, t1",
        "    lw s1, 0(t0)",
        "    addi s2, zero, 1",
        "    xor s3, s1, s2",
        "    lw s4, 0(t0)",
        "    addi s5, zero, 2",
        "    addi s6, zero, 3",
        "    sub s7, s4, s6",
        "    sw s7, 4(t0)",
        "    lw s8, 4(t0)",
        "    sw s8, 8(t0)",
        "    lw s9, 8(t0)",
        "    beq s9, s7, lsu_dependency_ok",
        "    addi s10, zero, 0x177",
        "lsu_dependency_ok:",
        "    addi s11, s9, 1",
    ])


@case("lsu_unaligned_edges")
def lsu_unaligned_edges():
    return finish(header() + [
        # scalar600 does not raise a misaligned-access exception.  Probe its
        # actual byte-lane behavior instead of assuming generic RISC-V trap
        # semantics.  Aligned byte loads make any address rounding visible.
        "    sw zero, 0(sp)",
        "    sw zero, 4(sp)",
        "    li t0, 0x11223344",
        "    sw t0, 1(sp)",
        "    lbu t1, 0(sp)",
        "    lbu t2, 1(sp)",
        "    lbu s0, 2(sp)",
        "    lbu s1, 3(sp)",
        "    lbu s2, 4(sp)",
        "    li s3, 0x0000a1b2",
        "    sh s3, 1(sp)",
        "    lbu s4, 0(sp)",
        "    lbu s5, 1(sp)",
        "    lbu s6, 2(sp)",
        "    lbu s7, 3(sp)",
        "    lh s8, 1(sp)",
        "    lhu s9, 3(sp)",
        "    lw s10, 1(sp)",
    ])


@case("divrem")
def divrem_case():
    return finish(header() + [
        "    addi t0, zero, -101",
        "    addi t1, zero, 7",
        "    div t2, t0, t1",
        "    rem s0, t0, t1",
        "    divu s1, t0, t1",
        "    remu s2, t0, t1",
        "    add s3, t2, s0",
        "    div s4, s3, t1",
        "    div s5, t0, zero",
        "    rem s6, t0, zero",
    ])


@case("div_zero_transitions")
def div_zero_transitions():
    return finish(header() + [
        "    addi t0, zero, -101",
        "    addi t1, zero, 7",
        # Establish standalone architectural divide-by-zero behavior.
        "    rem t2, t0, zero",
        "    addi s0, zero, 1",
        # Exercise the divider state transition that previously made REM/0
        # disagree only when it immediately followed DIV/0.
        "    div s1, t0, zero",
        "    rem s2, t0, zero",
        "    div s3, t0, zero",
        "    addi s4, zero, 2",
        "    rem s5, t0, zero",
        # Normal DIV/REM transitions provide a non-zero-divisor control.
        "    div s6, t0, t1",
        "    rem s7, t0, t1",
    ])


@case("div_alu_forward")
def div_alu_forward():
    return finish(header() + [
        # Match the production dependency shape which exposed stale-RF
        # divider admission: both operands are produced by immediately older
        # ALU instructions and have not reached architectural retirement.
        "    addi s9, zero, 7",
        "    addi s10, zero, 1512",
        "    andi a2, s10, 2047",
        "    andi a0, s9, 2047",
        "    divu s0, a2, a0",
        # Keep a second multi-level forwarded chain and the remainder path in
        # the same case; expected architectural results are s0=216, s1=0.
        "    addi a3, s0, -200",
        "    addi a4, a3, 5",
        "    remu s1, a4, a0",
    ])


def zero_pair(first, second, gap):
    lines = [
        "    addi t0, zero, -37",
        "    addi t1, zero, 5",
        f"    {first} t2, t0, zero",
    ]
    for index in range(gap):
        lines.append(f"    addi s{index}, zero, {index + 1}")
    lines += [
        f"    {second} s4, t0, zero",
        # A normal operation shows whether stale zero-divisor completion is
        # still visible to the next request.
        "    div s5, t0, t1",
        "    rem s6, t0, t1",
    ]
    return finish(header() + lines)


@case("div0_div0")
def div0_div0():
    return zero_pair("div", "div", 0)


@case("div0_rem0")
def div0_rem0():
    return zero_pair("div", "rem", 0)


@case("rem0_div0")
def rem0_div0():
    return zero_pair("rem", "div", 0)


@case("rem0_rem0")
def rem0_rem0():
    return zero_pair("rem", "rem", 0)


@case("div0_rem0_gap1")
def div0_rem0_gap1():
    return zero_pair("div", "rem", 1)


@case("div0_rem0_gap2")
def div0_rem0_gap2():
    return zero_pair("div", "rem", 2)


@case("div0_rem0_gap3")
def div0_rem0_gap3():
    return zero_pair("div", "rem", 3)


@case("branch_transitions")
def branch_transitions():
    return finish(header() + [
        "    addi t0, zero, 1",
        "    addi t1, zero, 1",
        "    bne t0, t1, branch_transition_bad",
        "    addi t2, zero, 1",
        "    beq t0, t1, branch_transition_taken0",
        "branch_transition_bad:",
        "    addi s0, zero, 0x111",
        "branch_transition_taken0:",
        # A taken branch whose target is another taken branch.
        "    beq t0, t1, branch_transition_control",
        "    addi s1, zero, 0x222",
        "branch_transition_control:",
        "    beq t0, t1, branch_transition_alu",
        "    addi s2, zero, 0x333",
        "branch_transition_alu:",
        "    addi t2, t2, 2",
        # Not-taken after taken, followed by a sequential ALU.
        "    bne t0, t1, branch_transition_bad2",
        "    addi t2, t2, 4",
        "    j branch_transition_done",
        "branch_transition_bad2:",
        "    addi s3, zero, 0x444",
        "branch_transition_done:",
        "    addi t2, t2, 8",
    ])


@case("branch_store_transitions")
def branch_store_transitions():
    return finish(header() + [
        "    addi t0, zero, 1",
        "    addi t1, zero, 2",
        # A sequential not-taken branch leaves the following store already
        # in the normal ID stream.
        "    beq t0, t1, branch_store_bad",
        "    sw t0, 0(sp)",
        # A taken branch must redirect/refill before its target store.
        "    beq t0, t0, branch_store_taken",
        "branch_store_bad:",
        "    addi s0, zero, -1",
        "branch_store_taken:",
        "    sw t1, 4(sp)",
        "    lw s1, 0(sp)",
        "    lw s2, 4(sp)",
    ])


@case("branch_load_transitions")
def branch_load_transitions():
    return finish(header() + [
        "    li t0, 0x1234",
        "    sh t0, 0(sp)",
        "    addi t1, zero, 0",
        "    addi t2, zero, 5",
        "    jal zero, branch_load_guard",
        "branch_load_body:",
        # Match the production loop shape: the taken loop-back first lands
        # on two ALU operations, then a repeatedly not-taken guard is
        # immediately followed by a sequential half-word load.
        "    addi t1, t1, 1",
        "    addi s0, s0, 3",
        "branch_load_guard:",
        "    blt t2, t1, branch_load_done",
        "    lhu s1, 0(sp)",
        "    add s2, s2, s1",
        "    blt t1, t2, branch_load_body",
        "branch_load_done:",
        "    add s3, s2, s0",
    ])


@case("branch_load_recurrence")
def branch_load_recurrence():
    return finish(header() + [
        "    li t0, 0x1234",
        "    sh t0, 0(sp)",
        "    addi t1, zero, 0",
        "    addi t2, zero, 5",
        "    li s0, 0x2000",
        # Shift the loop by one 32-bit word so its target, guard and
        # load-dependent loop-back occupy the same low/high halves of the
        # 64-bit ISPM fetch words as the production CCH recurrence.
        "    nop",
        "    jal zero, branch_load_recurrence_guard",
        "branch_load_recurrence_body:",
        "    addi t1, t1, 1",
        "    addi s2, s2, 2",
        "branch_load_recurrence_guard:",
        "    blt t2, t1, branch_load_recurrence_done",
        "    lhu s1, 0(sp)",
        # Keep the loop-back directly dependent on the load, matching the
        # shape that exposed the second-iteration admission boundary in the
        # production CCH task.  The independent transition case above keeps
        # the load->ALU->branch alternative covered as a separate oracle.
        "    blt s1, s0, branch_load_recurrence_body",
        "branch_load_recurrence_done:",
        "    add s3, s2, t1",
    ])


@case("branch_div_transitions")
def branch_div_transitions():
    return finish(header() + [
        "    addi t0, zero, 21",
        "    addi t1, zero, 4",
        # A not-taken branch leaves DIV on the sequential ID stream.  A
        # taken branch and JAL both make DIV the first redirect target.  This
        # distinguishes divider-internal completion from a blanket
        # control-to-DIV penalty.
        "    beq t0, t1, branch_div_bad0",
        "    div s0, t0, t1",
        "    beq t0, t0, branch_div_taken",
        "branch_div_bad0:",
        "    addi s1, zero, 0x111",
        "branch_div_taken:",
        "    rem s2, t0, t1",
        "    jal zero, branch_div_jal_target",
        "    addi s3, zero, 0x222",
        "branch_div_jal_target:",
        "    divu s4, t0, t1",
        "    add s5, s0, s2",
        "    add s6, s5, s4",
    ])


@case("fence_transitions")
def fence_transitions():
    return finish(header() + [
        "    addi t0, zero, 37",
        "    sw t0, 0(sp)",
        "    fence rw, rw",
        "    lw t1, 0(sp)",
        "    add t2, t1, t0",
        # Keep the exact FENCE.I encoding independent of assembler ISA
        # extension defaults.
        "    .4byte 0x0000100f",
        "    addi s0, t2, 1",
    ])


@case("ecall_nop")
def ecall_nop():
    return finish(header() + [
        # scalar600 decodes ECALL into the default no-writeback datapath; it
        # has no trap/privilege unit.  Keep it between dependent ALU ops so
        # both the architectural no-op and its retirement slot are visible.
        "    addi t0, zero, 7",
        "    .4byte 0x00000073",
        "    addi t1, t0, 1",
        "    add t2, t1, t0",
    ])


@case("zero_word_nop")
def zero_word_nop():
    return finish(header() + [
        # scalar600 explicitly decodes opcode 0 as a no-writeback NOP.
        "    addi t0, zero, 7",
        "    .4byte 0x00000000",
        "    addi t1, t0, 1",
        "    add t2, t1, t0",
    ])


@case("illegal_opcode_nop")
def illegal_opcode_nop():
    return finish(header() + [
        # An unmatched opcode retains the decoder's NOP/no-write defaults;
        # inst_illegal is not connected to a scalar600 trap path.
        "    addi t0, zero, 9",
        "    .4byte 0xffffffff",
        "    addi t1, t0, 1",
        "    xor t2, t1, t0",
    ])


@case("unsupported_system_nop")
def unsupported_system_nop():
    return finish(header() + [
        "    addi t0, zero, 11",
        "    addi t1, zero, 17",
        # csrrw t1, mhartid, zero: scalar600 supports only CSRRS reads.
        "    .4byte 0xf1401373",
        "    add t2, t1, t0",
        "    add s0, t2, t1",
    ])


@case("decoder_write_zero")
def decoder_write_zero():
    return finish(header() + [
        "    addi t0, zero, 11",
        "    addi t1, zero, 17",
        # LOAD/funct3=3: scalar600 keeps decode_we but RES_NOP writes zero.
        "    .4byte 0x00013303",
        "    add t2, t1, t0",
        "    addi t1, zero, 19",
        # CSRRS t1, 0x123, zero: unknown CSR has the same write-zero path.
        "    .4byte 0x12302373",
        "    add s0, t1, t2",
    ])


@case("decoder_lax_alu_encodings")
def decoder_lax_alu_encodings():
    return finish(header() + [
        # scalar600 selects these operations from opcode/funct3 and treats
        # funct7 only as an ADD/SUB or SRL/SRA selector.  The encodings are
        # reserved by base RV32I, but exercising them checks the RTL decoder
        # itself rather than silently inheriting gem5's broader ISA decoder.
        "    lui t0, 0x80000",
        "    addi t0, t0, 1",
        "    addi t1, zero, 3",
        "    .4byte 0x40129393",  # funct7=0x20 SLLI t2,t0,1
        "    .4byte 0x0222d413",  # funct7=0x01 SRAI s0,t0,2
        "    .4byte 0x046284b3",  # funct7=0x02 SUB s1,t0,t1
        "    .4byte 0x0462c933",  # funct7=0x02 XOR s2,t0,t1
        "    .4byte 0x0462d9b3",  # funct7=0x02 SRA s3,t0,t1
        "    add s4, t2, s0",
        "    xor s5, s1, s2",
        "    add s6, s3, s4",
    ])


@case("decoder_illegal_subops")
def decoder_illegal_subops():
    return finish(header() + [
        "    addi t0, zero, 41",
        "    sw t0, 0(sp)",
        "    addi t1, zero, 99",
        # Invalid BRANCH/funct3=2 and STORE/funct3=3 retain the decoder's
        # no-write NOP defaults.  They must not redirect or touch memory.
        "    .4byte 0x0062a063",
        "    .4byte 0x00613023",
        # scalar600's LR.W decode ignores rs2; the otherwise reserved rs2=6
        # form must still load the reserved word and participate in RAW.
        "    .4byte 0x106123af",
        "    addi s0, t2, 1",
        "    lw s1, 0(sp)",
        "    add s2, s0, s1",
    ])


@case("decoder_csr_aliases")
def decoder_csr_aliases():
    return finish(header() + [
        "    addi t0, zero, 1",
        # The RTL ignores CSR bit 0 and rs1 for its CSRRS-only read path.
        "    .4byte 0xc002a3f3",  # CSRRS t2,cycle,t0
        "    .4byte 0xc0102473",  # CSRRS s0,0xc01,x0 -> cycle alias
        "    .4byte 0xf15024f3",  # CSRRS s1,0xf15,x0 -> mhartid alias
        "    sub s2, s0, t2",
        "    add s3, s1, s2",
    ])


@case("decoder_lax_jalr")
def decoder_lax_jalr():
    return finish(header() + [
        # scalar600 selects JALR from opcode alone; funct3 is ignored.  Keep
        # the destination even so this case isolates decode/redirect/link
        # behavior from the independently covered odd-target rule.
        "    lui t0, %hi(decoder_lax_jalr_target)",
        "    addi t0, t0, %lo(decoder_lax_jalr_target)",
        "    .4byte 0x0002f367",  # funct3=7 JALR t1,0(t0)
        "    addi t2, zero, 0x111",
        "decoder_lax_jalr_target:",
        "    addi s0, t1, 1",
        "    add s1, s0, t0",
    ])


@case("decoder_false_dependencies")
def decoder_false_dependencies():
    return finish(header() + [
        "    addi t0, zero, 41",
        "    sw t0, 0(sp)",
        # LUI/AUIPC assert decode_rdata1_en in scalar600 even though their
        # result ignores op1.  Bits [19:15] deliberately name the immediately
        # preceding load destination to expose the resulting false RAW hold.
        "    lw t0, 0(sp)",
        "    .4byte 0x00028437",  # LUI s0,0x28; decoder rs1 field is x5
        "    addi s1, s0, 1",
        "    lw t1, 0(sp)",
        "    .4byte 0x00030497",  # AUIPC s1,0x30; decoder rs1 field is x6
        # Invalid branch/store retain their two read enables; invalid LOAD
        # retains rs1 and rd/write-zero.  Each follows a matching load so the
        # oracle covers their otherwise invisible decoder-side interlock.
        "    lw t2, 0(sp)",
        "    .4byte 0x0003a063",  # invalid BRANCH funct3=2, rs1=x7
        "    lw t2, 0(sp)",
        "    .4byte 0x0083b023",  # invalid STORE funct3=3, rs1=x7
        "    lw s2, 0(sp)",
        "    lw t2, 0(sp)",
        "    .4byte 0x0003b903",  # invalid LOAD funct3=3, s2,0(x7)
        "    addi s3, s2, 1",
    ])


@case("decoder_x0_load_hazard")
def decoder_x0_load_hazard():
    return finish(header() + [
        "    addi t0, zero, 23",
        "    sw t0, 0(sp)",
        # scalar600 forms its load-use equality before the `(rs != 0)` EX
        # bypass check.  A load whose rd is x0 therefore still stalls a
        # following decoded x0 read for the registered WB/RF edge.
        "    lw zero, 0(sp)",
        "    addi t1, zero, 1",
        "    lw zero, 0(sp)",
        "    .4byte 0x00000437",  # LUI s0,0; decoder rs1 field is x0
        "    lw zero, 0(sp)",
        "    add t2, zero, t1",
        "    add s1, s0, t2",
    ])


@case("wfi_lax_decode")
def wfi_lax_decode():
    return finish(header() + [
        "    addi t0, zero, 7",
        "    addi t1, zero, 9",
        # WFI is selected from opcode and imm[11:0] only.  funct3=7, rs1=x6
        # and rd=x5 must still enter the same halt/wake path with no write.
        "    .4byte 0x105372f3",
        "    add s0, t0, t1",
        "    addi s1, s0, 1",
    ])


@case("wfi_resume")
def wfi_resume():
    return finish(header() + [
        "    addi t0, zero, 7",
        # The tile-level scheduler/barrier logic wakes a single participating
        # scalar core.  Raw encoding avoids host assembler privilege-policy
        # differences.
        "    .4byte 0x10500073",
        "    addi t1, t0, 1",
        "    mul t2, t1, t0",
    ])


@case("venus_pico_irq_isa")
def venus_pico_irq_isa():
    return finish(header() + [
        # Venus L1 Scheduler's PicoRV32 CUSTOM-0 ABI.  Raw encodings keep
        # this conformance probe independent of host assembler extensions.
        "    lui t0, %hi(pico_retirq_target)",
        "    addi t0, t0, %lo(pico_retirq_target)",
        "    addi t1, zero, 0x35",
        "    .4byte 0x0202a00b",  # setq q0,t0
        "    .4byte 0x0203208b",  # setq q1,t1
        "    .4byte 0x0000438b",  # getq t2,q0
        "    .4byte 0x0000c40b",  # getq s0,q1
        "    .4byte 0x0603e48b",  # maskirq s1,t2
        "    .4byte 0x0604e90b",  # maskirq s2,s1
        "    .4byte 0x0a02e98b",  # timer s3,t0
        "    .4byte 0x0a006a0b",  # timer s4,zero
        "    .4byte 0x0400000b",  # retirq -> q0
        "    addi s5, zero, 0x111",
        "pico_retirq_target:",
        "    addi s6, s0, 1",
        "    xor s7, s6, t1",
    ])


@case("venus_pico_irq_entry")
def venus_pico_irq_entry():
    # Keep the architectural vector separate from the ELF entry point just
    # like the production Scheduler image: SEWorkload enters at _start while
    # an asynchronously posted cause redirects to the configured IRQ vector.
    lines = [
        '    .section .text, "ax", @progbits',
        "    .option norvc",
        "    .org 0x20",
        "pico_irq_vector:",
        "    .4byte 0x0000c30b",  # getq t1,q1 (admitted cause mask)
        "    addi s0, s0, 1",
        "    add s1, s1, t1",
        "    .4byte 0x0400000b",  # retirq -> interrupted q0
        "    .org 0x100",
        "    .global _start",
        "_start:",
        "    li sp, 0x24000",
        "    addi sp, sp, -64",
        "    .global measure_begin",
        "measure_begin:",
        # picorv32 irq_mask uses one=masked. Enable only Scheduler DMA bit 11.
        "    li t0, -2049",
        "    .4byte 0x0602e38b",  # maskirq t2,t0
        "    .4byte 0x0800440b",  # waitirq s0
        # waitirq returns the posted pending mask after the handler returns.
        "    add s2, s0, s1",
        "    addi s3, s2, 1",
    ]
    return finish(lines)


@case("lsu_widths")
def lsu_widths():
    return finish(header() + [
        "    addi t0, zero, -1",
        "    sb t0, 0(sp)",
        "    sh t0, 2(sp)",
        "    sw t0, 4(sp)",
        "    lb t1, 0(sp)",
        "    lbu t2, 0(sp)",
        "    lh s0, 2(sp)",
        "    lhu s1, 2(sp)",
        "    lw s2, 4(sp)",
        "    add s3, t1, t2",
        "    add s4, s0, s1",
        "    xor s5, s2, t0",
    ])


@case("load_use_control")
def load_use_control():
    return finish(header() + [
        "    addi t0, zero, 6",
        "    sw t0, 0(sp)",
        "    lw t1, 0(sp)",
        "    add t2, t1, t0",
        "    lw s0, 0(sp)",
        "    beq s0, t0, load_branch_taken",
        "    addi s1, zero, 0x111",
        "load_branch_taken:",
        "    sw s0, 4(sp)",
        "    lw s2, 4(sp)",
        "    bne s2, t0, load_branch_bad",
        "    addi s3, s2, 1",
        "    j load_branch_done",
        "load_branch_bad:",
        "    addi s4, zero, 0x222",
        "load_branch_done:",
        "    addi s5, s3, 1",
    ])


@case("lrsc_success")
def lrsc_success():
    return finish(header() + [
        "    addi t0, zero, 41",
        "    sw t0, 0(sp)",
        # lr.w t1, (sp); sc.w t2, t0, (sp).  Keep raw encodings so the
        # common rv32im build remains unchanged while probing scalar600's
        # implemented LR/SC subset.
        "    .4byte 0x1001232f",
        "    addi t0, t1, 1",
        "    .4byte 0x185123af",
        "    lw s0, 0(sp)",
        "    add s1, s0, t2",
    ])


@case("lrsc_invalidate")
def lrsc_invalidate():
    return finish(header() + [
        "    addi t0, zero, 11",
        "    sw t0, 0(sp)",
        "    .4byte 0x1001232f",  # lr.w t1, (sp)
        "    addi s0, zero, 23",
        # A normal store to the reserved word must invalidate the monitor.
        "    sw s0, 0(sp)",
        "    addi t0, t1, 1",
        "    .4byte 0x185123af",  # sc.w t2, t0, (sp)
        "    lw s1, 0(sp)",
        "    add s2, s1, t2",
    ])


@case("lrsc_edge_matrix")
def lrsc_edge_matrix():
    return finish(header() + [
        "    addi t0, zero, 31",
        "    sw t0, 0(sp)",
        "    addi t1, zero, 47",
        "    sw t1, 4(sp)",
        # SC without an older LR must fail.
        "    .4byte 0x185123af",  # sc.w t2, t0, (sp)
        # A store to a different word should not destroy a word-granular
        # reservation.  The following SC result exposes the monitor scope.
        "    .4byte 0x1001242f",  # lr.w s0, (sp)
        "    sw t1, 4(sp)",
        "    addi t0, s0, 1",
        "    .4byte 0x185124af",  # sc.w s1, t0, (sp)
        # An SC to a different address must not consume data at the reserved
        # word.  t3 holds sp+4; s2 receives the SC status.
        "    .4byte 0x1001242f",  # lr.w s0, (sp)
        "    addi t3, sp, 4",
        "    .4byte 0x185e292f",  # sc.w s2, t0, (t3)
        # A successful SC consumes the reservation, so the immediate repeat
        # without a new LR must fail.
        "    .4byte 0x1001242f",  # lr.w s0, (sp)
        "    addi t0, s0, 1",
        "    .4byte 0x185129af",  # sc.w s3, t0, (sp)
        "    .4byte 0x18512a2f",  # sc.w s4, t0, (sp)
        "    lw s5, 0(sp)",
        "    lw s6, 4(sp)",
    ])


@case("csr_mhartid")
def csr_mhartid():
    return finish(header() + [
        "    csrr t0, mhartid",
        "    addi t1, t0, 1",
        "    csrr t2, mhartid",
        "    beq t0, t2, csr_mhartid_ok",
        "    addi s0, zero, 0x111",
        "csr_mhartid_ok:",
        "    addi s1, t2, 2",
    ])


@case("csr_counters")
def csr_counters():
    return finish(header() + [
        # scalar600 implements these CSRRS/rs1=x0 forms as snapshots of its
        # ID-stage counters.  In particular, `instret` counts non-stalled,
        # non-flushed ID cycles rather than generic gem5 commit events.
        "    csrr t0, cycle",
        "    csrr t1, instret",
        "    addi t2, zero, 1",
        "    addi t2, t2, 2",
        "    csrr s0, cycle",
        "    csrr s1, instret",
        "    sub s2, s0, t0",
        "    sub s3, s1, t1",
        "    csrr s4, cycleh",
        "    csrr s5, instreth",
        # Consume the snapshots immediately so CSR-result forwarding and
        # architectural writeback are both part of the oracle.
        "    xor s6, s4, s5",
        "    add s7, s2, s3",
    ])


@case("csr_counter_stalls")
def csr_counter_stalls():
    return finish(header() + [
        # Snapshot both counters around each scalar600 pipeline stop/flush
        # class.  Absolute values check reset/admission phase; the paired
        # deltas check that count_instr freezes only where RTL's ID stage
        # holds or flushes while count_cycle continues every clock.
        "    addi a0, zero, 18",
        "    addi a1, zero, 3",
        "    csrr t0, cycle",
        "    csrr t1, instret",
        "    mul t2, a0, a1",
        "    add t2, t2, a1",
        "    csrr s0, cycle",
        "    csrr s1, instret",
        "    div t2, a0, a1",
        "    add t2, t2, a1",
        "    csrr s2, cycle",
        "    csrr s3, instret",
        "    sw a0, 0(sp)",
        "    lw t2, 0(sp)",
        "    add t2, t2, a1",
        "    csrr s4, cycle",
        "    csrr s5, instret",
        "    beq a0, a0, csr_counter_taken",
        "    addi t2, zero, 0x111",
        "csr_counter_taken:",
        "    csrr s6, cycle",
        "    csrr s7, instret",
        "    xor t3, s0, s1",
        "    xor t4, s2, s3",
        "    xor t5, s4, s5",
        "    xor t6, s6, s7",
    ])


@case("mixed_pipeline")
def mixed_pipeline():
    return finish(header() + [
        # Deterministic mixed-FU dependency graph.  It deliberately crosses
        # ALU, LSU, control, MUL and DIV boundaries without task knowledge.
        "    addi t0, zero, 7",
        "    addi t1, zero, 3",
        "    mul t2, t0, t1",
        "    add s0, t2, t1",
        "    sw s0, 0(sp)",
        "    lw s1, 0(sp)",
        "    xor s2, s1, t0",
        "    beq s2, zero, mixed_bad0",
        "    div s3, s2, t1",
        "    rem s4, s2, t1",
        "    add s5, s3, s4",
        "    bne s5, zero, mixed_taken",
        "mixed_bad0:",
        "    addi s6, zero, 0x111",
        "mixed_taken:",
        "    mul s7, s5, t0",
        "    sw s7, 4(sp)",
        "    lw s8, 4(sp)",
        "    addi s9, s8, -1",
    ])


def scalar_case(args):
    lines = CASES[args.case]()
    contract = {
        "schema": "venus-scalar600-cpu-microbench/v1",
        "case": args.case,
        "kind": args.kind,
        "operation": args.operation,
        "measurement": {
            "begin_symbol": "measure_begin",
            "end_symbol": "measure_end",
            "oracle": "ordered PC/class, relative retire cycle, writeback",
        },
        "controlled_variables": {
            "isa": "rv32im",
            "compressed": False,
            "lanes": 16,
            "rows": 128,
        },
    }
    if args.case in ("wfi_resume", "wfi_lax_decode"):
        # The scalar core has no autonomous wake source in this isolated
        # harness.  Record the deterministic scheduler/barrier wake stimulus
        # in the case contract so the complete matrix remains one-command
        # reproducible instead of requiring a hand-written special run.
        contract["external_events"] = {"wfi_wake_tick": 40000}
    elif args.case == "venus_pico_irq_entry":
        contract["gem5_entry"] = "0x100"
        contract["external_events"] = {
            "venus_pico_irq_tick": 40000,
            "venus_pico_irq_cause": "0x00000800",
            "venus_pico_irq_vector": "0x00000020",
        }
    return "\n".join(lines) + "\n", contract


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--case", choices=tuple(CASES), required=True)
    return parser.parse_args()


def main():
    parsed = parse_args()
    build_args = SimpleNamespace(
        output_dir=parsed.output_dir,
        case=parsed.case,
        kind=parsed.case,
        operation="scalar",
        ew=32,
        vl=1,
        streams=1,
        # ECALL itself is part of the scalar600 conformance surface.  Keep
        # termination on the scheduler's EBREAK/drain protocol so an ECALL
        # test cannot be confused with an artificial Linux SE exit syscall.
        gem5_termination="scheduler_ebreak",
    )
    base.build_case(
        build_args,
        case_builder=scalar_case,
        stem_prefix="cpu",
    )


if __name__ == "__main__":
    main()
