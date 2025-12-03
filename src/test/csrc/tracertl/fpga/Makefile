# tmp makefile to test fpga_main.cpp

CONFIG_DMA_CHANNELS = 1

FPGA_SIM ?= 1
FPGA_TRACEINST ?= 1

CXX_FILES += fpga_main.cpp xdma.cpp
CXX_FLAGS += -DCONFIG_DMA_CHANNELS=$(CONFIG_DMA_CHANNELS)

ifeq ($(FPGA_SIM), 1)
CXX_FILES += fpga_sim.cpp
CXX_FLAGS += -DFPGA_SIM=1
endif

ifeq ($(FPGA_TRACEINST), 1)
CXX_FLAGS += -DFPGA_TRACEINST=1
CXX_FILES += $(shell find ../ -name "*.cpp" | grep -v "fpga_main.cpp" | grep -v "xdma.cpp" | grep -v "fpga_sim.cpp")
# CXX_FILES += $(shell find ../ -name "*.cpp" | grep -v "fpga_main.cpp" | grep -v "xdma.cpp")
CXX_FILES += ../../plugin/spikedasm/spikedasm.cpp
CXX_FLAGS += -I../../plugin/spikedasm -I../../../../../../build/generated-src -I ../../../../../../build/tracertl
CXX_FLAGS += -lzstd
endif

compile:
	g++ $(CXX_FILES) -o fpga_main $(CXX_FLAGS)

transport:
	scp fpga_main fpga-v@fpga:/home/fpga-v/zhangzifei/transport_files/

clean:
	rm fpga_main
