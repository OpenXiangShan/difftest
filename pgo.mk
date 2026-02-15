#***************************************************************************************
# Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2025-2026 Beijing Institute of Open Source Chip
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


PGO_MAX_CYCLE ?= 100000
PGO_EMU_ARGS ?= --no-diff

LLVM_BOLT ?= $(shell readlink -f `command -v llvm-bolt 2> /dev/null`)
# We use readlink -f to get the absolute path of llvm-bolt, it's
# needed since we use argv[0]/../lib/libbolt_rt_instr.a as the path
# to find the runtime library inside llvm-bolt. It's a workaround
# for LLVM before commit abc2eae682("[BOLT] Enable standalone build (llvm#97130)").
# Ref: https://github.com/llvm/llvm-project/blob/release/18.x/bolt/lib/RuntimeLibs/RuntimeLibrary.cpp#L33
# Ref: https://github.com/llvm/llvm-project/blob/release/18.x/bolt/tools/driver/llvm-bolt.cpp#L187
PGO_BOLT ?= $(shell if [ -x "$(LLVM_BOLT)" ]; then echo 1; else echo 0; fi)

PGO_EMU_DIR ?= $(BUILD_DIR)
PGO_DIR = $(PGO_EMU_DIR)/pgo
PGO_RUN_OPT ?= -i $(PGO_WORKLOAD) --max-cycles=$(PGO_MAX_CYCLE) $(PGO_EMU_ARGS)
PGO_RUN_OPT += 	1>$(PGO_DIR)/`date +%s`.log \
				2>$(PGO_DIR)/`date +%s`.err


pgo-build:
ifeq ($(and $(PGO_WORKLOAD),$(PGO_CLEAN_OBJ),$(PGO_BUILD_TARGET),$(PGO_TARGET)),)
	$(error PGO_WORKLOAD, PGO_CLEAN_OBJ, PGO_BUILD_TARGET and PGO_TARGET must all be set)
endif
ifneq ($(PGO_BOLT),1)
	@echo "If available, please use install llvm-bolt for much reduced PGO build time."
	@echo "Building PGO profile..."
	@stat $(PGO_WORKLOAD) > /dev/null
	@$(MAKE) $(PGO_CLEAN_OBJ)
	@mkdir -p $(PGO_DIR)
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
	@$(MAKE) $(PGO_BUILD_TARGET) \
					   PGO_CFLAGS="-fprofile-generate=$(PGO_DIR)" \
					   PGO_LDFLAGS="-fprofile-generate=$(PGO_DIR)"
	@echo "Training emu with PGO Workload..."
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
	$(PGO_TARGET) $(PGO_RUN_OPT)
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
ifdef LLVM_PROFDATA
# When using LLVM's profile-guided optimization, the raw data can not
# directly be used in -fprofile-use. We need to use a specific version of
# llvm-profdata. This happens when verilator compiled with CC=clang
# CXX=clang++. In this case, we should add LLVM_PROFDATA=llvm-profdata
# when calling make. For GCC, this step should be skipped. Also, some
# machines may have multiple versions of llvm-profdata. So please never
# add default value for LLVM_PROFDATA unless we have a proper way to probe
# the compiler and the corresponding llvm-profdata value.
	$(LLVM_PROFDATA) merge $(PGO_DIR)/*.profraw -o $(PGO_DIR)/default.profdata
else # ifdef LLVM_PROFDATA
	@echo ""
	@echo "----------------------- NOTICE BEGIN -----------------------"
	@echo "If your verilator is compiled with LLVM, please don't forget"
	@echo "to add LLVM_PROFDATA=llvm-profdata when calling make."
	@echo ""
	@echo "If your verilator is compiled with GCC, please ignore this"
	@echo "message and NEVER adding LLVM_PROFDATA when calling make."
	@echo "----------------------- NOTICE  END  -----------------------"
	@echo ""
endif # ifdef LLVM_PROFDATA
	@echo "Building emu with PGO profile..."
	@$(MAKE) $(PGO_CLEAN_OBJ)
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
	@$(MAKE) $(PGO_BUILD_TARGET) \
					   PGO_CFLAGS="-fprofile-use=$(PGO_DIR)" \
					   PGO_LDFLAGS="-fprofile-use=$(PGO_DIR)"
else # ifneq ($(PGO_BOLT),1)
	@echo "Building emu..."
	@$(MAKE) $(PGO_BUILD_TARGET) PGO_LDFLAGS="-Wl,--emit-relocs -fuse-ld=bfd"
	@mv $(PGO_TARGET) $(PGO_TARGET).pre-bolt
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
	@echo "Training emu with PGO Workload..."
	@mkdir -p $(PGO_DIR)
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
	@((perf record -j any,u -o $(PGO_DIR)/perf.data -- sh -c "\
		$(PGO_TARGET).pre-bolt $(PGO_RUN_OPT) || true") && \
		(perf2bolt -p=$(PGO_DIR)/perf.data -o=$(PGO_DIR)/perf.fdata $(PGO_TARGET).pre-bolt || true)) || \
		(echo -e "\033[31mlinux-perf is not available, fallback to instrumentation-based PGO\033[0m" && \
		$(LLVM_BOLT) $(PGO_TARGET).pre-bolt \
			-instrument --instrumentation-file=$(PGO_DIR)/perf.fdata \
			-o $(PGO_DIR)/emu.instrumented && \
		($(PGO_DIR)/emu.instrumented $(PGO_RUN_OPT) || true))
	@echo "Processing BOLT profile data..."
	@$(LLVM_BOLT) $(PGO_TARGET).pre-bolt -o $(PGO_TARGET) -data=$(PGO_DIR)/perf.fdata -reorder-blocks=ext-tsp || \
		(echo -e "\033[31mBOLT optimization failed, fallback to non-PGO build\033[0m" && \
		mv $(PGO_TARGET).pre-bolt $(PGO_TARGET))
endif # ifneq ($(PGO_BOLT),1)
	@sync -d $(BUILD_DIR) -d $(PGO_EMU_DIR)
