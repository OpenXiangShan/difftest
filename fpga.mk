
FPGA           = FPGA_HOST
FPGA_TARGET    = $(abspath $(BUILD_DIR)/simv)
FPGA_BUILD_DIR = $(abspath $(BUILD_DIR)/simv-compile)
FPGA_RUN_DIR   = $(abspath $(BUILD_DIR)/$(notdir $(RUN_BIN)))

FPGA_CSRC_DIR   = $(abspath ./src/test/csrc/fpga)
FPGA_CONFIG_DIR = $(abspath ./config)

FPGA_CXXFILES  = $(SIM_CXXFILES) $(shell find $(FPGA_CSRC_DIR) -name "*.cpp")
FPGA_CXXFLAGS  = $(subst \\\",\", $(SIM_CXXFLAGS)) -I$(FPGA_CSRC_DIR) -DNUM_CORES=$(NUM_CORES)
FPGA_LDFLAGS   = $(SIM_LDFLAGS) -lpthread -ldl

fpga-build: fpga-clean fpga-host

fpga-host:
	$(CXX) $(FPGA_CXXFLAGS) $(FPGA_CXXFILES) $^ -o $@ $(FPGA_LDFLAGS)
fpga-clean:
	rm -f fpga-host
