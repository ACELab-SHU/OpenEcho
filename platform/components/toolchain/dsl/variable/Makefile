include ../config.mk

TARGETS := $(wildcard ../IJ/**/*.c)
TARGETS := $(basename $(TARGETS))

SOURCE_DIRS := $(sort $(dir $(TARGETS)))

all: dag_variable_pass asm link_all

dag_variable_pass:
	python3 ../script/dag_variable_pass.py
	python3 ../script/dag_variable_pass_return.py

asm:
	$(foreach TARGET, $(TARGETS), \
	    $(CC) $(CFLAGS) $(TARGET).c -o $(TARGET).0.ll; \
	    $(OPT) $(VENUSOPTFLAGS) $(TARGET).0.ll -o $(TARGET).split.ll; \
	    $(LLC) $(VENUSLLCFLAGS) $(TARGET).split.ll -o $(TARGET).s; \
	)

link_all:
	@for DIR in $(SOURCE_DIRS); do \
	    echo "Processing directory: $$DIR"; \
	    OBJ_FILES=$$(find $$DIR -type f -name "*.s"); \
	    if [ -z "$$OBJ_FILES" ]; then \
	        echo "No .s files found in $$DIR, skipping."; \
	        continue; \
	    fi; \
	    echo "Collected files: $$OBJ_FILES"; \
	    $(CC) -v $(VENUSARCH) -o $$DIR/variable.o $$OBJ_FILES \
	        -ffreestanding -nostdlib --std=gnu99 \
	        -Wl,-Bstatic,-T,variable.ld,-Map,$$DIR/variable.map,--strip-debug -lgcc; \
	    $(CPY) -O binary $$DIR/variable.o "$$DIR"variable.bin; \
	    $(DUMP) $(VENUSDUMPFLAGS) "$$DIR"variable.o > $$DIR/variable.txt; \
	done

map_variable:
	$(PYTHON) map_variable.py

clean:
	@$(RM) -rf $(foreach DIR, $(SOURCE_DIRS), $(wildcard $(DIR)/*.s) $(wildcard $(DIR)/*.split.ll) $(wildcard $(DIR)/*.o) $(wildcard $(DIR)/*.c) $(wildcard $(DIR)/*.bin) $(wildcard $(DIR)/*.map) $(wildcard $(DIR)/*.0.ll)$(wildcard $(DIR)/*.txt))
	@$(RM) -rf ./map/*.json