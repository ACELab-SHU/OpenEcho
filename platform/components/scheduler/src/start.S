
.section .text
	.equ KERNAL_STACK, 131072	# stack size for kernel start

	.global	_start
_start:
	# Initialize all registers with 0
	addi	x1, zero, 0
	addi	x2, zero, 0
	addi	x3, zero, 0
	addi	x4, zero, 0
	addi	x5, zero, 0
	addi	x6, zero, 0
	addi	x7, zero, 0
	addi	x8, zero, 0
	addi	x9, zero, 0
	addi	x10, zero, 0
	addi	x11, zero, 0
	addi	x12, zero, 0
	addi	x13, zero, 0
	addi	x14, zero, 0
	addi	x15, zero, 0
	addi	x16, zero, 0
	addi	x17, zero, 0
	addi	x18, zero, 0
	addi	x19, zero, 0
	addi	x20, zero, 0
	addi	x21, zero, 0
	addi	x22, zero, 0
	addi	x23, zero, 0
	addi	x24, zero, 0
	addi	x25, zero, 0
	addi	x26, zero, 0
	addi	x27, zero, 0
	addi	x28, zero, 0
	addi	x29, zero, 0
	addi	x30, zero, 0
	addi	x31, zero, 0

	la	a0, _bss_start
	la	a1, _bss_end
	bgeu	a0, a1, 2f
1:
	sw	zero, (a0)
	addi	a0, a0, 4
	bltu	a0, a1, 1b
2:
	# Setup stacks, the stack grows from bottom to top, so we put the
	# stack pointer to the very end of the stack range.
	# Prologue
	la	sp, _stack_top
	add	sp, sp, t0

	j	start_kernel		# hart 0 jump to c

	.global vcs_stop
vcs_stop:
	li      x20,0x1fff1000
	li      x12,0x24 #"$"
	sw      x12,0x0(x20) #"$"
	li      x12,0x73 #"s"
	sw      x12,0x0(x20) #"s"
	li      x12,0x74 #"t"
	sw      x12,0x0(x20) #"t"
	li      x12,0x6f #"o"
	sw      x12,0x0(x20) #"o"
	li      x12,0x70 #"p"
	sw      x12,0x0(x20) #"p"
	li      x12,0x0 #"\0"
	sw      x12,0x0(x20) #"\0"
	ret

.section .bss.stack

	.globl _stack_start
_stack_start:

	.space	KERNAL_STACK

	.globl _stack_top
_stack_top:

	.end              # End of file


