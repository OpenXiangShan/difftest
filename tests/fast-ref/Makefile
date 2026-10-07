# Run from the DiffTest root after generating build/generated-src.
include Makefile
FAST_TEST_DIR := $(abspath tests/fast-ref)
FAST_TEST_OUT := $(BUILD_DIR)/fast-ref-tests
FAST_TEST_SOURCES := $(filter-out $(FPGA_CSRC_DIR)/fpga_main.cpp $(FPGA_CSRC_DIR)/xdma.cpp,$(FPGA_CXXFILES))
.PHONY: fast-ref-runtime-test fast-ref-pool-test fast-ref-xdma-test
fast-ref-runtime-test:
	mkdir -p $(FAST_TEST_OUT)
	$(CXX) $(FPGA_CXXFLAGS) $(FAST_TEST_SOURCES) $(FAST_TEST_DIR)/runtime_test.cpp -o $(FAST_TEST_OUT)/runtime-test $(FPGA_LDFLAGS)
fast-ref-pool-test:
	mkdir -p $(FAST_TEST_OUT)
	$(CXX) -std=c++17 -O2 -I$(SIM_CSRC_DIR) $(FAST_TEST_DIR)/rawpool_test.cpp -o $(FAST_TEST_OUT)/rawpool-test
	$(FAST_TEST_OUT)/rawpool-test
fast-ref-xdma-test:
	mkdir -p $(FAST_TEST_OUT)
	$(CXX) $(filter-out -flto,$(FPGA_CXXFLAGS)) -ffunction-sections -fdata-sections $(FAST_TEST_DIR)/xdma_test.cpp $(FPGA_CSRC_DIR)/xdma.cpp $(SIM_CSRC_DIR)/mpool.cpp -o $(FAST_TEST_OUT)/xdma-test -lpthread -Wl,--gc-sections -Wl,--wrap=open
	timeout 10 $(FAST_TEST_OUT)/xdma-test complete
	timeout 10 $(FAST_TEST_OUT)/xdma-test truncated
