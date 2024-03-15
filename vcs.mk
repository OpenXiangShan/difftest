#***************************************************************************************
# Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2020-2021 Peng Cheng Laboratory
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

VCS_TARGET = simv

VCS_SIM_DIR = $(SIM_DIR)/rtl

VCS_CSRC_DIR = $(abspath ./src/test/csrc/vcs)
VCS_CXXFILES = $(SIM_CXXFILES) $(DIFFTEST_CXXFILES) $(GEN_CXXFILES) $(PLUGIN_DASM_CXXFILES)
VCS_CXXFILES += $(shell find $(VCS_CSRC_DIR) -name "*.cpp")

VCS_CXXFLAGS += -std=c++11 -static -Wall -I$(CFG_DIR) -I$(GEN_CSRC_DIR) -I$(VCS_CSRC_DIR) -I$(SIM_CSRC_DIR)
VCS_CXXFLAGS += -I$(DIFFTEST_CSRC_DIR) -I$(PLUGIN_DASM_DIR) -I$(PLUGIN_INC_DIR)
VCS_CXXFLAGS += -DNUM_CORES=$(NUM_CORES)
VCS_LDFLAGS  += -Wl,--no-as-needed -lpthread -lSDL2 -ldl -lz -lsqlite3

ifneq ($(REF),)
ifneq ($(wildcard $(REF)),)
VCS_CXXFLAGS += -DREF_PROXY=LinkedProxy -DLINKED_REFPROXY_LIB=\\\"$(REF)\\\"
VCS_LDFLAGS  += $(REF)
else
VCS_CXXFLAGS += -DREF_PROXY=$(REF)Proxy -DSELECTED$(REF)
REF_HOME_VAR = $(shell echo $(REF)_HOME | tr a-z A-Z)
ifneq ($(origin $(REF_HOME_VAR)), undefined)
VCS_CXXFLAGS += -DREF_HOME=\\\"$(shell echo $$$(REF_HOME_VAR))\\\"
endif
endif
endif

ifeq ($(RELEASE),1)
VCS_CXXFLAGS += -DBASIC_DIFFTEST_ONLY
VCS_FLAGS    += +define+SNPS_FAST_SIM_FFV +define+USE_RF_DEBUG
endif

# SparseMM
ifeq ($(CONFIG_USE_SPARSEMM), 1)
$(warning "NOTE: Sparse Memory is enable")
VCS_CXXFLAGS += -DCONFIG_USE_SPARSEMM
endif

ifeq ($(FCOV),on)
	VCS_FLAGS += +define+NANHUV3_FUNCOV
	VCS_FLAGS += $(FCOV_OPT)
endif

VCS_FLAGS += -cm_dir $(VCS_SIM_DIR)/comp/simv

ifeq ($(CCOV), on)
	VCS_FLAGS += -cm_dir $(VCS_SIM_DIR)/comp/simv
endif

# if fsdb is considered
# CONSIDER_FSDB ?= 0
ifeq ($(CONSIDER_FSDB),1)
EXTRA = +define+CONSIDER_FSDB
# if VERDI_HOME is not set automatically after 'module load', please set manually.
ifndef VERDI_HOME
$(error VERDI_HOME is not set. Try whereis verdi, abandon /bin/verdi and set VERID_HOME manually)
else
NOVAS_HOME = $(VERDI_HOME)
NOVAS = $(NOVAS_HOME)/share/PLI/VCS/LINUX64
EXTRA += -P $(NOVAS)/novas.tab $(NOVAS)/pli.a
endif
endif

VCS_VSRC_DIR = $(abspath ./src/test/vsrc/vcs)
VCS_VFILES   = $(SIM_VSRC) $(shell find $(VCS_VSRC_DIR) -name "*.v")
SIM_VSRC_DIR = $(abspath ./src/test/vsrc/common)
VCS_VFILES   += $(SIM_VSRC) $(shell find $(SIM_VSRC_DIR) -name "*.v")

VCS_SEARCH_DIR = $(abspath $(BUILD_DIR))

VCS_FLAGS += -full64 +v2k -timescale=1ns/1ns -sverilog -debug_access+all +lint=TFIPC-L -l vcs.log -top tb_top
VCS_FLAGS += -fgp -lca -kdb +nospecify +notimingcheck -xprop
# DiffTest
VCS_FLAGS += +define+DIFFTEST +define+PRINTF_COND=1 +define+VCS +define+SIM_TOP_MODULE_NAME=tb_top.sim
# C++ flags
VCS_FLAGS += -CFLAGS "$(VCS_CXXFLAGS)" -LDFLAGS "$(VCS_LDFLAGS)" -j200
# enable fsdb dump
VCS_FLAGS += $(EXTRA)

#ROT include file
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_dv_crypto_prince_ref_0.1
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_dv_secded_enc_0
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_prim_util_get_scramble_params_0/rtl
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_prim_util_memload_0/rtl
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_dv_scramble_model_0
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_dv_verilator_memutil_dpi_0/cpp
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_prim_assert_0.1/rtl
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_dv_verilator_memutil_dpi_scrambled_0/cpp
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_ip_sm3_1.0/rtl
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/zgc_ip_puf_1.0/rtl
ROT_include += +incdir+$(NOOP_HOME)/src/main/resources/TLROT/src/lowrisc_prim_macros_0.1/rtl/

SIM_FLIST := $(shell pwd)/sim_flist.f
$(VCS_TARGET): $(SIM_TOP_V) $(VCS_CXXFILES) $(VCS_VFILES) $(CFG_HEADERS)
	$(shell if [ ! -e $(VCS_SIM_DIR)/comp ];then mkdir -p $(VCS_SIM_DIR)/comp; fi)
	$(shell echo -f $(NOOP_HOME)/src/main/resources/TLROT/vcs_filelist > $(SIM_FLIST))
	$(shell echo -f $(BUILD_DIR)/cpu_flist.f >> $(SIM_FLIST))
	$(shell find $(VCS_VSRC_DIR) -name "*.v" >> $(SIM_FLIST))
	$(shell find $(SIM_VSRC_DIR) -name "*.v" -or -name "*.sv" >> $(SIM_FLIST))
	cp $(SIM_FLIST) $(VCS_SIM_DIR)/comp/
	cd $(VCS_SIM_DIR)/comp && vcs $(VCS_FLAGS) -f $(SIM_FLIST) $(VCS_CXXFILES) $(ROT_include)
