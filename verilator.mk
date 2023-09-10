#***************************************************************************************
# Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

EMU          = $(BUILD_DIR)/emu
EMU_TOP      = SimTop

EMU_CSRC_DIR   = $(abspath ./src/test/csrc/verilator)
EMU_CONFIG_DIR = $(abspath ./config)

EMU_CXXFILES   = $(shell find $(EMU_CSRC_DIR) -name "*.cpp")
EMU_CXXFILES  += $(SIM_CXXFILES) $(GEN_CXXFILES)

EMU_CXXFLAGS += -I$(SIM_CSRC_DIR) -I$(GEN_CSRC_DIR) -I$(PLUGIN_INC_DIR)
EMU_CXXFLAGS += -I$(EMU_CONFIG_DIR) -I$(EMU_CSRC_DIR)
EMU_CXXFLAGS += -DVERILATOR -DNUM_CORES=$(NUM_CORES) --std=c++17
EMU_LDFLAGS  += -ldl

# DiffTest support
ifeq ($(NO_DIFF), 1)
EMU_CXXFLAGS += -DCONFIG_NO_DIFFTEST
else
EMU_CXXFILES += $(DIFFTEST_CXXFILES)
EMU_CXXFLAGS += -I$(DIFFTEST_CSRC_DIR)
VEXTRA_FLAGS += +define+DIFFTEST
endif

# Check if XFUZZ is set
ifeq ($(XFUZZ), 1)
XFUZZ_HOME_VAR = XFUZZ_HOME
ifeq ($(origin $(XFUZZ_HOME_VAR)), undefined)
$(error $(XFUZZ_HOME_VAR) is not set)
endif
FUZZER_LIB   = $(shell echo $$$(XFUZZ_HOME_VAR))/target/release/libfuzzer.a
EMU_LDFLAGS += -lrt -lpthread
endif

# Link fuzzer libraries
ifneq ($(FUZZER_LIB), )
# the target is named as fuzzer for clarity
EMU           = $(BUILD_DIR)/fuzzer
EMU_CXXFLAGS += -DFUZZER_LIB
EMU_LDFLAGS  += $(abspath $(FUZZER_LIB))
FUZZING       = 1
endif

# ChiselDB
WITH_CHISELDB ?= 1
ifeq ($(WITH_CHISELDB), 1)
EMU_CXXFILES += $(BUILD_DIR)/chisel_db.cpp
EMU_CXXFLAGS += -I$(BUILD_DIR) -DENABLE_CHISEL_DB
EMU_LDFLAGS  += -lsqlite3
endif

WITH_CONSTANTIN ?= 1
ifeq ($(WITH_CONSTANTIN), 1)
EMU_CXXFILES += $(BUILD_DIR)/constantin.cpp
EMU_CXXFLAGS += -I$(BUILD_DIR) -DENABLE_CONSTANTIN
endif

ifeq ($(WITH_IPC), 1)
EMU_CXXFLAGS += -I$(BUILD_DIR) -DENABLE_IPC
endif

# CCACHE
CCACHE := $(if $(shell which ccache),ccache,)
ifneq ($(CCACHE),)
export OBJCACHE = ccache
endif

# REF SELECTION
ifneq ($(REF),)
ifneq ($(wildcard $(REF)),)
EMU_CXXFLAGS += -DREF_PROXY=LinkedProxy -DLINKED_REFPROXY_LIB=\\\"$(REF)\\\"
EMU_LDFLAGS  += $(REF)
else
EMU_CXXFLAGS += -DREF_PROXY=$(REF)Proxy
REF_HOME_VAR = $(shell echo $(REF)_HOME | tr a-z A-Z)
ifneq ($(origin $(REF_HOME_VAR)), undefined)
EMU_CXXFLAGS += -DREF_HOME=\\\"$(shell echo $$$(REF_HOME_VAR))\\\"
endif
endif
endif

# Verilator
USE_VERILATOR ?= 1
ifeq ($(USE_VERILATOR),1)
EMU_CXXFLAGS += -I$(EMU_CSRC_DIR)/verilator
endif

# Verilator version check
VERILATOR_VER_CMD = verilator --version | cut -f2 -d' ' | tr -d '.'
VERILATOR_4_210 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 4210)
ifeq ($(VERILATOR_4_210),1)
EMU_CXXFLAGS += -DVERILATOR_4_210
VEXTRA_FLAGS += --instr-count-dpi 1
endif
VERILATOR_5_000 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 5000)
ifeq ($(VERILATOR_5_000),1)
VEXTRA_FLAGS += --no-timing
endif

# Verilator trace support
EMU_TRACE ?=
ifneq (,$(filter $(EMU_TRACE),1 vcd VCD))
VEXTRA_FLAGS += --trace
endif
ifneq (,$(filter $(EMU_TRACE),fst FST))
VEXTRA_FLAGS += --trace-fst
EMU_CXXFLAGS += -DENABLE_FST
endif

# Verilator trace underscore support
ifeq ($(EMU_TRACE_ALL),1)
VEXTRA_FLAGS += --trace-underscore
endif

# Verilator multi-thread support
EMU_THREADS  ?= 0
ifneq ($(EMU_THREADS),0)
VEXTRA_FLAGS += --threads $(EMU_THREADS) --threads-dpi all
EMU_CXXFLAGS += -DEMU_THREAD=$(EMU_THREADS)
endif

# Verilator savable
EMU_SNAPSHOT ?=
ifeq ($(EMU_SNAPSHOT),1)
VEXTRA_FLAGS += --savable
EMU_CXXFLAGS += -DVM_SAVABLE
endif

# Verilator coverage
EMU_COVERAGE ?=
ifeq ($(EMU_COVERAGE),1)
VEXTRA_FLAGS += --coverage-line --coverage-toggle
endif

# co-simulation with DRAMsim3
ifeq ($(WITH_DRAMSIM3),1)
EMU_CXXFLAGS += -I$(DRAMSIM3_HOME)/src
EMU_CXXFLAGS += -DWITH_DRAMSIM3 -DDRAMSIM3_CONFIG=\\\"$(DRAMSIM3_HOME)/configs/XiangShan.ini\\\" -DDRAMSIM3_OUTDIR=\\\"$(BUILD_DIR)\\\"
EMU_LDFLAGS  += $(DRAMSIM3_HOME)/build/libdramsim3.a
endif

ifeq ($(RELEASE),1)
EMU_CXXFLAGS += -DBASIC_DIFFTEST_ONLY
endif

# VGA support
ifeq ($(SHOW_SCREEN),1)
EMU_CXXFLAGS += $(shell sdl2-config --cflags) -DSHOW_SCREEN
EMU_LDFLAGS  += -lSDL2
endif

# GZ image support
EMU_GZ_COMPRESS ?= 1
ifeq ($(EMU_GZ_COMPRESS),0)
EMU_CXXFLAGS += -DNO_GZ_COMPRESSION
else
EMU_LDFLAGS  += -lz
endif

# Verilator
WITH_SPIKE_DASM ?= 1
ifeq ($(WITH_SPIKE_DASM),1)
EMU_CXXFLAGS += -I$(PLUGIN_DASM_DIR)
EMU_CXXFILES += $(PLUGIN_DASM_CXXFILES)
endif

# runahead support
ifeq ($(WITH_RUNAHEAD),1)
EMU_CXXFLAGS += -I$(PLUGIN_RUNAHEAD_DIR)
EMU_CXXFILES += $(PLUGIN_RUNAHEAD_CXXFILES)
endif

# Fuzzer support
ifeq ($(FUZZING),1)
EMU_CXXFLAGS += -DFUZZING
endif

# FIRRTL Coverage support
ifneq ($(FIRRTL_COVER),)
EMU_CXXFLAGS += -DFIRRTL_COVER
endif

# LLVM Sanitizer Coverage support
ifneq ($(LLVM_COVER),)
EMU_CXXFLAGS += -DLLVM_COVER
EMU_LDFLAGS += -fsanitize-coverage=trace-pc-guard -fsanitize-coverage=pc-table
endif

VERILATOR_FLAGS =                   \
  --exe                             \
  --cc -O3 --top-module $(EMU_TOP)  \
  +define+VERILATOR=1               \
  +define+PRINTF_COND=1             \
  +define+RANDOMIZE_REG_INIT        \
  +define+RANDOMIZE_MEM_INIT        \
  +define+RANDOMIZE_GARBAGE_ASSIGN  \
  +define+RANDOMIZE_DELAY=0         \
  -Wno-STMTDLY -Wno-WIDTH           \
  --assert --x-assign unique        \
  --stats-vars                      \
  --output-split 30000              \
  --output-split-cfuncs 30000       \
  -I$(BUILD_DIR)                    \
  -CFLAGS "$(EMU_CXXFLAGS)"         \
  -LDFLAGS "$(EMU_LDFLAGS)"         \
  -o $(abspath $(EMU))              \
  $(VEXTRA_FLAGS)

EMU_MK    := $(BUILD_DIR)/emu-compile/V$(EMU_TOP).mk
EMU_DEPS  := $(SIM_VSRC) $(EMU_CXXFILES)
EMU_HEADERS := $(shell find $(EMU_CSRC_DIR) -name "*.h")     \
               $(shell find $(SIM_CSRC_DIR) -name "*.h")     \
               $(shell find $(DIFFTEST_CSRC_DIR) -name "*.h")

$(EMU_MK): $(SIM_TOP_V) | $(EMU_DEPS)
ifeq ($(EMU_COVERAGE),1)
	@python3 ./scripts/coverage/vtransform.py $(BUILD_DIR)
endif
	@mkdir -p $(@D)
	@echo "\n[verilator] Generating C++ files..." >> $(TIMELOG)
	@date -R | tee -a $(TIMELOG)
	$(TIME_CMD) verilator $(VERILATOR_FLAGS) -Mdir $(@D) $^ $(EMU_DEPS)
ifneq ($(VERILATOR_5_000),1)
	@find -L $(BUILD_DIR) -name "VSimTop.h" | xargs sed -i 's/private/public/g'
	@find -L $(BUILD_DIR) -name "VSimTop.h" | xargs sed -i 's/const vlSymsp/vlSymsp/g'
	@find -L $(BUILD_DIR) -name "VSimTop__Syms.h" | xargs sed -i 's/VlThreadPool\* const/VlThreadPool*/g'
endif

EMU_COMPILE_FILTER =
# 2> $(BUILD_DIR)/g++.err.log | tee $(BUILD_DIR)/g++.out.log | grep 'g++' | awk '{print "Compiling/Generating", $$NF}'

build_emu_local: $(EMU_MK)
	@echo "\n[g++] Compiling C++ files..." >> $(TIMELOG)
	@date -R | tee -a $(TIMELOG)
	$(TIME_CMD) $(MAKE) -s VM_PARALLEL_BUILDS=1 OPT_FAST="-O3" -C $(<D) -f $(<F) $(EMU_COMPILE_FILTER)

$(EMU): $(EMU_MK) $(EMU_DEPS) $(EMU_HEADERS)
ifeq ($(REMOTE),localhost)
	@$(MAKE) build_emu_local
else
	ssh -tt $(REMOTE) 'export NOOP_HOME=$(NOOP_HOME); $(MAKE) -C $(NOOP_HOME)/difftest -j230 build_emu_local'
endif

emu: $(EMU)

COVERAGE_DATA ?= $(shell find $(BUILD_DIR) -maxdepth 1 -name "*.dat")
COVERAGE_DIR  ?= $(DESIGN_DIR)/$(basename $(notdir $(COVERAGE_DATA)))
coverage:
	@verilator_coverage --annotate $(COVERAGE_DIR) --annotate-min 1 $(COVERAGE_DATA)
	@python3 scripts/coverage/coverage.py $(COVERAGE_DIR)/$(SIM_TOP).v $(COVERAGE_DIR)/$(SIM_TOP)_annotated.v
	@python3 scripts/coverage/statistics.py $(COVERAGE_DIR)/$(SIM_TOP)_annotated.v > $(COVERAGE_DIR)/coverage_$(SIM_TOP).log
	@mv $(COVERAGE_DATA) $(COVERAGE_DIR)

.PHONY: build_emu_local
