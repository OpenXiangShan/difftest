LIBDIFFTEST_SO   = $(BUILD_DIR)/libdifftest.so
CC_OBJ_DIR	 = $(abspath $(BUILD_DIR)/cc_obj)

LIB_CSRC_DIR 	 = $(abspath ./src/test/csrc/vcs)
LIB_CXXFILES 	 = $(SIM_CXXFILES) $(shell find $(LIB_CSRC_DIR) -name "*.cpp")
LIB_CXXFLAGS 	 = -m64 -c -fPIC -g -std=c++11
LIB_CXXFLAGS 	+= $(subst \\\",\", $(SIM_CXXFLAGS)) $(SIM_LDFLAGS) -I$(LIB_CSRC_DIR)

ifeq ($(WITH_DRAMSIM3),1)
LD_LIB  	 = -L $(DRAMSIM3_HOME)/ -ldramsim3 -Wl,-rpath-link=$(DRAMSIM3_HOME)/libdramsim3.so
endif

# Include headers that are implicitly included by RTL simulators
# Priority: (1) VCS, (2) Verilator
ifneq ($(VCS_HOME),)
LIB_CXXFLAGS += -I$(VCS_HOME)/include
else
VERILATOR_ROOT ?= $(shell verilator --getenv VERILATOR_ROOT 2> /dev/null)
ifneq ($(VERILATOR_ROOT),)
LIB_CXXFLAGS += -I$(VERILATOR_ROOT)/include/vltstd
else
LIBSO_WARNING_MISSING_HEADERS = 1
endif
endif

$(CC_OBJ_DIR):
	mkdir -p $(CC_OBJ_DIR)

difftest-so: $(CC_OBJ_DIR)
ifeq ($(LIBSO_WARNING_MISSING_HEADERS),1)
	@echo "No available RTL simulators. Headers may be missing. Still try to build it."
endif
	cd $(CC_OBJ_DIR) 					&& \
	$(CC) $(LIB_CXXFLAGS) $(LIB_CXXFILES)			&& \
	$(CC) -o $(LIBDIFFTEST_SO) -m64 -shared *.o $(LD_LIB)
