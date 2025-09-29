#***************************************************************************************
# Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2025 Beijing Institute of Open Source Chip
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

EMU_ELF_NAME = emu
# the target is named as fuzzer for clarity in fuzzing
ifneq ($(FUZZER_LIB), )
EMU_ELF_NAME = fuzzer
endif

EMU          = $(BUILD_DIR)/$(EMU_ELF_NAME)
EMU_TOP      = SimTop

EMU_CSRC_DIR   = $(abspath ./src/test/csrc/emu)
EMU_CONFIG_DIR = $(abspath ./config)

EMU_CXXFILES  = $(SIM_CXXFILES) $(shell find $(EMU_CSRC_DIR) -name "*.cpp")
EMU_CXXFLAGS  = $(SIM_CXXFLAGS) -I$(EMU_CSRC_DIR) -DNUM_CORES=$(NUM_CORES)

EMU_HEADERS := $(shell find $(SIM_CSRC_DIR) -name "*.h")      \
               $(shell find $(DIFFTEST_CSRC_DIR) -name "*.h") \
			   $(shell find $(EMU_CSRC_DIR) -name "*.h")

########## Supported Configuration Options ##########
# trace (waveform)
EMU_TRACE ?=

# trace (waveform) underscore values
EMU_TRACE_ALL ?=

# multi-threading RTL-simulation
EMU_THREADS ?= 0
ifneq ($(EMU_THREADS),0)
EMU_CXXFLAGS += -DEMU_THREAD=$(EMU_THREADS)
endif

# RTL-level savable models
EMU_SNAPSHOT ?=
ifeq ($(EMU_SNAPSHOT),1)
EMU_CXXFLAGS += -DVM_SAVABLE
endif

# RTL-level structural coverage (instrumented by RTL simulators)
EMU_COVERAGE ?=

# optimization level for RTL simulators
EMU_OPTIMIZE ?= -O3

include verilator.mk
include gsim.mk

########## Emu build recipes ##########

emu-verilator: verilator-emu
	@ln -sf $(VERILATOR_TARGET) $(EMU)

emu-gsim: gsim-emu
	@ln -sf $(GSIM_EMU_TARGET) $(EMU)

# By default, when no simulator is specified, emu refers to verilator-emu
emu:
ifeq ($(GSIM),1)
	@$(MAKE) emu-gsim
else
	@$(MAKE) emu-verilator
endif

emu-mk: verilator-emu-mk

clean-obj: verilator-clean-obj gsim-clean-obj
