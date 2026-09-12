include ./common.mk
include ./scheduler_l1.mk
ifeq ($(ENABLE_DRIVER), 1)
include ./gc0802_driver.mk
endif

CFLAGS += -I$(BSP_DIR)/include -I$(ADC_DIR)
OBJS = $(OBJS_SCHEDULER) $(OBJS_DRIVER)
PDCCH_TV9_INPUT = dags/input/nrPDCCH_tv9.params
PDCCH_TV9_DATA = dags/generated/nrPDCCH_tv9_data.inc
PDCCH_TV9_GOLDEN_DIR = dags/golden/nrPDCCH_tv9
PDCCH_TV9_GOLDEN = dags/generated/nrPDCCH_tv9_golden.inc

all: venus

$(PDCCH_TV9_DATA): $(PDCCH_TV9_INPUT) python/materialize_pdcch_tv9.py
	@$(PYTHON) python/materialize_pdcch_tv9.py --input $< --output $@

$(PDCCH_TV9_GOLDEN): $(PDCCH_TV9_GOLDEN_DIR)/DAGRet_pdcchbits.log $(PDCCH_TV9_GOLDEN_DIR)/DAGRet_dci.log $(PDCCH_TV9_GOLDEN_DIR)/DAGRet_crc_result.log $(PDCCH_TV9_GOLDEN_DIR)/DAGRet_pdsch_config.log $(PDCCH_TV9_GOLDEN_DIR)/DAGRet_pdsch_start_symbol.log $(PDCCH_TV9_GOLDEN_DIR)/DAGRet_pdsch_symbol_length.log python/materialize_pdcch_tv9_golden.py
	@$(PYTHON) python/materialize_pdcch_tv9_golden.py --input-dir $(PDCCH_TV9_GOLDEN_DIR) --output $@

src/main_nrPDCCH_tv9.o: $(PDCCH_TV9_DATA) $(PDCCH_TV9_GOLDEN)

.PHONY: venus-pdcch-tv9
venus-pdcch-tv9: $(PDCCH_TV9_DATA) $(PDCCH_TV9_GOLDEN)
	@$(MAKE) MAIN_SRC=src/main_nrPDCCH_tv9.c venus

# start.o must be the first in dependency!
l1.elf: ${OBJS}
	# @${CC} ${CFLAGS} -T l1.ld -Xlinker -verbose -Wl,-Bstatic,--no-warn-rwx-segments,-Map,l1.map -o l1.elf ${OBJS}  $< -lm -lm_prerequisites -lgcc_without_rv32a
	@${CC} ${CFLAGS} -T l1.ld -Wl,-Bstatic,--no-warn-rwx-segments,-Map,l1.map -o l1.elf ${OBJS}  $< -lm -lm_prerequisites -lgcc_without_rv32a

${OBJS}: dag-compile

$(DAGTASK)/%.o : $(DAGTASK)/%.c
	@${CC} ${CFLAGS} -c -o $@ $<
	@echo "  $< ---> $@" >> echo.txt

$(DRIVER_DIR)/%.o: $(DRIVER_DIR)/%.c
	$(CC) $(CFLAGS) $(CFLAGS_DRIVER) -c -o $@ $<
	@echo "  $< ---> $@ ($(CFLAGS_DRIVER))" >> echo.txt

%.o : %.c
	@${CC} ${CFLAGS} $(CFLAGS_SCHEDULER) -c -o $@ $<
	@echo "  $< ---> $@ ($(CFLAGS_SCHEDULER))" >> echo.txt

%.o : %.S
	@${CC} ${CFLAGS} $(CFLAGS_SCHEDULER) -c -o $@ $<
	@echo "  $< ---> $@ (-O3)" >> echo.txt

.PHNOY: dag-compile
dag-compile:
	@cd dags && ${MAKE} create_cbin
	@$(PYTHON) python/modify_daginfo.py
	@echo ""
	@echo "  =============================================================" > echo.txt
	@echo -e "\033[33m  ============== COMPILE L1 SCHEDULER =========================\033[0m" >> echo.txt
	@echo "  =============================================================" >> echo.txt
	@echo ""

.PHONY : venus
venus: clean l1.elf
	@${OBJCOPY} -O binary l1.elf l1.bin
	@${OBJDUMP} --disassemble-all l1.elf > l1_objdump.txt
	@hexdump l1.bin > l1_bin.txt
	@echo "  Make venus done"

.PHONY : move
move: venus
	@cp l1.bin ../0731_venus/software/scheduler/
	@echo "  =============================================================" >> echo.txt
	@echo "  Copied l1.bin to venus_soc"

.PHONY : clean
clean:
	rm -rf *.o *.bin *.elf *.txt *.out *.map *.zip *_bin.c *.log
	@rm -rf $(ADC_DIR)/*.d $(ADC_DIR)/*.o
	@rm -rf $(BSP_DIR)/dw_spi/*.d $(BSP_DIR)/dw_spi/*.o
	# @rm -rf $(BSP_DIR)/xilinx_spi/*.d $(BSP_DIR)/xilinx_spi/*.o
	@cd src && rm -rf *.o *.bin *.elf *.txt *_bin.c
	@cd lib && rm -rf *.o *.bin *.elf *.txt *_bin.c
	@cd dags && ${MAKE} clean
	@$(PYTHON) python/rollback_daginfo.py

.PHONY : size
size:
	${CROSS_COMPILE}nm -n l1.elf > l1_nm.log
	${CROSS_COMPILE}readelf -s l1.elf > l1_readelf.log
	${CROSS_COMPILE}size l1.elf > l1_size.log
