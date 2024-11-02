#***************************************************************************************
# Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
# Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

GSIM_TOP       = SimTop
GSIM_TARGET    = $(abspath $(BUILD_DIR)/gsim)
GSIM_BUILD_DIR = $(abspath $(BUILD_DIR)/gsim-compile)

GSIM_CSRC_DIR   = $(abspath ./src/test/csrc/verilator)
GSIM_CONFIG_DIR = $(abspath ./config)

GSIM_CXXFILES  = $(SIM_CXXFILES) $(shell find $(GSIM_CSRC_DIR) -name "*.cpp")
GSIM_CXXFLAGS  = $(SIM_CXXFLAGS) -I$(GSIM_CSRC_DIR) -DNUM_CORES=$(NUM_CORES) -DGSIM
GSIM_LDFLAGS   = $(SIM_LDFLAGS) -ldl

GSIM_HOME := $(abspath $(GSIM_HOME))

$(GSIM_TARGET):
ifndef GSIM_HOME
	$(error GSIM_HOME is not set, please set manually)
endif
	# make -C $(GSIM_HOME) compile BUILD_DIR=$(GSIM_BUILD_DIR) NAME=$(GSIM_TOP) FIRRTL_FILE=$(SIM_TOP_FIR)
	make -C $(GSIM_HOME) build-emu BUILD_DIR=$(GSIM_BUILD_DIR) NAME=$(GSIM_TOP) GSIM_TARGET=$(GSIM_TARGET) GSIM_CXXFILES="$(GSIM_CXXFILES)" GSIM_CXXFLAGS="$(GSIM_CXXFLAGS)" GSIM_LDFLAGS="$(GSIM_LDFLAGS)"

gsim: $(GSIM_TARGET)
