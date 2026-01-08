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

ifneq ($(CORVUSITOR_PATH),)
CORVUSITOR_REAL_PATH = $(CORVUSITOR_PATH)
else
CORVUSITOR_REAL_PATH = corvusitor
endif
CORVUSITOR_BUILD_DIR ?= $(BUILD_DIR)/corvusitor-compile

CORVUS_MODULE_FILES := $(shell find $(RTL_DIR) -name "corvus_comb_*.sv") \
                       $(shell find $(RTL_DIR) -name "corvus_seq_*.sv") \
					   $(shell find $(RTL_DIR) -name "corvus_external.sv")

define RUN_CORVUS_MODULE
.corvus.run.$1:
	@echo "Running Corvus on module $(basename $(notdir $1))"
	+$(MAKE) emu \
		VERILATOR_BUILD_DIR=$(BUILD_DIR)/verilator-compile-$(basename $(notdir $1)) \
		VERILATOR_ARCHIVE=1 \
		EMU_TOP=$(basename $(notdir $1)) \
		EMU_ELF_NAME=$(basename $(notdir $1)) \
		SIM_TOP=$(basename $(notdir $1))
endef

corvus-archive: $(CORVUS_MODULE_FILES:%=.corvus.run.%)

corvus: corvus-archive
	mkdir -p $(CORVUSITOR_BUILD_DIR)
	$(CORVUSITOR_REAL_PATH) -m $(BUILD_DIR) -o $(CORVUSITOR_BUILD_DIR)/VCorvusTopWrapper_generated.cpp

$(foreach f,$(CORVUS_MODULE_FILES),$(eval $(call RUN_CORVUS_MODULE,$f)))

ifeq ($(CORVUSITOR),1)

CORVUSITOR_TARGET = $(CORVUSITOR_BUILD_DIR)/$(EMU_ELF_NAME)

########## Corvusitor Configuration Options ##########

CORVUSITOR_CSRC_DIR = $(abspath ./src/test/csrc/corvusitor)
CORVUSITOR_CXXFILES = $(EMU_CXXFILES) $(shell find $(CORVUSITOR_CSRC_DIR) -name "*.cpp")
CORVUSITOR_CXXFLAGS = $(subst \\\",\", $(EMU_CXXFLAGS)) -I$(CORVUSITOR_CSRC_DIR) -DCORVUSITOR --std=c++17
CORVUSITOR_LDFLAGS  = $(SIM_LDFLAGS) -ldl

ifeq ($(VERILATOR_4_210),1)
CORVUSITOR_CXXFLAGS += -DVERILATOR_4_210
endif

ifneq (,$(filter $(EMU_TRACE),fst FST))
CORVUSITOR_CXXFLAGS += -DENABLE_FST
endif

_CORVUS_USER_INCLUDE_FLAGS = -I$(RTL_DIR) -I$(GEN_VSRC_DIR) -I$(CORVUSITOR_BUILD_DIR) $(CORVUSITOR_CXXFLAGS)
_CORVUS_USER_LIB_FLAGS = $(CORVUSITOR_LDFLAGS)
_CORVUS_MODULE_PATH = $(BUILD_DIR)
_CORVUS_USER_SRC_FILES = $(CORVUSITOR_CXXFILES)
_CORVUS_TARGET = $(CORVUSITOR_TARGET)
_CORVUS_MAIN_SRC =
_CORVUS_CXX = clang++


CORVUSITOR_MK = $(CORVUSITOR_BUILD_DIR)/VCorvusTopWrapper_generated.mk
CORVUSITOR_HEADERS := $(EMU_HEADERS) $(shell find $(CORVUSITOR_CSRC_DIR) -name "*.h")

include $(CORVUSITOR_MK)

corvusitor-emu: $(CORVUSITOR_TARGET)

corvusitor-clean-obj:
	rm -f $(CORVUSITOR_BUILD_DIR)/*.o $(CORVUSITOR_BUILD_DIR)/*.gch $(CORVUSITOR_BUILD_DIR)/*.a $(CORVUSITOR_TARGET)

.PHONY: corvusitor-clean-obj

endif
