include ../config.mk

TARGET_TASKS := $(wildcard *.c)
TARGET_TASKS := $(basename $(TARGET_TASKS))
TARGET_TASKS_NOT_DIR := $(notdir $(basename $(TARGET_TASKS)))
ASM_PATH := ./asm/
VENUS_IR := ./ir/
VENUS_IR_ANALYSIS := ./ir/analysis/
VENUS_BIN := ../bin/
# Define lists of targets for each phase
IR_TARGETS := $(addsuffix .0.ll, $(addprefix $(VENUS_IR), $(TARGET_TASKS_NOT_DIR)))
SPLIT_IR_TARGETS := $(addsuffix .split.ll, $(addprefix $(VENUS_IR), $(TARGET_TASKS_NOT_DIR)))
ASSEMBLY_TARGETS := $(addsuffix .s, $(addprefix $(VENUS_IR),$(TARGET_TASKS_BASE)))
ASSEMBLY_ANALYSIS_TARGETS := $(addsuffix _analysis.s, $(addprefix $(VENUS_IR_ANALYSIS), $(TARGET_TASKS_NOT_DIR)))
O_TARGETS := $(addsuffix .o, $(addprefix $(VENUS_IR), $(TARGET_TASKS_NOT_DIR)))
BIN_TARGETS := $(addsuffix .bin, $(addprefix $(VENUS_IR), $(TARGET_TASKS_NOT_DIR)))
HEX_TARGETS := $(addsuffix .hex, $(addprefix $(VENUS_IR), $(TARGET_TASKS_NOT_DIR)))
DISASSEMBLY_TARGETS := $(addsuffix _asm.txt, $(addprefix $(VENUS_IR), $(TARGET_TASKS_NOT_DIR)))




# Define phony targets
.PHONY: all header ASM input_num OUTBINHEX clean

all: header ASM $(IR_TARGETS) $(SPLIT_IR_TARGETS) $(ASSEMBLY_TARGETS) $(ASSEMBLY_ANALYSIS_TARGETS) $(O_TARGETS) $(BIN_TARGETS) $(HEX_TARGETS) $(DISASSEMBLY_TARGETS) input_num input_type
# all: header ASM $(IR_TARGETS) $(SPLIT_IR_TARGETS) $(ASSEMBLY_TARGETS) $(ASSEMBLY_ANALYSIS_TARGETS) $(O_TARGETS) $(BIN_TARGETS) $(HEX_TARGETS) input_num input_type

ASM: $(ASM_PATH)
	$(PYTHON) asm.py $(mode) $(VENUSROW) $(VENUSLANE) $(TARGET_TASKS_NOT_DIR)

header: venusbuiltin.h venustype.h

venusbuiltin.h:
	$(PYTHON) gen_venusbuiltin_h.py

venustype.h:
	$(PYTHON) gen_venustype_h.py

# Feed C through venus_ext_vector_normalize.py so __vNNNiM matches ext_vector_type(N)
# for the Venus LLVM backend (see venus_ext_vector_normalize.py). Original .c on disk
# is unchanged; scalar_o/type_verify still parse ext_vector_type from sources.
$(VENUS_IR)%.0.ll: %.c
	$(PYTHON) venus_ext_vector_normalize.py $< | $(CC) $(CFLAGS) -mllvm --venus-task-name=$* -iquote . -x c - -o $@

$(VENUS_IR)%.split.ll: $(VENUS_IR)%.0.ll
	$(OPT) $(VENUSOPTFLAGS) --venus-task-name=$* $< -o $@

$(VENUS_IR)%.s: $(VENUS_IR)%.split.ll
	$(LLC) $(VENUSLLCFLAGS) $< -o $@

$(VENUS_IR_ANALYSIS)%_analysis.s: $(VENUS_IR)%.split.ll	
	$(LLC) $(VENUSLLCFLAGS2) $< -o $@

$(VENUS_IR)%.o:$(VENUS_IR)%.s
	$(CC) -v $(VENUSARCH) -o $@ $(ASM_PATH)$(notdir $*).S $< -ffreestanding -nostdlib --std=gnu99 -Wl,-Bstatic,-T,firmware.ld,-Map,$*.map,--strip-debug -lgcc
	
$(VENUS_IR)%.bin: $(VENUS_IR)%.o
	$(CPY) -O binary $< $@

$(VENUS_IR)%.hex: $(VENUS_IR)%.bin
	$(PYTHON) makehex.py $< 65536 > $@

$(VENUS_IR)%_asm.txt: $(VENUS_IR)%.o
	$(DUMP) $(VENUSDUMPFLAGS) $< > $@
	
map_task:
	$(PYTHON) map_task.py

single_task_test:
	# $(PYTHON) single_task_test.py
	# $(PYTHON) single_without_padding.py

input_type:
	$(PYTHON) type_verify.py

input_num:
	$(PYTHON) input_num.py
	TARGET_DAG="$(TARGET_DAG)" $(PYTHON) scalar_o.py

input_dest_addr:
	$(PYTHON) ../script/input_dest_addr.py $(VENUSLANE) $(VenusInputStructAddr)

# Define BIN_FILES and HEX_FILES
BIN_FILES := $(wildcard $(VENUS_BIN)*.bin)
HEX_FILES := $(patsubst $(VENUS_BIN)%.bin, $(VENUS_BIN)%.hex, $(BIN_FILES))

# Pattern rule to convert .bin to .hex
$(VENUS_BIN)%.hex: $(VENUS_BIN)%.bin
	$(PYTHON) makehex.py $< 65536 > $@

OUTBINHEX: $(HEX_FILES)

clean:
	-$(RM) *.d *.hash *.s *_asm.txt *.bin *.map $(TARGETS) *.json
	$(RM) -rf $(VENUS_IR)/*
	$(RM) -rf $(ASM_PATH)/*

.PRECIOUS: $(VENUS_IR)%.s
