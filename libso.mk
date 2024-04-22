LIBDIFFTEST_SO    	 = $(BUILD_DIR)/libdifftest.so
CC_OBJ_DIR	 	 = $(abspath $(BUILD_DIR)/cc_obj)

PLDM_CSRC_DIR 	 = $(abspath ./src/test/csrc/vcs)
PLDM_CXXFILES 	 = $(SIM_CXXFILES) $(shell find $(PLDM_CSRC_DIR) -name "*.cpp")
PLDM_CXXFLAGS 	 = -m64 -c -fPIC -g -std=c++11
PLDM_CXXFLAGS 	+= $(subst \\\",\", $(SIM_CXXFLAGS)) -I$(PLDM_CSRC_DIR) -DNUM_CORES=$(NUM_CORES)

ifeq ($(WITH_DRAMSIM3),1)
LD_LIB  	 = -L $(DRAMSIM3_HOME)/ -ldramsim3 -Wl,-rpath-link=$(DRAMSIM3_HOME)/libdramsim3.so
endif

$(CC_OBJ_DIR):
	mkdir -p $(CC_OBJ_DIR)
difftest-so-pldm: $(CC_OBJ_DIR)
	cd $(CC_OBJ_DIR) 					&& \
	$(CC) $(PLDM_CXXFLAGS) $(PLDM_CXXFILES)			&& \
	$(CC) -o $(LIBDIFFTEST_SO) -m64 -shared *.o $(LD_LIB)