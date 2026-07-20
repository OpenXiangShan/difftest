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

DIFFTEST_STATE_HEADER = $(GEN_CSRC_DIR)/difftest-state.h
DIFFTEST_STATE_HAS_CUDA = $(shell grep -c '^#define CONFIG_DIFFTEST_MMA_CUDA' $(DIFFTEST_STATE_HEADER) 2> /dev/null)
USE_CUDA_MMA_BACKEND = $(or $(findstring C,$(DIFFTEST_CONFIG)),$(filter-out 0,$(DIFFTEST_STATE_HAS_CUDA)))
CUDA_MMA_BACKEND_SRC = $(DIFFTEST_CSRC_DIR)/mma/backend/mma_backend_cuda_kernel.cu
CUDA_MMA_BACKEND_OBJ = $(BUILD_DIR)/mma_backend_cuda_kernel.o
NVCC ?= nvcc
CUDA_MMA_BACKEND_HEADERS = $(DIFFTEST_CSRC_DIR)/mma/backend/mma_backend_cuda_impl.h \
	$(DIFFTEST_CSRC_DIR)/mma/backend/mma_backend_cute_model.h

# Optional CUDA toolchain detection for MMA backend.
# NOTE: strict-mode behavior is enforced in C++: CONFIG_DIFFTEST_MMA_CUDA
# without CONFIG_DIFFTEST_HAS_CUDA_TOOLCHAIN will trigger compile-time error.
CUDA_HOME ?= /usr/local/cuda
CUDA_INCLUDE_FILE = $(firstword $(wildcard $(CUDA_HOME)/include/cuda_runtime_api.h /usr/local/cuda/include/cuda_runtime_api.h /usr/include/cuda_runtime_api.h))
CUDA_LIB_FILE = $(firstword $(wildcard $(CUDA_HOME)/lib64/libcudart.so /usr/local/cuda/lib64/libcudart.so /usr/lib/x86_64-linux-gnu/libcudart.so /usr/lib64/libcudart.so))
CUDA_INCLUDE_DIR ?= $(patsubst %/,%,$(dir $(CUDA_INCLUDE_FILE)))
CUDA_LIB_DIR ?= $(patsubst %/,%,$(dir $(CUDA_LIB_FILE)))
ifneq ($(USE_CUDA_MMA_BACKEND),)
ifneq ($(wildcard $(CUDA_INCLUDE_DIR)/cuda_runtime_api.h),)
ifneq ($(wildcard $(CUDA_LIB_DIR)/libcudart.so),)
SIM_CXXFLAGS += -DCONFIG_DIFFTEST_HAS_CUDA_TOOLCHAIN -I$(CUDA_INCLUDE_DIR)
SIM_LDFLAGS  += -L$(CUDA_LIB_DIR) $(CUDA_MMA_BACKEND_OBJ) -lcudart
endif
endif

SIM_EXTRA_OBJS += $(CUDA_MMA_BACKEND_OBJ)

$(CUDA_MMA_BACKEND_OBJ): $(CUDA_MMA_BACKEND_SRC) $(CUDA_MMA_BACKEND_HEADERS) $(DIFFTEST_STATE_HEADER) | $(BUILD_DIR)
	$(NVCC) -std=c++11 -c $(CUDA_MMA_BACKEND_SRC) $(SIM_CXXFLAGS) -o $@
endif
