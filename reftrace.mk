REFTRACE_TARGET = $(BUILD_DIR)/ref-drive
REFTRACE_CSRC_DIR = $(abspath ./src/test/csrc/reftrace)
REFTRACE_CXXFILES = $(filter-out $(wildcard $(GEN_CSRC_DIR)/*.cpp),$(SIM_CXXFILES)) $(shell find $(REFTRACE_CSRC_DIR) -name "*.cpp")
REFTRACE_HEADERS = $(shell find $(SIM_CSRC_DIR) $(DIFFTEST_CSRC_DIR) $(SIM_CONFIG_DIR) $(GEN_CSRC_DIR) $(REFTRACE_CSRC_DIR) -name '*.h')
REFTRACE_CXXFLAGS = $(subst \\\",\", $(SIM_CXXFLAGS)) -DCONFIG_DIFFTEST_FPGA= -DCONFIG_REF_DRIVE
REFTRACE_CXXFLAGS += -DCONFIG_DIFFTEST_FAST_REF -ffunction-sections -fdata-sections

$(REFTRACE_TARGET): $(REFTRACE_CXXFILES) $(REFTRACE_HEADERS) reftrace.mk
	@mkdir -p $(@D)
	$(CXX) $(REFTRACE_CXXFLAGS) -std=c++20 -O2 $(REFTRACE_CXXFILES) -o $@ $(SIM_LDFLAGS) -lpthread -ldl -Wl,--gc-sections

.PHONY: ref-drive
ref-drive: $(REFTRACE_TARGET)
