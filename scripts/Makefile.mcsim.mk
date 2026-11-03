# MCSim timing backend.

ifeq ($(WITH_MCSIM),1)
ifndef MCSIM_HOME
$(error MCSIM_HOME is not set)
endif
MCSIM_LIB ?= $(MCSIM_HOME)/build-verilator-lib/libmcsim.a
MCSIM_CPU_FREQ_MHZ ?= 3000
ifeq ($(wildcard $(MCSIM_HOME)/include/mcsim_c_api.h),)
$(error MCSIM_HOME is invalid: $(MCSIM_HOME))
endif
ifeq ($(wildcard $(MCSIM_LIB)),)
$(error MCSim library is missing. Run: make -C $(MCSIM_HOME) verilator-lib)
endif
SIM_CXXFLAGS += -I$(MCSIM_HOME)/include -DWITH_MCSIM -DMCSIM_CPU_FREQ_MHZ=$(MCSIM_CPU_FREQ_MHZ)
SIM_LDFLAGS  += $(MCSIM_LIB)
endif
