PLDM_TB_TOP  		 = tb_top
PLDM_TOP_MODULE 	 = $(SIM_TOP)

PLDM_BUILD_DIR 		 = $(abspath $(BUILD_DIR)/pldm-compile)
PLDM_CC_OBJ_DIR	 	 = $(abspath $(PLDM_BUILD_DIR)/cc_obj)

# Macro Flags
PLDM_MACRO_FLAGS  	 = +define+TOP_MODULE=$(PLDM_TOP_MODULE)
PLDM_MACRO_FLAGS 	+= +define+PALLADIUM
PLDM_MACRO_FLAGS 	+= +define+RANDOMIZE_GARBAGE_ASSIGN
PLDM_MACRO_FLAGS 	+= +define+RANDOMIZE_INVALID_ASSIGN
PLDM_MACRO_FLAGS 	+= +define+RANDOMIZE_MEM_INIT
PLDM_MACRO_FLAGS 	+= +define+RANDOMIZE_REG_INIT
PLDM_MACRO_FLAGS 	+= +define+RANDOMIZE_DELAY=0
ifeq ($(RELEASE_WITH_ASSERT), 1)
PLDM_MACRO_FLAGS 	+= +define+SYNTHESIS +define+TB_NO_DPIC
else
PLDM_MACRO_FLAGS 	+= +define+DIFFTEST +define+TB_DPIC_NONBLOCK
endif
PLDM_MACRO_FLAGS 	+= $(PLDM_EXTRA_MACRO)

# UA Args
IXCOM_FLAGS  	 = -clean -64 -ua +sv +ignoreSimVerCheck +xe_alt_xlm
ifeq ($(RELEASE_WITH_ASSERT), 1)
IXCOM_FLAGS 	+= +1xua
else
IXCOM_FLAGS 	+= +iscDelay+tb_top +iscDelay+SimJTAG -enableLargeSizeMem
endif

# Compiler Args
IXCOM_FLAGS 	+= -xecompile compilerOptions=$(PLDM_SCRIPTS_DIR)/compilerOptions.qel
IXCOM_FLAGS 	+= +tb_import_systf+fwrite +tb_import_systf+fflush
IXCOM_FLAGS 	+= $(PLDM_MACRO_FLAGS)
IXCOM_FLAGS 	+= +dut+$(PLDM_TB_TOP)
ifeq ($(RELEASE_WITH_ASSERT), 1)
PLDM_CLOCK	 = clock_gen
PLDM_CLOCK_DEF 	 = $(PLDM_SCRIPTS_DIR)/$(PLDM_CLOCK).xel
PLDM_CLOCK_SRC 	 = $(PLDM_BUILD_DIR)/$(PLDM_CLOCK).sv
IXCOM_FLAGS    	+= +dut+$(PLDM_CLOCK) $(PLDM_CLOCK_SRC)
endif

# Other Args
IXCOM_FLAGS 	+= -v $(PLDM_IXCOM)/IXCclkgen.sv
IXCOM_FLAGS 	+= +iscdisp+Rob +iscdisp+tb_top +iscdisp+MemRWHelper
ifneq ($(RELEASE_WITH_ASSERT), 1)
IXCOM_FLAGS 	+= +rtlCommentPragma +tran_relax -relativeIXCDIR -rtlNameForGenerate
endif
IXCOM_FLAGS 	+= +tfconfig+$(PLDM_SCRIPTS_DIR)/argConfigs.qel

# Verilog Files
PLDM_VSRC_DIR  	 = $(RTL_DIR)
ifeq ($(RELEASE_WITH_ASSERT), 1)
PLDM_VSRC_DIR 	+= $(abspath ./src/test/vsrc/vcs) $(abspath ./src/test/vsrc/common/SimJTAG.v)
else
PLDM_VSRC_DIR 	+= $(abspath ./src/test/vsrc)
endif
PLDM_VFILELIST 	 = $(PLDM_BUILD_DIR)/vfiles.f
IXCOM_FLAGS   	+= -F $(PLDM_VFILELIST)

# VLAN Flags
ifneq ($(RELEASE_WITH_ASSERT), 1)
VLAN_FLAGS 	 = -64 -sv
VLAN_FLAGS 	+= $(addprefix -incdir , $(PLDM_VSRC_DIR))
VLAN_FLAGS	+= -vtimescale 1ns/1ns
VLAN_FLAGS 	+= $(PLDM_MACRO_FLAGS)
VLAN_FLAGS 	+= -F $(PLDM_VFILELIST)
endif

# SoftWare Compile
PLDM_SIMTOOL  	 = $(shell cds_root xrun)/tools/include
PLDM_IXCOM 	 = $(shell cds_root ixcom)/share/uxe/etc/ixcom
DPILIB_EMU    	 = $(PLDM_BUILD_DIR)/libdpi_emu.so
PLDM_CSRC_DIR 	 = $(abspath ./src/test/csrc/vcs)
PLDM_CXXFILES 	 = $(SIM_CXXFILES) $(shell find $(PLDM_CSRC_DIR) -name "*.cpp")
PLDM_CXXFLAGS 	 = -m64 -c -fPIC -g -std=c++11 -I$(PLDM_IXCOM) -I$(PLDM_SIMTOOL)
PLDM_CXXFLAGS 	+= $(SIM_CXXFLAGS) -I$(PLDM_CSRC_DIR) -DNUM_CORES=$(NUM_CORES) -DTB_DEFERRED_RESULT

# XMSIM Flags
XMSIM_FLAGS 	 = --xmsim -64 +xcprof -profile -PROFTHREAD
ifneq ($(RELEASE_WITH_ASSERT), 1)
XMSIM_FLAGS 	+= -sv_lib ${DPILIB_EMU}
endif
XMSIM_FLAGS 	+= $(PLDM_EXTRA_ARGS)
XMSIM_FLAGS 	+= --

$(PLDM_BUILD_DIR):
	mkdir -p $(PLDM_BUILD_DIR)

$(PLDM_CC_OBJ_DIR):
	mkdir -p $(PLDM_CC_OBJ_DIR)

$(PLDM_VFILELIST):
	find $(PLDM_VSRC_DIR) -name "*.v" -or -name "*.sv" >> $(PLDM_VFILELIST)

$(PLDM_CLOCK_SRC): $(PLDM_CLOCK_DEF)
	ixclkgen -input $(PLDM_CLOCK_DEF) -output $(PLDM_CLOCK_SRC) -module $(PLDM_CLOCK) -hierarchy "$(PLDM_TB_TOP)."


ifeq ($(RELEASE_WITH_ASSERT), 1)
pldm-build: $(PLDM_BUILD_DIR) $(PLDM_VFILELIST) $(PLDM_CLOCK_SRC)
	cd $(PLDM_BUILD_DIR) 					&& \
	ixcom $(IXCOM_FLAGS) -l $(PLDM_BUILD_DIR)/ixcom.log
else
pldm-build: $(PLDM_BUILD_DIR) $(PLDM_VFILELIST) $(PLDM_CC_OBJ_DIR)
	cd $(PLDM_BUILD_DIR) 					&& \
	vlan $(VLAN_FLAGS) -l $(PLDM_BUILD_DIR)/vlan.log	&& \
	ixcom $(IXCOM_FLAGS) -l $(PLDM_BUILD_DIR)/ixcom.log	&& \
	cd $(PLDM_CC_OBJ_DIR) 					&& \
	$(CC) $(PLDM_CXXFLAGS) $(PLDM_CXXFILES)			&& \
	$(CC) -o $(DPILIB_EMU) -m64 -shared *.o
endif

pldm-run: $(PLDM_BUILD_DIR)
	cd $(PLDM_BUILD_DIR) 					&& \
	xeDebug $(XMSIM_FLAGS) -input $(PLDM_SCRIPTS_DIR)/run.tcl -l run-$$(date +%Y%m%d-%H%M%S).log

pldm-debug: $(PLDM_BUILD_DIR)
	cd $(PLDM_BUILD_DIR) 					&& \
	xeDebug $(XMSIM_FLAGS) -gui -xedebugargs -fsdb -input $(PLDM_SCRIPTS_DIR)/run_debug.tcl -l debug-$$(date +%Y%m%d-%H%M%S).log

pldm-clean:
	rm -rf $(PLDM_BUILD_DIR)
