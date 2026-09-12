SOURCE = src
LIB = lib
TASK = task
MAIN_SRC ?= $(SOURCE)/main.c
CFLAGS += -O3
# CFLAGS += -Os 
CFLAGS_SCHEDULER = -O3
QEMUS = qemu

SRCS_ASM = \
		$(SOURCE)/irq_vector.S \
		$(SOURCE)/start.S \
		$(SOURCE)/hal.S \
		$(SOURCE)/mem.S

SRCS_C = \
		$(SOURCE)/kernel.c \
		$(LIB)/uart.c \
		$(SOURCE)/cluster.c \
		$(SOURCE)/api.c \
		$(SOURCE)/fifo.c \
		$(SOURCE)/timer.c \
		$(LIB)/printf.c \
		$(LIB)/malloc.c \
		$(SOURCE)/irq.c \
		$(SOURCE)/dma.c \
		$(SOURCE)/coe.c \
		$(SOURCE)/dfe.c \
		$(SOURCE)/dagproc.c \
		$(SOURCE)/padding.c \
		$(SOURCE)/rfdata.c \
		$(SOURCE)/peripheral_init.c \
		$(SOURCE)/fft.c \
		$(MAIN_SRC) \
		$(LIB)/spi.c \
		$(SOURCE)/flash.c \
		$(SOURCE)/w25q128_driver.c \

SRCS_DAG = $(wildcard ./dags/bin/*.bin)
SRCS_DAG_BIN = $(patsubst ./dags/bin/%.bin,./dags/cbin/%_bin.c,$(SRCS_DAG))
SRCS_DAG_JSON = $(patsubst ./dags/bin/%.bin,./dags/cjson/%_json.c,$(SRCS_DAG))

OBJS_SCHEDULER  = $(SRCS_ASM:.S=.o)
OBJS_SCHEDULER += $(SRCS_C:.c=.o)
OBJS_SCHEDULER += $(SRCS_DAG_BIN:.c=.o)
OBJS_SCHEDULER += $(SRCS_DAG_JSON:.c=.o)



