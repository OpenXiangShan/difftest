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

VCS           = vcs
VCS_TOP       = tb_top
VCS_TARGET    = $(abspath $(BUILD_DIR)/simv)
VCS_BUILD_DIR = $(abspath $(BUILD_DIR)/simv-compile)

VCS_CSRC_DIR   = $(abspath ./src/test/csrc/vcs)
VCS_CONFIG_DIR = $(abspath ./config)

VCS_CXXFILES  = $(SIM_CXXFILES) $(shell find $(VCS_CSRC_DIR) -name "*.cpp")
VCS_CXXFLAGS  = $(SIM_CXXFLAGS) -I$(VCS_CSRC_DIR) -DNUM_CORES=$(NUM_CORES)
VCS_LDFLAGS   = $(SIM_LDFLAGS) -lpthread -ldl

# DiffTest support
ifneq ($(NO_DIFF), 1)
VCS_FLAGS    += +define+DIFFTEST
endif

ifeq ($(RELEASE),1)
VCS_FLAGS    += +define+SNPS_FAST_SIM_FFV +define+USE_RF_DEBUG
endif

# if fsdb is considered
# CONSIDER_FSDB ?= 0
ifeq ($(CONSIDER_FSDB),1)
EXTRA = +define+CONSIDER_FSDB
# if VERDI_HOME is not set automatically after 'module load', please set manually.
ifndef VERDI_HOME
$(error VERDI_HOME is not set. Try whereis verdi, abandon /bin/verdi and set VERID_HOME manually)
else
NOVAS_HOME = $(VERDI_HOME)
NOVAS = $(NOVAS_HOME)/share/PLI/VCS/LINUX64
EXTRA += -P $(NOVAS)/novas.tab $(NOVAS)/pli.a
endif
endif

ifeq ($(VCS),verilator)
VCS_FLAGS += --exe --cc --main --top-module $(VCS_TOP) -Wno-WIDTH
VCS_FLAGS += --instr-count-dpi 1 --timing +define+VERILATOR_5
VCS_FLAGS += -Mdir $(VCS_BUILD_DIR)  --compiler gcc
VCS_CXXFLAGS += -std=c++20
else
VCS_FLAGS += -full64 +v2k -timescale=1ns/1ns -sverilog -debug_access+all +lint=TFIPC-L
VCS_FLAGS += -Mdir=$(VCS_BUILD_DIR) -j200
# randomize all undefined signals (instead of using X)
VCS_FLAGS += +vcs+initreg+random +define+VCS
VCS_CXXFLAGS += -std=c++11 -static
endif

VCS_FLAGS += -o $(VCS_TARGET)
VCS_FLAGS += +define+RANDOMIZE_GARBAGE_ASSIGN
VCS_FLAGS += +define+RANDOMIZE_INVALID_ASSIGN
VCS_FLAGS += +define+RANDOMIZE_MEM_INIT
VCS_FLAGS += +define+RANDOMIZE_REG_INIT
# manually set RANDOMIZE_DELAY to avoid VCS from incorrect random initialize
# NOTE: RANDOMIZE_DELAY must NOT be rounded to 0
VCS_FLAGS += +define+RANDOMIZE_DELAY=1
# SRAM lib defines
VCS_FLAGS += +define+UNIT_DELAY +define+no_warning
# C++ flags
VCS_FLAGS += -CFLAGS "$(VCS_CXXFLAGS)" -LDFLAGS "$(VCS_LDFLAGS)"
# search build for other missing verilog files
VCS_FLAGS += -y $(RTL_DIR) +libext+.v
# enable fsdb dump
VCS_FLAGS += $(EXTRA)

VCS_VSRC_DIR = $(abspath ./src/test/vsrc/vcs)
VCS_VFILES   = $(SIM_VSRC) $(shell find $(VCS_VSRC_DIR) -name "*.v")
$(VCS_TARGET): $(SIM_TOP_V) $(VCS_CXXFILES) $(VCS_VFILES)
	$(VCS) $(VCS_FLAGS) $(SIM_TOP_V) $(VCS_CXXFILES) $(VCS_VFILES)
ifeq ($(VCS),verilator)
	$(MAKE) -s -C $(VCS_BUILD_DIR) -f V$(VCS_TOP).mk
endif

simv: $(VCS_TARGET)

vcs-clean:
	rm -rf simv csrc DVEfiles simv.daidir stack.info.* ucli.key $(VCS_BUILD_DIR)
