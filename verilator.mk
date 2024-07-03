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

EMU_CXXFILES  = $(SIM_CXXFILES) $(shell find $(EMU_CSRC_DIR) -name "*.cpp")
EMU_CXXFLAGS  = $(SIM_CXXFLAGS) -I$(EMU_CSRC_DIR)
EMU_CXXFLAGS += -DVERILATOR -DNUM_CORES=$(NUM_CORES) --std=c++17
EMU_LDFLAGS   = $(SIM_LDFLAGS) -ldl

# DiffTest support
ifneq ($(NO_DIFF), 1)
VEXTRA_FLAGS += +define+DIFFTEST
endif

# Link fuzzer libraries
ifneq ($(FUZZER_LIB), )
# the target is named as fuzzer for clarity
EMU = $(BUILD_DIR)/fuzzer
endif

# Verilator version check
VERILATOR_VER_CMD = verilator --version 2> /dev/null | cut -f2 -d' ' | tr -d '.'
VERILATOR_4_210 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 4210 2> /dev/null)
ifeq ($(VERILATOR_4_210),1)
EMU_CXXFLAGS += -DVERILATOR_4_210
VEXTRA_FLAGS += --instr-count-dpi 1
endif
VERILATOR_5_000 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 5000 2> /dev/null)
ifeq ($(VERILATOR_5_000),1)
VEXTRA_FLAGS += --no-timing +define+VERILATOR_5
else
VEXTRA_FLAGS += +define+VERILATOR_LEGACY
endif
VERILATOR_5_024 := $(shell expr `$(VERILATOR_VER_CMD)` \>= 5024 2> /dev/null)
ifeq ($(VERILATOR_5_024),1)
VEXTRA_FLAGS += --quiet-stats
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

# Verilator optimization
EMU_OPTIMIZE ?= -O3

# C optimization
OPT_FAST ?= -O3

VERILATOR_FLAGS =                   \
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
  -CFLAGS "$(EMU_CXXFLAGS)"         \
  -LDFLAGS "$(EMU_LDFLAGS)"         \
  -CFLAGS "\$$(PGO_CFLAGS)"         \
  -LDFLAGS "\$$(PGO_LDFLAGS)"       \
  -o $(abspath $(EMU))              \
  $(VEXTRA_FLAGS)

EMU_DIR = $(BUILD_DIR)/emu-compile
EMU_MK  = $(EMU_DIR)/V$(EMU_TOP).mk
EMU_DEPS  := $(SIM_VSRC) $(EMU_CXXFILES)
EMU_HEADERS := $(shell find $(EMU_CSRC_DIR) -name "*.h")     \
               $(shell find $(SIM_CSRC_DIR) -name "*.h")     \
               $(shell find $(DIFFTEST_CSRC_DIR) -name "*.h")

# Profile Guided Optimization
EMU_PGO_DIR  = $(EMU_DIR)/pgo
PGO_MAX_CYCLE ?= 2000000

$(EMU_MK): $(SIM_TOP_V) | $(EMU_DEPS)
ifeq ($(EMU_COVERAGE),1)
	@python3 ./scripts/coverage/vtransform.py $(RTL_DIR)
endif
	@mkdir -p $(@D)
	@echo -e "\n[verilator] Generating C++ files..." >> $(TIMELOG)
	@date -R | tee -a $(TIMELOG)
	$(TIME_CMD) verilator $(VERILATOR_FLAGS) -Mdir $(@D) $^ $(EMU_DEPS)
ifneq ($(VERILATOR_5_000),1)
	@sed -i 's/private/public/g' $(EMU_DIR)/VSimTop.h
	@sed -i 's/const vlSymsp/vlSymsp/g' $(EMU_DIR)/VSimTop.h
	@sed -i 's/VlThreadPool\* const/VlThreadPool*/g' $(EMU_DIR)/VSimTop__Syms.h
endif

EMU_COMPILE_FILTER =
# 2> $(BUILD_DIR)/g++.err.log | tee $(BUILD_DIR)/g++.out.log | grep 'g++' | awk '{print "Compiling/Generating", $$NF}'

build_emu:
ifeq ($(REMOTE),localhost)
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)
	$(TIME_CMD) $(MAKE) -s VM_PARALLEL_BUILDS=1 OPT_SLOW="-O0" \
						OPT_FAST=$(OPT_FAST) \
						PGO_CFLAGS=$(PGO_CFLAGS) \
						PGO_LDFLAGS=$(PGO_LDFLAGS) \
						-C $(EMU_DIR) -f $(EMU_MK) $(EMU_COMPILE_FILTER)
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)
else
	ssh -tt $(REMOTE) 'export NOOP_HOME=$(NOOP_HOME); \
					   $(MAKE) -C $(NOOP_HOME)/difftest build_emu \
					   -j `nproc` \
					   OPT_FAST="'"$(OPT_FAST)"'" \
					   PGO_CFLAGS="'"$(PGO_CFLAGS)"'" \
					   PGO_LDFLAGS="'"$(PGO_LDFLAGS)"'"'
endif

$(EMU): $(EMU_MK) $(EMU_DEPS) $(EMU_HEADERS)
	@echo -e "\n[c++] Compiling C++ files..." >> $(TIMELOG)
	@date -R | tee -a $(TIMELOG)
ifdef PGO_WORKLOAD
	@echo "Building PGO profile..."
	@stat $(PGO_WORKLOAD) > /dev/null
	@$(MAKE) clean_obj
	@mkdir -p $(EMU_PGO_DIR)
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)
	@$(MAKE) build_emu OPT_FAST=$(OPT_FAST) \
					   PGO_CFLAGS="-fprofile-generate=$(EMU_PGO_DIR)" \
					   PGO_LDFLAGS="-fprofile-generate=$(EMU_PGO_DIR)"
	@echo "Training emu with PGO Workload..."
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)
	$(EMU) -i $(PGO_WORKLOAD) --max-cycles=$(PGO_MAX_CYCLE) \
		   1>$(EMU_PGO_DIR)/`date +%s`.log \
		   2>$(EMU_PGO_DIR)/`date +%s`.err \
		   $(PGO_EMU_ARGS)
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)
ifdef LLVM_PROFDATA
# When using LLVM's profile-guided optimization, the raw data can not
# directly be used in -fprofile-use. We need to use a specific version of
# llvm-profdata. This happens when verilator compiled with CC=clang
# CXX=clang++. In this case, we should add LLVM_PROFDATA=llvm-profdata
# when calling make. For GCC, this step should be skipped. Also, some
# machines may have multiple versions of llvm-profdata. So please never
# add default value for LLVM_PROFDATA unless we have a proper way to probe
# the compiler and the corresponding llvm-profdata value.
	$(LLVM_PROFDATA) merge $(EMU_PGO_DIR)/*.profraw -o $(EMU_PGO_DIR)/default.profdata
else # ifdef LLVM_PROFDATA
	@echo ""
	@echo "----------------------- NOTICE BEGIN -----------------------"
	@echo "If your verilator is compiled with LLVM, please don't forget"
	@echo "to add LLVM_PROFDATA=llvm-profdata when calling make."
	@echo ""
	@echo "If your verilator is compiled with GCC, please ignore this"
	@echo "message and NEVER adding LLVM_PROFDATA when calling make."
	@echo "----------------------- NOTICE  END  -----------------------"
	@echo ""
endif # ifdef LLVM_PROFDATA
	@echo "Building emu with PGO profile..."
	@$(MAKE) clean_obj
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)
	@$(MAKE) build_emu OPT_FAST=$(OPT_FAST) \
					   PGO_CFLAGS="-fprofile-use=$(EMU_PGO_DIR)" \
					   PGO_LDFLAGS="-fprofile-use=$(EMU_PGO_DIR)"
else # ifdef PGO_WORKLOAD
	@echo "Building emu..."
	@$(MAKE) build_emu OPT_FAST=$(OPT_FAST)
endif # ifdef PGO_WORKLOAD
	@sync -d $(BUILD_DIR) -d $(EMU_DIR)

emu: $(EMU)

COVERAGE_DATA ?= $(shell find $(BUILD_DIR) -maxdepth 1 -name "*.dat")
COVERAGE_DIR  ?= $(DESIGN_DIR)/$(basename $(notdir $(COVERAGE_DATA)))
coverage:
	@verilator_coverage --annotate $(COVERAGE_DIR) --annotate-min 1 $(COVERAGE_DATA)
	@python3 scripts/coverage/coverage.py $(COVERAGE_DIR)
	@python3 scripts/coverage/statistics.py $(COVERAGE_DIR) > $(COVERAGE_DIR)/coverage_$(SIM_TOP).log
	@mv $(COVERAGE_DATA) $(COVERAGE_DIR)

clean_obj:
	rm -f $(EMU_DIR)/*.o $(EMU_DIR)/*.gch $(EMU_DIR)/*.a $(EMU)

.PHONY: build_emu clean_obj
