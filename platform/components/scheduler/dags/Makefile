include ../common.mk

BIN_FILES := $(wildcard ./bin/*.bin)
JSON_FILES := $(wildcard ./json/*.json)
DAG_INPUT_TYPE_BITS ?= 3

.PHONY: create_cbin
create_cbin:
	@echo "============== COMPILE DAGS CFILE ==================="
	@echo ""
	$(PYTHON) read_dag_bin.py $(BIN_FILES)
	$(PYTHON) read_dag_json.py --task-input-type-bits $(DAG_INPUT_TYPE_BITS) $(JSON_FILES)

.PHONY: clean
clean:
	rm -rf ./cbin/*_bin.*
	rm -rf ./cjson/*_json.*
