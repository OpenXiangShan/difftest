# tmp makefile to test fpga_main.cpp

TRACERTL_FPGAHOST_TARGET = $(BUILD_DIR)/tracertl-fpgahost

CONFIG_DMA_CHANNELS = 1

FPGA_SIM ?= 1
FPGA_TRACEINST ?= 1

TRACERTL_FPGAHOST_DIR = $(abspath ./src/test/csrc/tracertl-fpgahost)
TRACERTL_FPGA_CXX_FILES += $(shell find $(TRACERTL_FPGAHOST_DIR) -name "*.cpp")
TRACERTL_FPGA_CXX_FLAGS += -DCONFIG_DMA_CHANNELS=$(CONFIG_DMA_CHANNELS)

ifeq ($(FPGA_SIM), 1)
# TRACERTL_FPGA_CXX_FILES += fpga_sim.cpp
TRACERTL_FPGA_CXX_FLAGS += -DFPGA_SIM=1
endif

ifeq ($(FPGA_TRACEINST), 1)
TRACERTL_CXX_DIR = $(abspath ./src/test/csrc/tracertl)
TRACERTL_SPIKEDASM_DIR = $(abspath ./src/test/csrc/plugin/spikedasm)
TRACERTL_FPGA_CXX_FILES += $(shell find $(TRACERTL_CXX_DIR) -name "*.cpp")
TRACERTL_FPGA_CXX_FILES += $(shell find $(TRACERTL_SPIKEDASM_DIR) -name "*.cpp")

TRACERTL_FPGA_CXX_FLAGS += -DFPGA_TRACEINST=1
TRACERTL_FPGA_CXX_FLAGS += -I$(TRACERTL_SPIKEDASM_DIR) -I$(BUILD_DIR)/generated-src -I$(BUILD_DIR)/tracertl
TRACERTL_FPGA_CXX_FLAGS += -lzstd
endif

tracertl-fpgahost-compile:
	g++ $(TRACERTL_FPGA_CXX_FILES) $(TRACERTL_FPGA_CXX_FLAGS) -o $(TRACERTL_FPGAHOST_TARGET)

tracertl-fpgahost-transport:
	scp $(TRACERTL_FPGAHOST_TARGET) fpga-v@fpga:/home/fpga-v/zhangzifei/transport_files/

tracertl-fpgahost-clean:
	rm -f $(TRACERTL_FPGAHOST_TARGET)
