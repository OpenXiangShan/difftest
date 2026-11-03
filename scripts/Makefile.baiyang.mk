# Baiyang memory-controller backend.

BAIYANG_ADAPTER_HOME ?= $(abspath $(DESIGN_DIR)/src/main/baiyang)
ifeq ($(WITH_BAIYANG),1)
ifeq ($(wildcard $(BAIYANG_ADAPTER_HOME)/rtl/ddr_wrapper_baiyang.sv),)
$(error BAIYANG_ADAPTER_HOME is invalid: $(BAIYANG_ADAPTER_HOME))
endif
BAIYANG_RTL_DIR = $(BAIYANG_ADAPTER_HOME)/rtl
BAIYANG_HOME ?= $(abspath $(DESIGN_DIR)/YuQuan)
BAIYANG_BUILD_DIR = $(BAIYANG_HOME)/build
BAIYANG_VSRC = $(shell find $(BAIYANG_BUILD_DIR) \( -name "*.v" -o -name "*.sv" \) 2> /dev/null)
ifeq ($(wildcard $(BAIYANG_HOME)/Makefile),)
$(error BAIYANG_HOME is invalid: $(BAIYANG_HOME))
endif
ifeq ($(wildcard $(BAIYANG_BUILD_DIR)/mc_top.sv)$(wildcard $(BAIYANG_BUILD_DIR)/mc_top_baiyang.sv)$(wildcard $(BAIYANG_BUILD_DIR)/BY_mc_top.sv),)
$(error Baiyang generated RTL is missing. Run: make -C $(BAIYANG_HOME) verilog)
endif
SIM_VSRC += $(BAIYANG_RTL_DIR)/baiyang_axi_async_bridge.sv $(BAIYANG_RTL_DIR)/ddr_wrapper_baiyang.sv $(BAIYANG_VSRC)
SIM_CXXFLAGS += -DBAIYANG_DFI_CHANNELS=2 -DBAIYANG
BAIYANG_ADDRESS_MAPPING ?= $(BAIYANG_ADAPTER_HOME)/address_mapping_baiyang_dual.ini
SIM_VFLAGS += -I$(BAIYANG_RTL_DIR) -I$(BAIYANG_BUILD_DIR)
BAIYANG_DFI_CSRC_DIR = $(BAIYANG_ADAPTER_HOME)/src/dfi
BAIYANG_LOCAL_CSRC_DIR = $(abspath ./src/test/csrc/baiyang)
SIM_CXXFILES += $(filter-out $(BAIYANG_DFI_CSRC_DIR)/dfi_dpi_helper.cpp,$(shell find $(BAIYANG_DFI_CSRC_DIR) -name "*.cpp"))
SIM_CXXFILES += $(BAIYANG_LOCAL_CSRC_DIR)/dfi_dpi_helper.cpp
SIM_CXXFLAGS += -DWITH_BAIYANG -I$(BAIYANG_ADAPTER_HOME)/include
SIM_CXXFLAGS += -DBAIYANG_ADDRESS_MAPPING=\\\"$(BAIYANG_ADDRESS_MAPPING)\\\"
DIFFTEST_FIRST_COMMIT_LIMIT ?= 2000000ULL
SIM_VFLAGS += -Wno-PINMISSING -Wno-IMPLICIT -Wno-WIDTHCONCAT -Wno-UNOPTFLAT
endif
