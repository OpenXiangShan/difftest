FPGA_TARGET = $(BUILD_DIR)/fpga-host
FPGA_CSRC_DIR   = $(abspath ./src/test/csrc/fpga)
FPGA_CONFIG_DIR = $(abspath ./config) # Reserve storage for xdma configuration

DMA_CHANNELS ?= 1
USE_SERIAL_PORT ?= 1

FPGA_CXXFILES  = $(SIM_CXXFILES) $(shell find $(FPGA_CSRC_DIR) -name "*.cpp" ! -name "gbus_transport.cpp")
FPGA_CXXFLAGS  = $(subst \\\",\", $(SIM_CXXFLAGS)) -I$(FPGA_CSRC_DIR) -DCONFIG_DMA_CHANNELS=$(DMA_CHANNELS) -DFPGA_HOST
FPGA_CXXFLAGS += -std=c++20 -O3 -flto -march=native -mtune=native
FPGA_LDFLAGS   = $(SIM_LDFLAGS) -lpthread -ldl

DIFFTEST_HOSTIF ?= XDMA
ifneq ($(filter XDMA GBUS,$(DIFFTEST_HOSTIF)), $(DIFFTEST_HOSTIF))
$(error DIFFTEST_HOSTIF must be XDMA or GBUS, got $(DIFFTEST_HOSTIF))
endif
FPGA_CXXFLAGS += -DDIFFTEST_HOSTIF_$(DIFFTEST_HOSTIF)

ifeq ($(DIFFTEST_HOSTIF),GBUS)
# Prefer the checked-in UVHS runtime so a checkout is reproducible.  Sites may
# still override this with an approved UVHS installation when the vendor
# runtime is supplied outside the repository.
GBUS_RUNTIME_ROOT ?= $(firstword $(wildcard ../env-scripts/fpga_diff/third_party/gbus_runtime /nfs/tools/UVHS/runtime_sw_service/export/gbus_runtime))
ifeq ($(strip $(GBUS_RUNTIME_ROOT)),)
$(error GBus runtime not found; set GBUS_RUNTIME_ROOT to a UVHS gbus_runtime directory)
endif
GBUS_HOST ?= localhost
FPGA_CXXFILES += $(FPGA_CSRC_DIR)/gbus_transport.cpp
FPGA_CXXFLAGS += -I$(GBUS_RUNTIME_ROOT)/include
FPGA_LDFLAGS += -L$(GBUS_RUNTIME_ROOT)/lib \
                -Wl,-rpath,$(abspath $(GBUS_RUNTIME_ROOT))/lib -luvgbus
endif

fpga-build: fpga-clean fpga-host

ifneq ($(FPGA_SIM), 1)
ifneq ($(USE_SERIAL_PORT), 0)
FPGA_CXXFLAGS += -DUSE_SERIAL_PORT
endif
endif

ifeq ($(USE_XDMA_DDR_LOAD), 1)
FPGA_CXXFLAGS += -DUSE_XDMA_DDR_LOAD
endif

ifeq ($(UVHS), 1)
FPGA_CXXFLAGS += -DUVHS
endif

ifeq ($(USE_XDMA_H2C), 1)
FPGA_CXXFLAGS += -DCONFIG_USE_XDMA_H2C
endif

ifeq ($(USE_THREAD_MEMPOOL), 1)
FPGA_CXXFLAGS += -DUSE_THREAD_MEMPOOL
endif

$(FPGA_TARGET): $(FPGA_CXXFILES)
	$(CXX) $(FPGA_CXXFLAGS) $(FPGA_CXXFILES) -o $@ $(FPGA_LDFLAGS)

fpga-host: $(FPGA_TARGET)

fpga-clean:
	rm -f $(FPGA_TARGET)

RELEASE_DIR ?= $(NOOP_HOME)
fpga-release:
	bash ./scripts/fpga/release.sh $(NOOP_HOME) $(RELEASE_DIR) $(RELEASE_SUFFIX)
