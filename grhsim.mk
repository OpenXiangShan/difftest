#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

GRHSIM_EMU_BUILD_DIR = $(abspath $(BUILD_DIR)/grhsim-compile)
GRHSIM_EMU_TARGET = $(abspath $(GRHSIM_EMU_BUILD_DIR)/emu)
GRHSIM_CSRC_DIR = $(abspath ./src/test/csrc/grhsim)
GRHSIM_MODEL_DIR ?=

ifeq ($(strip $(GRHSIM_MODEL_DIR)),)

grhsim-build-emu:
	@echo "GRHSIM_MODEL_DIR is not set."
	@false

grhsim-emu: grhsim-build-emu

grhsim-clean-obj:
	@true

else

GRHSIM_MODEL_DIR_ABS := $(abspath $(GRHSIM_MODEL_DIR))
GRHSIM_MODEL_HEADER := $(GRHSIM_MODEL_DIR_ABS)/grhsim_$(SIM_TOP).hpp
GRHSIM_MODEL_MAKEFILE := $(GRHSIM_MODEL_DIR_ABS)/Makefile
GRHSIM_MODEL_LIB := $(GRHSIM_MODEL_DIR_ABS)/libgrhsim_$(SIM_TOP).a
GRHSIM_MODEL_SRCS := $(sort $(wildcard $(GRHSIM_MODEL_DIR_ABS)/*.cpp) $(wildcard $(GRHSIM_MODEL_DIR_ABS)/*.hpp))
GRHSIM_MODEL_SELECT_HEADER := $(GRHSIM_EMU_BUILD_DIR)/grhsim_model_select.hpp
GRHSIM_MODEL_BUILD_JOBS ?= $(if $(strip $(VM_BUILD_JOBS)),$(VM_BUILD_JOBS),64)
GRHSIM_MODEL_MAKE_J := -j $(GRHSIM_MODEL_BUILD_JOBS)
GRHSIM_MODEL_CXXFLAGS ?= -std=c++20 -O3

GRHSIM_CXX = $(CXX)
GRHSIM_CXXFILES = $(EMU_CXXFILES) $(shell find $(GRHSIM_CSRC_DIR) -name "*.cpp")
GRHSIM_CXXFLAGS = $(subst \\\",\", $(EMU_CXXFLAGS))
GRHSIM_CXXFLAGS += -I$(GRHSIM_CSRC_DIR) -I$(GRHSIM_MODEL_DIR_ABS) -I$(GRHSIM_EMU_BUILD_DIR) -DGRHSIM --std=c++20
GRHSIM_CXXFLAGS += -I$(abspath ../../../wolvrix/external/libfst/src)
GRHSIM_CXXFLAGS += -DWOLVRIX_GRHSIM_WAVEFORM=$(WOLVRIX_GRHSIM_WAVEFORM)
GRHSIM_CXXFLAGS += $(EMU_OPTIMIZE) $(PGO_CFLAGS)
GRHSIM_LDFLAGS = $(SIM_LDFLAGS) -ldl $(PGO_LDFLAGS)

# $(1): object file
# $(2): source file
# $(3): compile flags
# $(4): object file list
# $(5): extra dependency
define GRHSIM_CXX_TEMPLATE =
$(1): $(2) $(THIS_MAKEFILE) $(5)
	@mkdir -p $$(@D) && echo + CXX $$<
	@$(GRHSIM_CXX) $$< $(3) -c -o $$@
$(4) += $(1)
endef

# $(1): object file
# $(2): source file
# $(3): ld flags
define GRHSIM_LD_TEMPLATE =
$(1): $(2) $(GRHSIM_MODEL_LIB)
	@mkdir -p $$(@D) && echo + LD $$@
	@$(GRHSIM_CXX) $$^ $(3) -o $$@
endef

$(foreach x,$(GRHSIM_CXXFILES),$(eval \
	$(call GRHSIM_CXX_TEMPLATE,$(GRHSIM_EMU_BUILD_DIR)/other/$(basename $(notdir $(x))).o,$(x),$(GRHSIM_CXXFLAGS),GRHSIM_EMU_OBJS,$(GRHSIM_MODEL_SELECT_HEADER))))

$(eval $(call GRHSIM_LD_TEMPLATE,$(GRHSIM_EMU_TARGET),$(GRHSIM_EMU_OBJS),$(GRHSIM_LDFLAGS)))

$(GRHSIM_MODEL_SELECT_HEADER): $(GRHSIM_MODEL_HEADER)
	@mkdir -p $(@D)
	@printf '%s\n' '#pragma once' '#include "grhsim_$(SIM_TOP).hpp"' 'using GrhSIMModel = GrhSIM_$(SIM_TOP);' > $@

$(GRHSIM_MODEL_LIB): $(GRHSIM_MODEL_MAKEFILE) $(GRHSIM_MODEL_HEADER) $(GRHSIM_MODEL_SRCS)
	$(MAKE) $(GRHSIM_MODEL_MAKE_J) -C $(GRHSIM_MODEL_DIR_ABS) \
		CXX="$(GRHSIM_CXX)" \
		AR="$(AR)" \
		ARFLAGS="$(ARFLAGS)" \
		CXXFLAGS="$(GRHSIM_MODEL_CXXFLAGS)"

grhsim-build-emu: $(GRHSIM_EMU_TARGET)

grhsim-emu: grhsim-build-emu

grhsim-clean-obj:
	rm -rf $(GRHSIM_EMU_BUILD_DIR)

endif

.PHONY: grhsim-build-emu grhsim-clean-obj
