#!/usr/bin/env python3
"""Executable bit-level model of hardware/spiritrv32/scalar600_div.v.

This is a diagnostic oracle for integrating the divider into MinorCPU.  It
models Verilog non-blocking edge semantics; it deliberately preserves the
RTL's history-dependent divide-by-zero result register rather than replacing
it with architectural RV32IM behavior.
"""

from dataclasses import dataclass


MASK32 = (1 << 32) - 1
MASK33 = (1 << 33) - 1


@dataclass
class Inputs:
    en: bool = False
    signed: bool = False
    rem: bool = False
    op1: int = 0
    op2: int = 0


class Scalar600Div:
    def __init__(self):
        self.en_reg = False
        self.op1_reg = 0
        self.op2_reg = 0
        self.signed_buffer = False
        self.op1_31_buffer = False
        self.op2_31_buffer = False
        self.unsigned_div_result = 0
        self.unsigned_rem_result = 0
        self.temp_rem = 0
        self.count = 32
        self.result_reg = 0
        self.div_or_rem_reg = False

    @staticmethod
    def _neg32(value):
        return ((~value) + 1) & MASK32

    def _comb(self):
        complete = self.count == 0xFF
        complete_delay = self.count == 0xF0
        real_complete = complete or complete_delay
        real_signed = self.signed_buffer if real_complete else False
        # The caller supplies current div_signed separately for the non-final
        # path; step() patches these three values before using this helper.
        return complete, complete_delay, real_complete, real_signed

    def _result_wire(self, current_signed):
        complete = self.count == 0xFF
        complete_delay = self.count == 0xF0
        real_complete = complete or complete_delay
        real_signed = self.signed_buffer if real_complete else current_signed
        real_op1_31 = self.op1_31_buffer if real_complete else bool(self.op1_reg >> 31)
        real_op2_31 = self.op2_31_buffer if real_complete else bool(self.op2_reg >> 31)
        quotient = self.unsigned_div_result & MASK33
        remainder = self.unsigned_rem_result & MASK33
        if real_signed and real_op1_31 != real_op2_31:
            quotient = (~(quotient - 1)) & MASK33
        if real_signed and real_op1_31:
            remainder = (~(remainder - 1)) & MASK33
        return (remainder if self.div_or_rem_reg else quotient) & MASK32

    def step(self, inputs):
        old_en = self.en_reg
        old_op1 = self.op1_reg
        old_op2 = self.op2_reg
        old_count = self.count
        old_complete = old_count == 0xFF
        old_complete_delay = old_count == 0xF0
        old_real_complete = old_complete or old_complete_delay
        old_result_wire = self._result_wire(inputs.signed)

        real_signed = self.signed_buffer if old_real_complete else inputs.signed
        magnitude1 = self._neg32(old_op1) if real_signed and old_op1 >> 31 else old_op1
        magnitude2 = self._neg32(old_op2) if real_signed and old_op2 >> 31 else old_op2
        unsigned_op1 = magnitude1 & MASK32
        unsigned_op2 = magnitude2 & MASK32

        new_count = old_count
        new_temp_rem = self.temp_rem
        new_div = self.unsigned_div_result
        new_rem = self.unsigned_rem_result
        if not old_en or old_complete_delay:
            new_count = 32
            new_temp_rem = 0
        elif old_op2 == 0:
            new_div = 1 if real_signed and old_op1 >> 31 else MASK32
            new_rem = unsigned_op1
            new_count = 0xFF
        elif not (old_count & 0x80):
            bit = (unsigned_op1 >> old_count) & 1
            temp_result = ((self.temp_rem & MASK32) << 1) | bit
            temp_div = (temp_result - unsigned_op2) & MASK33
            if temp_div & (1 << 32):
                new_div = ((self.unsigned_div_result & MASK32) << 1) & MASK33
                new_temp_rem = temp_result
            else:
                new_div = (((self.unsigned_div_result & MASK32) << 1) | 1) & MASK33
                new_temp_rem = temp_div
            new_count = (old_count - 1) & 0xFF
        else:
            new_rem = self.temp_rem
            new_count = 0xF0

        if old_en:
            new_signed_buffer = inputs.signed
            new_op1_31_buffer = bool(old_op1 >> 31)
            new_op2_31_buffer = bool(old_op2 >> 31)
        else:
            new_signed_buffer = self.signed_buffer
            new_op1_31_buffer = self.op1_31_buffer
            new_op2_31_buffer = self.op2_31_buffer

        self.en_reg = inputs.en
        self.op1_reg = inputs.op1 & MASK32
        self.op2_reg = inputs.op2 & MASK32
        self.signed_buffer = new_signed_buffer
        self.op1_31_buffer = new_op1_31_buffer
        self.op2_31_buffer = new_op2_31_buffer
        self.unsigned_div_result = new_div
        self.unsigned_rem_result = new_rem
        self.temp_rem = new_temp_rem
        self.count = new_count
        if old_real_complete:
            self.result_reg = old_result_wire
        self.div_or_rem_reg = inputs.rem

        complete = self.count == 0xFF
        complete_delay = self.count == 0xF0
        result = self._result_wire(inputs.signed) if complete_delay else self.result_reg
        return complete, result & MASK32


def run_instruction_stream(stream):
    """Run the controller contract around the divider.

    ``None`` is one non-divider ID/EX edge.  A divider request remains on the
    bus while ``en && !complete`` stalls scalar600; it retires on the first
    edge that starts with ``complete`` asserted.  Returns
    ``(retire_cycle, result)`` for divider entries.
    """
    dut = Scalar600Div()
    cycle = 0
    retired = []
    for request in stream:
        if request is None:
            cycle += 1
            dut.step(Inputs())
            continue
        while True:
            complete_before_edge = dut.count == 0xFF
            cycle += 1
            _, result = dut.step(request)
            if complete_before_edge:
                retired.append((cycle, result))
                break
    return retired


if __name__ == "__main__":
    op1 = -37
    print("div0/div0/div/rem", [
        (cycle, f"0x{value:08x}") for cycle, value in
        run_instruction_stream([
            Inputs(True, True, False, op1, 0),
            Inputs(True, True, False, op1, 0),
            Inputs(True, True, False, op1, 5),
            Inputs(True, True, True, op1, 5),
        ])
    ])
    print("div0/alu/rem0/div/rem", [
        (cycle, f"0x{value:08x}") for cycle, value in
        run_instruction_stream([
            Inputs(True, True, False, op1, 0), None,
            Inputs(True, True, True, op1, 0),
            Inputs(True, True, False, op1, 5),
            Inputs(True, True, True, op1, 5),
        ])
    ])
