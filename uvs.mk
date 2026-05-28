#***************************************************************************************
# Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
# Copyright (c) 2026 Shanghai UniVista Industrial Software Group Co., Ltd
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
#
# ==============================================================================
# 1. COMPANY PROFILE
#    Name : Shanghai UniVista Industrial Software Group Co., Ltd.
#    Web  : https://www.univista-isg.com/
#
# 2. SIMULATOR: UniVista Simulator Plus (UVS+)
#    A high-performance simulator built on a self-developed architecture.
#    Supports full-flow verification from RTL to gate-level (SDF), mixed-signal,
#    low-power (UPF), etc. Delivers industry-leading performance for
#    large-scale SoC verification.
#
# 3. DEBUGGER: UniVista Debugger Plus (UVD+)
#    A full-featured debug platform with a self-developed architecture and modern GUI,
#    featuring a high-performance waveform engine and smart source tracing.
#    Supports coverage analysis, mixed-signal, low-power, and transaction-level
#    protocol analysis, etc., and delivers industry-leading performance on debugging.
#***************************************************************************************

UVS           = uvs
UVS_TOP       = tb_top
UVS_TARGET    = $(abspath $(BUILD_DIR)/uvsim)
RUN_BIN       ?= $(DESIGN_DIR)/ready-to-run/copy_and_run.bin
UVS_RUN_DIR   = $(abspath $(BUILD_DIR)/$(notdir $(RUN_BIN)))

UVS_CSRC_DIR   = $(abspath ./src/test/csrc/vcs)
UVS_CONFIG_DIR = $(abspath ./config)

UVS_CXXFILES  = $(SIM_CXXFILES) $(shell find $(UVS_CSRC_DIR) -name "*.cpp")
UVS_CXXFLAGS  = $(SIM_CXXFLAGS) -I$(UVS_CSRC_DIR) -DNUM_CORES=$(NUM_CORES)
UVS_LDFLAGS   = $(SIM_LDFLAGS) -lpthread -ldl

# DiffTest support
ifneq ($(NO_DIFF),1)
UVS_FLAGS    += +define+DIFFTEST
endif

ifeq ($(RELEASE),1)
UVS_FLAGS    += +define+SNPS_FAST_SIM_FFV +define+USE_RF_DEBUG
endif

# core soft rst
ifeq ($(WORKLOAD_SWITCH),1)
UVS_FLAGS    += +define+ENABLE_WORKLOAD_SWITCH
endif

ifeq ($(SYNTHESIS), 1)
UVS_FLAGS    += +define+SYNTHESIS +define+TB_NO_DPIC
else
ifeq ($(DISABLE_DIFFTEST_RAM_DPIC), 1)
UVS_FLAGS    += +define+DISABLE_DIFFTEST_RAM_DPIC
endif
ifeq ($(DISABLE_DIFFTEST_FLASH_DPIC), 1)
UVS_FLAGS    += +define+DISABLE_DIFFTEST_FLASH_DPIC
endif
endif

# if usdb is considered
# CONSIDER_USDB ?= 0
ifeq ($(CONSIDER_USDB),1)
EXTRA = +define+CONSIDER_USDB

endif


UVS_FLAGS += -timescale 1ns/1ns -sv -ddb -debug all -lint 126013 -logfile uvs_compile.log
UVS_FLAGS += +define+UVS

ifeq ($(ENABLE_XPROP),1)
UVS_FLAGS += -xprop
else
# randomize all undefined signals (instead of using X)
UVS_FLAGS += -deposit_initval 
endif
UVS_CXXFLAGS += -std=c++11 -static

UVS_FLAGS += -o $(UVS_TARGET)
ifneq ($(ENABLE_XPROP),1)
UVS_FLAGS += +define+RANDOMIZE_GARBAGE_ASSIGN
UVS_FLAGS += +define+RANDOMIZE_INVALID_ASSIGN
UVS_FLAGS += +define+RANDOMIZE_MEM_INIT
UVS_FLAGS += +define+RANDOMIZE_REG_INIT
endif
# manually set RANDOMIZE_DELAY to avoid UVS from incorrect random initialize
# NOTE: RANDOMIZE_DELAY must NOT be rounded to 0
UVS_FLAGS += +define+RANDOMIZE_DELAY=1
# SRAM lib defines
UVS_FLAGS += +define+UNIT_DELAY +define+no_warning
# C++ flags
UVS_FLAGS += -cflags "$(UVS_CXXFLAGS)" -cxxflags "$(UVS_CXXFLAGS)" -ldflags "$(UVS_LDFLAGS)"
# search build for other missing verilog files
UVS_FLAGS += -libdir $(RTL_DIR) +libext+.v +libext+.sv
# search generated-src for verilog included files
UVS_FLAGS += +incdir+$(GEN_VSRC_DIR)
# enable usdb dump
UVS_FLAGS += $(EXTRA)

UVS_EXTRA_OPTS ?=
UVS_FLAGS += $(UVS_EXTRA_OPTS)

UVS_VSRC_DIR = $(abspath ./src/test/vsrc/vcs)
UVS_VFILES   = $(SIM_VSRC) $(shell find $(UVS_VSRC_DIR) -name "*.v" -or -name "*.sv")
$(UVS_TARGET): $(SIM_TOP_V) $(UVS_CXXFILES) $(UVS_VFILES)
	$(UVS) $(UVS_FLAGS) $(SIM_TOP_V) $(UVS_CXXFILES) $(UVS_VFILES)

uvsim: $(UVS_TARGET)

RUN_OPTS := +workload=$(RUN_BIN)

ifeq ($(CONSIDER_USDB),1)
RUN_OPTS += +dump-wave=usdb
endif

ifneq ($(REF_SO),)
RUN_OPTS += +diff=$(REF_SO)
endif

ifeq ($(NO_DIFF),1)
RUN_OPTS += +no-diff
endif

RUN_OPTS += -sva_single_exit_maxfail 30 -sva_exit_maxfail 10000   

RUN_EXTRA_OPTS ?=
RUN_OPTS += $(RUN_EXTRA_OPTS)

uvsim-run:
	$(shell if [ ! -e $(UVS_RUN_DIR) ]; then mkdir -p $(UVS_RUN_DIR); fi)
	touch $(UVS_RUN_DIR)/uvs_sim.log
	$(shell if [ -e $(UVS_RUN_DIR)/uvsim ]; then rm -f $(UVS_RUN_DIR)/uvsim; fi)
	$(shell if [ -e $(UVS_RUN_DIR)/uvsim.db ]; then rm -rf $(UVS_RUN_DIR)/uvsim.db; fi)
	ln -s $(UVS_TARGET) $(UVS_RUN_DIR)/uvsim
	ln -s $(BUILD_DIR)/uvsim.db $(UVS_RUN_DIR)/uvsim.db
	cd $(UVS_RUN_DIR) && (./uvsim $(RUN_OPTS) 2> assert.log | tee uvs_sim.log) 

uvsim-debug:
	$(shell if [ ! -e $(UVS_RUN_DIR) ]; then mkdir -p $(UVS_RUN_DIR); fi)
	$(shell if [ ! -e $(UVS_RUN_DIR)/uvsim ]; then  ln -s $(UVS_TARGET) $(UVS_RUN_DIR)/uvsim; fi)
	$(shell if [ ! -e $(UVS_RUN_DIR)/uvsim.db ]; then ln -s $(BUILD_DIR)/uvsim.db $(UVS_RUN_DIR)/uvsim.db; fi)
	cd $(UVS_RUN_DIR) && (uvd -d uvsim.db -u uvsim.usdb &)
	
uvs-clean:
	rm -rf uvs_sim.log uvsim.db uvsim  
