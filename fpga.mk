FPGA_TARGET = $(BUILD_DIR)/fpga-host
FPGA_CSRC_DIR   = $(abspath ./src/test/csrc/fpga)
FPGA_CONFIG_DIR = $(abspath ./config) # Reserve storage for xdma configuration

FPGA_CXXFILES  = $(SIM_CXXFILES) $(shell find $(FPGA_CSRC_DIR) -name "*.cpp")
FPGA_CXXFLAGS  = $(subst \\\",\", $(SIM_CXXFLAGS)) -I$(FPGA_CSRC_DIR) -O3
FPGA_LDFLAGS   = $(SIM_LDFLAGS) -lpthread -ldl

DMA_CHANNELS ?= 1
FPGA_CXXFLAGS += -DCONFIG_DMA_CHANNELS=$(DMA_CHANNELS) -DNUM_CORES=$(NUM_CORES) -DCONFIG_PLATFORM_FPGA

ifeq ($(SEMATIC_CHECKPOINT),1)
FPGA_CXXFLAGS += -DSEMATIC_CHECKPOINT
endif
fpga-build: fpga-clean fpga-host

$(FPGA_TARGET): $(FPGA_CXXFILES)
	$(CXX) $(FPGA_CXXFLAGS) $(FPGA_CXXFILES) -o $@ $(FPGA_LDFLAGS)

fpga-host: $(FPGA_TARGET)

fpga-clean:
	rm -f $(FPGA_TARGET)
