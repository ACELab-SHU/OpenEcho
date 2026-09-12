CROSS_COMPILE = /server17/ic/opt-riscv32-glibc-ima/bin/riscv32-unknown-linux-gnu-
# CFLAGS = -I./include -nostdlib -fno-builtin -ffreestanding -march=rv32im_zicsr -mabi=ilp32 -g -Wall -ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS = -I./include -nostdlib -fno-builtin -ffreestanding -march=rv32im_zicsr -mabi=ilp32 -Wall -ffunction-sections -fdata-sections -Wl,--gc-sections
# CFLAGS = -I./include -nostdlib -fno-builtin -ffreestanding -march=rv32im_zicsr -mabi=ilp32 -g -Wall -ffunction-sections -fdata-sections -Wl,--gc-sections
# CFLAGS = -I./include  -nostdlib -fno-builtin -ffreestanding -march=rv32im -mabi=ilp32 -g -Wall -ffunction-sections -fdata-sections -Wl,--gc-sections



GDB = gdb-multiarch
CC = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump
NM = ${CROSS_COMPILE}nm
MAKEFLAGS += --silent
PYTHON = /usr/bin/python3.8
ENABLE_DRIVER = 1
ifeq ($(ENABLE_DRIVER), 1)
CFLAGS += -DENABLE_DRIVER
endif



