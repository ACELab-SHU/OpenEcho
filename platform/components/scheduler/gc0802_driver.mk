DRIVER_DIR = ./driver
SOURCE     = ./src
LIB        = ./lib
BSP_DIR    = $(DRIVER_DIR)/bsp
ADC_DIR    = $(DRIVER_DIR)/gc080x_driver
CFLAGS    += -I$(BSP_DIR)/include -I$(ADC_DIR)
CFLAGS_DRIVER    = -Os

DRIVER_C += \
		$(SOURCE)/driver.c \
		$(LIB)/memset.c \
		$(LIB)/stdlib.c \
		$(LIB)/sleep.c
DRIVER_C += $(wildcard $(ADC_DIR)/*.c)
DRIVER_C += $(wildcard $(BSP_DIR)/dw_spi/*.c)
# DRIVER_C += $(wildcard $(BSP_DIR)/xilinx_spi/*.c)



# OBJS += $(DRIVER_C:.c=.o)
OBJS_DRIVER = $(DRIVER_C:.c=.o)