#include "irq_instr.S"

.section .text
	.global EN_Interrupts
	.global DIS_Interrupts
	.global EN_All_Cluster_Interrupts
	.global Mask_irq
	.global Wait_irq
	.global Forloop_timer
	.global Set_timer
#--------------------+-------+
# RESERVED           |   31  |
# ...                |   ... |
# L1_DMA_ERROR       |   12  |
# L1_DMA_DONE        |   11  |
# CLUSTER_7          |   10  |
# CLUSTER_6          |   9   |
# CLUSTER_5          |   8   |
# CLUSTER_4          |   7   |
# CLUSTER_3          |   6   |
# CLUSTER_2          |   5   |
# CLUSTER_1          |   4   |
# CLUSTER_0          |   3   |
# PICO_MEMERROR      |   2   |
# PICO_BADINSTR      |   1   |
# PICO_TIMER         |   0   |
#--------------------+-------+
  # enable responds to all external interrupt
	EN_All_Cluster_Interrupts:
		li t0, 0
		picorv32_maskirq_insn(t0, t0)
		ret

	# maskirq: 0 -> enable 1 -> disable
	EN_Interrupts:
		not t0, zero
		picorv32_maskirq_insn(t0, t0) # load interrupts from register
		not a0, a0
		and t0, t0, a0
		picorv32_maskirq_insn(zero, t0) # write back new value
		mv a0, t0
		ret

	# '1' means to diable the interrupt
	DIS_Interrupts:
		not t0, zero
		picorv32_maskirq_insn(t0, t0)
		or t0, t0, a0
		picorv32_maskirq_insn(zero, t0)
		mv a0, t0
		ret

	Wait_irq:
		picorv32_waitirq_insn(zero)

	# input - new irq mask | output - formal irq mask
	Mask_irq:
		mv a4, a0
		picorv32_maskirq_insn(a5, a4)
		mv a0, a5
		ret

	Forloop_timer:
		mv t1, a0
	loop:
		beqz t1, end
		addi t1, t1, -1
		j loop
	end:
		ret

	Set_timer:
		mv a4, a0
		picorv32_timer_insn(zero, a4)
		ret



