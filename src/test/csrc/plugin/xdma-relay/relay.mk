XDMA_RELAY_PLUGIN_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
FPGA_CXXFILES += $(XDMA_RELAY_PLUGIN_DIR)/xdma_relay.cpp
FPGA_CXXFLAGS += -I$(XDMA_RELAY_PLUGIN_DIR)

# The board daemon does not depend on a generated CPU profile.
FPGA_RELAY_TARGET = $(BUILD_DIR)/fpga-xdma-relay
$(FPGA_RELAY_TARGET): $(XDMA_RELAY_PLUGIN_DIR)/daemon.cc $(XDMA_RELAY_PLUGIN_DIR)/wire.h
	mkdir -p $(dir $@)
	$(CXX) -std=c++20 -O3 -pthread $< -o $@ -lzstd

.PHONY: fpga-xdma-relay
fpga-xdma-relay: $(FPGA_RELAY_TARGET)
