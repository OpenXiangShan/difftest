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
DIFFTEST_CXXFILES := $(filter-out $(DIFFTEST_CSRC_DIR)/mma/worker/main.cpp,$(DIFFTEST_CXXFILES))

ifeq ($(MMA_BACKEND),cuda)
ifneq ($(NO_DIFF),1)
CUDA_MMA_WORKER = $(BUILD_DIR)/difftest-mma-worker
CUDA_MMA_WORKER_SRCS = $(DIFFTEST_CSRC_DIR)/mma/worker/main.cpp \
  $(addprefix $(DIFFTEST_CSRC_DIR)/mma/backend/,mma_backend.cpp mma_backend_cuda.cpp \
    mma_backend_cuda_impl.cpp mma_backend_cuda_process.cpp)
CUDA_MMA_WORKER_CXXFLAGS = $(subst \\\",\", $(SIM_CXXFLAGS))
CUDA_MMA_WORKER_OBJS = $(patsubst $(DIFFTEST_CSRC_DIR)/mma/%.cpp,$(BUILD_DIR)/mma-worker/%.o,$(CUDA_MMA_WORKER_SRCS))
CUDA_MMA_WORKER_LINKS = $(addsuffix /difftest-mma-worker,$(BUILD_DIR)/verilator-compile $(BUILD_DIR)/gsim-compile)
SIM_EXTRA_OBJS += $(CUDA_MMA_WORKER_LINKS)
$(abspath $(BUILD_DIR)/gsim-compile/emu): | $(SIM_EXTRA_OBJS)
$(addprefix $(BUILD_DIR)/,fpga-host ref-drive simv uvsim xsim libdifftest.so pldm-compile/libdpi_emu.so): | $(CUDA_MMA_WORKER) $(CUDA_MMA_BACKEND_OBJ)
vcs-build difftest-so: $(CUDA_MMA_WORKER) $(CUDA_MMA_BACKEND_OBJ)

$(BUILD_DIR)/mma-worker/%.o: $(DIFFTEST_CSRC_DIR)/mma/%.cpp $(DIFFTEST_STATE_HEADER) cuda.mk $(DIFFTEST_CSRC_DIR)/mma/worker/worker.mk
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -O3 -pthread $(CUDA_MMA_WORKER_CXXFLAGS) -MMD -MP -c $< -o $@

$(CUDA_MMA_WORKER): $(CUDA_MMA_WORKER_OBJS) $(CUDA_MMA_BACKEND_OBJ)
	$(CXX) -pthread $^ -L$(CUDA_LIB_DIR) -lcudart -o $@

# build/emu is a symlink; place the worker beside the real simulator as well.
$(CUDA_MMA_WORKER_LINKS): $(CUDA_MMA_WORKER)
	@mkdir -p $(@D)
	ln -sf ../difftest-mma-worker $@

-include $(CUDA_MMA_WORKER_OBJS:.o=.d)
endif
endif
