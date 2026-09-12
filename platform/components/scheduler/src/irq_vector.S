#include "irq_instr.S"

	.global irq_handler
	
.section .startup
reset_vec:
	j _start

.section .startirq
irq_vec:
	picorv32_setq_insn(q2, x1)
	picorv32_setq_insn(q3, x2)   # 把两个通用寄存器的值暂存起来(按理来说任意的两个GPR都可以)

  /*
   * 上面两步操作是因为普通的指令sw addi等只能通过操作通用寄存器实现，
   * 后续的中断处理操作若想进行基本的指令操作，
   * 会破坏原有的上下文环境，
   * 需要先将要使用的临时通用寄存器暂存起来。
   * x1被用来标记irq_regs作为中断内存的起始地址
   * x2被用来作为临时寄存器传递数据
   */


	lui x1, %hi(irq_regs)
	addi x1, x1, %lo(irq_regs)  # 将irq_regs放在x1里

	picorv32_getq_insn(x2, q0)
	sw x2,   0(x1)              # 存储中断发生地址
	picorv32_getq_insn(x2, q2)
	sw x2,   4(x1)              # 存储原来的x1寄存器

	picorv32_getq_insn(x2, q3)  # 恢复x2寄存器原来的值
	sw x2,   8(x1)

	# Only save registers in IRQ wrapper that are to be saved by the caller in
	# the RISC-V ABI, with the excpetion of the stack pointer. The IRQ handler
	# will save the rest if necessary. I.e. skip x3, x4, x8, x9, and x18-x27.
	
	sw x5,   20(x1)
	sw x6,   24(x1)
	sw x7,   28(x1)
	sw x10,  40(x1)
	sw x11,  44(x1)
	sw x12,  48(x1)
	sw x13,  52(x1)
	sw x14,  56(x1)
	sw x15,  60(x1)
	sw x16,  64(x1)
	sw x17,  68(x1)
	sw x28, 112(x1)
	sw x29, 116(x1)
	sw x30, 120(x1)
	sw x31, 124(x1)             # 存储所有的中断发生之前的上下文

	# 后面研究一下sp到底要不要存，因为目前我不打算专门给irq handler开个栈

	// void irq_handler(uint32_t *regs, uint32_t cause)

	lui a0, %hi(irq_regs)
	addi a0, a0, %lo(irq_regs)  # a0装入函数第一个形参的实参，里面放入irq_regs的地址

	picorv32_getq_insn(a1, q1)  # a1装入bitmask

	jal ra, irq_handler         # 参数指定完毕后调用irq_handler(定义在c中的中断处理函数)

	addi x1, a0, 0
	
	lw x2,   0(x1)
	picorv32_setq_insn(q0, x2)  # 恢复q0 (可能在irq_handler里会修改中断返回值)

	lw x2,   4(x1)
	picorv32_setq_insn(q2, x2)  # 把原来x1的值暂存到q2里面 (因为目前x1还需要被用作地址偏移寻址，不能直接恢复)

	lw x2,   8(x1)
	picorv32_setq_insn(q3, x2)  # 把原来x2的值暂存到q3里面 (因为目前x2还需要被用作临时寄存器，不能直接恢复)

	lw x5,   20(x1)
	lw x6,   24(x1)
	lw x7,   28(x1)
	lw x10,  40(x1)
	lw x11,  44(x1)
	lw x12,  48(x1)
	lw x13,  52(x1)
	lw x14,  56(x1)
	lw x15,  60(x1)
	lw x16,  64(x1)
	lw x17,  68(x1)
	lw x28, 112(x1)
	lw x29, 116(x1)
	lw x30, 120(x1)
	lw x31, 124(x1)

	picorv32_getq_insn(x1, q2)
	picorv32_getq_insn(x2, q3)  # 恢复x1和x2寄存器的值

	picorv32_retirq_insn()      # 中断返回

irq_regs:                     # 定义了一个标签，标注其在内存中的位置，这里将保存中断处理程序的寄存器状态
  /* .fill repeat, size, value 用size个字节填充value填充repeat次 */
	.fill 32,4                  # 用4-byte的0填充32次，留够32个寄存器的空间


