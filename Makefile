#***************************************************************************************
# Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2020-2021 Peng Cheng Laboratory
#
# XiangShan is licensed under Mulan PSL v2.
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

SIM_TOP    ?= SimTop
DESIGN_DIR ?= ..
NUM_CORES  ?= 1

BUILD_DIR = $(DESIGN_DIR)/build
SIM_TOP_V = $(BUILD_DIR)/$(SIM_TOP).v

DIFF_SCALA_FILE = $(shell find ./src/main/scala -name '*.scala')
SCALA_FILE = $(shell find $(DESIGN_DIR)/src/main/scala -name '*.scala')

# generate SimTop.v
$(SIM_TOP_V): $(DIFF_SCALA_FILE) $(SCALA_FILE)
	$(MAKE) -C $(DESIGN_DIR) sim-verilog

# co-simulation with DRAMsim3
ifeq ($(WITH_DRAMSIM3),1)
ifndef DRAMSIM3_HOME
$(error DRAMSIM3_HOME is not set)
endif
override SIM_ARGS += --with-dramsim3
endif

TIMELOG = $(BUILD_DIR)/time.log
TIME_CMD = time -a -o $(TIMELOG)

# remote machine with more cores to speedup c++ build
REMOTE ?= localhost
.DEFAULT_GOAL = emu

sim-verilog: $(SIM_TOP_V)

SIM_CSRC_DIR = $(abspath ./src/test/csrc/common)
SIM_CXXFILES = $(shell find $(SIM_CSRC_DIR) -name "*.cpp")

DIFFTEST_CSRC_DIR = $(abspath ./src/test/csrc/difftest)
DIFFTEST_CXXFILES = $(shell find $(DIFFTEST_CSRC_DIR) -name "*.cpp")

SIM_VSRC = $(shell find ./src/test/vsrc/common -name "*.v" -or -name "*.sv")

include verilator.mk
include vcs.mk

ifndef NEMU_HOME
$(error NEMU_HOME is not set)
endif
REF_SO := $(NEMU_HOME)/build/riscv64-nemu-interpreter-so
$(REF_SO):
	$(MAKE) -C $(NEMU_HOME) riscv64-xs-ref_defconfig
	$(MAKE) -C $(NEMU_HOME)

SEED ?= $(shell shuf -i 1-10000 -n 1)

release-lock:
	ssh -tt $(REMOTE) 'rm -f $(LOCK)'

clean: vcs-clean
	rm -rf $(BUILD_DIR)

.PHONY: sim-verilog emu clean$(REF_SO)

