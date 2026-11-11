#***************************************************************************************
# Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
#
# DiffTest is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************

# The standalone worker entry must not be linked into simulators.
DIFFTEST_CXXFILES := $(filter-out $(DIFFTEST_CSRC_DIR)/mma/worker/%,$(DIFFTEST_CXXFILES))

ifneq ($(NO_DIFF),1)
ifeq ($(MMA_BACKEND),cuda)
MMA_WORKER_SRCS = $(DIFFTEST_CSRC_DIR)/mma/worker/main.cpp $(DIFFTEST_CSRC_DIR)/mma/worker/worker.cpp \
  $(addprefix $(DIFFTEST_CSRC_DIR)/mma/backend/,mma_backend.cpp mma_backend_cuda.cpp \
    mma_backend_cuda_impl.cpp)
MMA_WORKER_EXTRA_OBJS = $(CUDA_MMA_BACKEND_OBJ)
MMA_WORKER_LDFLAGS = -L$(CUDA_LIB_DIR) -lcudart
else ifneq ($(MMA_ASYNC),0)
MMA_WORKER_SRCS = $(DIFFTEST_CSRC_DIR)/mma/worker/main.cpp $(DIFFTEST_CSRC_DIR)/mma/worker/worker.cpp \
  $(addprefix $(DIFFTEST_CSRC_DIR)/mma/backend/,mma_backend.cpp mma_backend_cpu.cpp)
endif

ifneq ($(MMA_WORKER_SRCS),)
MMA_WORKER = $(BUILD_DIR)/difftest-mma-worker
MMA_WORKER_BINARY = $(BUILD_DIR)/mma-worker/$(MMA_BACKEND)/difftest-mma-worker
MMA_WORKER_CXXFLAGS = $(subst \\\",\", $(SIM_CXXFLAGS))
MMA_WORKER_OBJS = $(patsubst $(DIFFTEST_CSRC_DIR)/mma/%.cpp,$(BUILD_DIR)/mma-worker/$(MMA_BACKEND)/%.o,$(MMA_WORKER_SRCS))
MMA_WORKER_LINKS = $(addsuffix /difftest-mma-worker,$(BUILD_DIR)/verilator-compile $(BUILD_DIR)/gsim-compile)
SIM_EXTRA_OBJS += $(MMA_WORKER_LINKS)
$(abspath $(BUILD_DIR)/gsim-compile/emu): | $(SIM_EXTRA_OBJS)
$(addprefix $(BUILD_DIR)/,fpga-host ref-drive simv uvsim xsim libdifftest.so pldm-compile/libdpi_emu.so): | $(MMA_WORKER) $(MMA_WORKER_EXTRA_OBJS)
vcs-build difftest-so: $(MMA_WORKER) $(MMA_WORKER_EXTRA_OBJS)

$(BUILD_DIR)/mma-worker/$(MMA_BACKEND)/%.o: $(DIFFTEST_CSRC_DIR)/mma/%.cpp $(DIFFTEST_STATE_HEADER) cuda.mk $(DIFFTEST_CSRC_DIR)/mma/worker/worker.mk
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -O3 -pthread $(MMA_WORKER_CXXFLAGS) -MMD -MP -c $< -o $@

$(MMA_WORKER_BINARY): $(MMA_WORKER_OBJS) $(MMA_WORKER_EXTRA_OBJS)
	$(CXX) -pthread $^ $(MMA_WORKER_LDFLAGS) -o $@

.PHONY: mma-worker-select
$(MMA_WORKER): $(MMA_WORKER_BINARY) | mma-worker-select
mma-worker-select: $(MMA_WORKER_BINARY)
	@test "$$(readlink "$(MMA_WORKER)")" = "mma-worker/$(MMA_BACKEND)/difftest-mma-worker" || \
	  ln -sf mma-worker/$(MMA_BACKEND)/difftest-mma-worker $(MMA_WORKER)

# build/emu is a symlink; place the worker beside the real simulator as well.
$(MMA_WORKER_LINKS): $(MMA_WORKER)
	@mkdir -p $(@D)
	ln -sf ../difftest-mma-worker $@

-include $(MMA_WORKER_OBJS:.o=.d)
endif
endif
