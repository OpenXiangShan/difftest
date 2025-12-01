#***************************************************************************************
# Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
# Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2020-2025 X-EPIC Co., Ltd
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
# GalaxSim, developed by X-EPIC Technologies, is an event-driven Verilog/SystemVerilog simulator.
#
# GalaxSim Turbo, another innovation from X-EPIC, is a hybrid simulation accelerator that
# combines event-driven and cycle-based methodologies. 
#
# FusionDebug, a feature-rich debugger that supports a wide array of simulation debugging techniques.
#
# These 3 tools are available for free trial on the website: https://free.x-epic.com/
#
#***************************************************************************************
RUN_BIN       ?= $(DESIGN_DIR)/ready-to-run/copy_and_run.bin
GLX           = galaxsim
GLX_TOP       = tb_top
GLX_TARGET    = $(abspath $(BUILD_DIR)/xsim)
GLX_BUILD_DIR = $(abspath $(BUILD_DIR)/xsim-compile)
GLX_RUN_DIR   = $(abspath $(BUILD_DIR)/$(notdir $(RUN_BIN)))


GLX_CSRC_DIR   = $(abspath ./src/test/csrc/vcs)
GLX_CONFIG_DIR = $(abspath ./config)

GLX_CXXFILES  = $(SIM_CXXFILES) $(shell find $(GLX_CSRC_DIR) -name "*.cpp")

GLX_CXXFLAGS  = $(SIM_CXXFLAGS) -I$(GLX_CONFIG_DIR) -I$(GLX_CSRC_DIR)

GLX_LDFLAGS   = $(SIM_LDFLAGS)

GLX_VSRC_DIR 	= $(abspath ./src/test/vsrc/vcs)

GLX_VFILES  = $(SIM_VSRC) $(shell find $(GLX_VSRC_DIR) -name "*.v" -or -name "*.sv")

GLX_FLAGS = $(SIM_VFLAGS)
GLX_FLAGS += -timescale=1ns/1ns  -debug_access+all
GLX_FLAGS += -j8
GLX_FLAGS += +define+GLX
ifeq ($(ENABLE_XPROP),1)
GLX_FLAGS += -xprop
else
# randomize all undefined signals (instead of using X)
# GLX_FLAGS += -initreg
endif
GLX_CXXFLAGS += -std=c++11


GLX_FLAGS += -o $(GLX_TARGET)
ifneq ($(ENABLE_XPROP),1)
GLX_FLAGS += +define+RANDOMIZE_GARBAGE_ASSIGN
GLX_FLAGS += +define+RANDOMIZE_INVALID_ASSIGN
GLX_FLAGS += +define+RANDOMIZE_MEM_INIT
GLX_FLAGS += +define+RANDOMIZE_REG_INIT
endif
# manually set RANDOMIZE_DELAY to avoid GLX from incorrect random initialize
# NOTE: RANDOMIZE_DELAY must NOT be rounded to 0
GLX_FLAGS += +define+RANDOMIZE_DELAY=1
# SRAM lib defines
GLX_FLAGS += +define+UNIT_DELAY +define+no_warning
# C++ flags
GLX_FLAGS += -XCFLAGS "$(GLX_CXXFLAGS)" -XLDFLAGS "$(GLX_LDFLAGS)"
# search build for other missing verilog files
GLX_FLAGS += -y $(RTL_DIR) +libext+.v +libext+.sv
# search generated-src for verilog included files
GLX_FLAGS += +incdir+$(GEN_VSRC_DIR)


#glaxism turbo mode
ifeq ($(GLX_TURBO),1)
GLX_CXXFLAGS += -DVERILATOR_4_210
GLX_FLAGS += -turbo_mode +define+VERILATOR_5 +define+VERILATOR=1 +define+PRINTF_COND=1 +define+RANDOMIZE_DELAY=0
endif


$(GLX_TARGET): $(SIM_TOP_V) $(GLX_CXXFILES) $(GLX_VFILES)
	$(GLX) $(GLX_USER_COMP_OPTS) $(GLX_FLAGS) $(SIM_TOP_V) $(GLX_CXXFILES) $(GLX_VFILES)

xsim: $(GLX_TARGET)

RUN_OPTS := +workload=$(RUN_BIN)

#glaxism turbo threads
ifneq ($(GLX_TURBO_THREADS),0)
RUN_OPTS += -turbo_threads=$(GLX_TURBO_THREADS)
endif

ifeq ($(NO_DIFF),1)
RUN_OPTS += +no-diff
endif

RUN_OPTS += -assert finish_maxfail=30 -assert maxfail=100

xsim-run:
	$(shell if [ ! -e $(GLX_RUN_DIR) ]; then mkdir -p $(GLX_RUN_DIR); fi)
	touch $(GLX_RUN_DIR)/sim.log
	$(shell if [ -e $(GLX_RUN_DIR)/xsim ]; then rm -f $(GLX_RUN_DIR)/xsim; fi)
	$(shell if [ -e $(GLX_RUN_DIR)/xsim.data ]; then rm -rf $(GLX_RUN_DIR)/xsim.data; fi)
	$(shell if [ -e $(GLX_RUN_DIR)/xsim.src ]; then rm -rf $(GLX_RUN_DIR)/xsim.src; fi)
	cp $(GLX_TARGET) $(GLX_RUN_DIR)/xsim
	cp -r $(BUILD_DIR)/xsim.data $(GLX_RUN_DIR)/xsim.data
	cp -r $(BUILD_DIR)/xsim.src $(GLX_RUN_DIR)/xsim.src
	cd $(GLX_RUN_DIR) && (./xsim $(GLX_USER_RUN_OPTS) $(RUN_OPTS)  | tee sim.log)

glx-clean:
	rm -rf xsim xsim.data xsim.src $(GLX_BUILD_DIR)


