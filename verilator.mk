#***************************************************************************************
# Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
# Copyright (c) 2020-2021 Peng Cheng Laboratory
#
# DiffTest is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************

VERILATOR_BUILD_DIR = $(BUILD_DIR)/verilator-compile
VERILATOR_TARGET = $(VERILATOR_BUILD_DIR)/$(EMU_ELF_NAME)

########## Verilator Configuration Options ##########
VERILATOR_FLAGS = $(SIM_VFLAGS)

VERILATOR_CSRC_DIR = $(abspath ./src/test/csrc/verilator)
VERILATOR_CXXFILES = $(EMU_CXXFILES) $(shell find $(VERILATOR_CSRC_DIR) -name "*.cpp")
VERILATOR_CXXFLAGS = $(EMU_CXXFLAGS) -I$(VERILATOR_CSRC_DIR) -DVERILATOR --std=c++17
VERILATOR_LDFLAGS  = $(SIM_LDFLAGS) -ldl

# Verilator binary
VERILATOR ?= verilator

# Verilator version check
VERILATOR_VER_CMD = $(VERILATOR) --version 2> /dev/null | cut -f2 -d' ' | tr -d '.'
VERILATOR_4_210 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 4210 2> /dev/null)
ifeq ($(VERILATOR_4_210),1)
VERILATOR_CXXFLAGS += -DVERILATOR_4_210
VERILATOR_FLAGS += --instr-count-dpi 1
endif
VERILATOR_5_000 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 5000 2> /dev/null)
ifeq ($(VERILATOR_5_000),1)
VERILATOR_FLAGS += --no-timing +define+VERILATOR_5
else
VERILATOR_FLAGS += +define+VERILATOR_LEGACY
endif
VERILATOR_5_024 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 5024 2> /dev/null)
ifeq ($(VERILATOR_5_024),1)
VERILATOR_FLAGS += --quiet-stats
endif

ifneq (,$(filter $(EMU_TRACE),1 vcd VCD))
VERILATOR_FLAGS += --trace
endif
ifneq (,$(filter $(EMU_TRACE),fst FST))
VERILATOR_FLAGS += --trace-fst
VERILATOR_CXXFLAGS += -DENABLE_FST
endif

# Verilator trace underscore support
ifeq ($(EMU_TRACE_ALL),1)
VERILATOR_FLAGS += --trace-underscore
endif

ifneq ($(EMU_THREADS),0)
VERILATOR_FLAGS += --threads $(EMU_THREADS) --threads-dpi all
endif

ifeq ($(EMU_SNAPSHOT),1)
VERILATOR_FLAGS += --savable
endif

ifeq ($(EMU_COVERAGE),1)
VERILATOR_FLAGS += --coverage-line --coverage-toggle
endif

# C optimization
OPT_FAST ?= -O3

########## Verilator Build Recipes ##########
VERILATOR_FLAGS_ALL =               \
  --exe $(EMU_OPTIMIZE)             \
  --cc --top-module $(EMU_TOP)      \
  +define+VERILATOR=1               \
  +define+PRINTF_COND=1             \
  +define+RANDOMIZE_REG_INIT        \
  +define+RANDOMIZE_MEM_INIT        \
  +define+RANDOMIZE_GARBAGE_ASSIGN  \
  +define+RANDOMIZE_DELAY=0         \
  -Wno-STMTDLY -Wno-WIDTH           \
  --max-num-width 150000            \
  --assert --x-assign unique        \
  --output-split 30000              \
  --output-split-cfuncs 30000       \
  -I$(RTL_DIR)                      \
  -I$(GEN_VSRC_DIR)                 \
  -CFLAGS "$(VERILATOR_CXXFLAGS)"   \
  -LDFLAGS "$(VERILATOR_LDFLAGS)"   \
  -CFLAGS "\$$(PGO_CFLAGS)"         \
  -LDFLAGS "\$$(PGO_LDFLAGS)"       \
  -o $(VERILATOR_TARGET)            \
  $(VERILATOR_FLAGS)

VERILATOR_MK = $(VERILATOR_BUILD_DIR)/V$(EMU_TOP).mk
VERILATOR_HEADERS := $(EMU_HEADERS) $(shell find $(VERILATOR_CSRC_DIR) -name "*.h")

$(VERILATOR_MK): $(SIM_TOP_V) | $(SIM_VSRC) $(VERILATOR_CXXFILES)
ifeq ($(EMU_COVERAGE),1)
	@python3 ./scripts/coverage/vtransform.py $(RTL_DIR)
endif
	@mkdir -p $(@D)
	@echo -e "\n[verilator] Generating C++ files..." >> $(TIMELOG)
	@date -R | tee -a $(TIMELOG)
	$(TIME_CMD) $(VERILATOR) $(VERILATOR_FLAGS_ALL) --Mdir $(@D) $^ $(SIM_VSRC) $(VERILATOR_CXXFILES)
	@sed -i -e 's/$(subst /,\/,$(NOOP_HOME))/$$(NOOP_HOME)/g' \
	       -e '/^default:/i\NOOP_HOME ?= $(subst /,\/,$(NOOP_HOME))\n' $@
ifneq ($(VERILATOR_5_000),1)
	@sed -i 's/private/public/g' $(VERILATOR_BUILD_DIR)/VSimTop.h
	@sed -i 's/const vlSymsp/vlSymsp/g' $(VERILATOR_BUILD_DIR)/VSimTop.h
	@sed -i 's/VlThreadPool\* const/VlThreadPool*/g' $(VERILATOR_BUILD_DIR)/VSimTop__Syms.h
endif

EMU_COMPILE_FILTER =
# 2> $(BUILD_DIR)/g++.err.log | tee $(BUILD_DIR)/g++.out.log | grep 'g++' | awk '{print "Compiling/Generating", $$NF}'

verilator-build-emu:
ifeq ($(REMOTE),localhost)
	@sync -d $(BUILD_DIR) -d $(VERILATOR_BUILD_DIR)
	$(TIME_CMD) $(MAKE) -s VM_PARALLEL_BUILDS=1 OPT_SLOW="-O0" \
						OPT_FAST=$(OPT_FAST) \
						PGO_CFLAGS="$(PGO_CFLAGS)" \
						PGO_LDFLAGS="$(PGO_LDFLAGS)" \
						-C $(VERILATOR_BUILD_DIR) -f $(VERILATOR_MK) $(EMU_COMPILE_FILTER)
	@sync -d $(BUILD_DIR) -d $(VERILATOR_BUILD_DIR)
else
	ssh -tt $(REMOTE) 'export NOOP_HOME=$(NOOP_HOME); \
					   $(MAKE) -C $(NOOP_HOME)/difftest verilator-build-emu \
					   -j `nproc` \
					   OPT_FAST="'"$(OPT_FAST)"'" \
					   PGO_CFLAGS="'"$(PGO_CFLAGS)"'" \
					   PGO_LDFLAGS="'"$(PGO_LDFLAGS)"'"'
endif

# Profile Guided Optimization
PGO_MAX_CYCLE = 2000000   # Default for Verilator; cmdline can override

$(VERILATOR_TARGET): $(VERILATOR_MK) $(SIM_VSRC) $(VERILATOR_CXXFILES) $(VERILATOR_HEADERS)
	@echo -e "\n[c++] Compiling C++ files..." >> $(TIMELOG)
	@date -R | tee -a $(TIMELOG)
ifdef PGO_WORKLOAD
	$(MAKE) pgo-build \
		PGO_CLEAN_OBJ=verilator-clean-obj \
		PGO_BUILD_TARGET=verilator-build-emu \
		PGO_TARGET=$(VERILATOR_TARGET) \
		PGO_EMU_DIR=$(VERILATOR_BUILD_DIR) \
		PGO_MAX_CYCLE=$(PGO_MAX_CYCLE)
else
	@$(MAKE) verilator-build-emu
endif # ifdef PGO_WORKLOAD

verilator-emu: $(VERILATOR_TARGET)
verilator-emu-mk: $(VERILATOR_MK)

COVERAGE_DATA ?= $(shell find $(BUILD_DIR) -maxdepth 1 -name "*.dat")
COVERAGE_DIR  ?= $(DESIGN_DIR)/$(basename $(notdir $(COVERAGE_DATA)))
coverage:
	@verilator_coverage --annotate $(COVERAGE_DIR) --annotate-min 1 $(COVERAGE_DATA)
	@python3 scripts/coverage/coverage.py $(COVERAGE_DIR)
	@python3 scripts/coverage/statistics.py $(COVERAGE_DIR) > $(COVERAGE_DIR)/coverage_$(SIM_TOP).log
	@mv $(COVERAGE_DATA) $(COVERAGE_DIR)

verilator-clean-obj:
	rm -f $(VERILATOR_BUILD_DIR)/*.o $(VERILATOR_BUILD_DIR)/*.gch $(VERILATOR_BUILD_DIR)/*.a $(VERILATOR_TARGET)

.PHONY: verilator-build-emu verilator-clean-obj
