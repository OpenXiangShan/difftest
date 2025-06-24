PYTHON_DIR   = $(abspath $(BUILD_DIR)/xspdb/python)
CC_OBJ_DIR   = $(abspath $(BUILD_DIR)/xspdb/swig_obj)

picker_include = $(shell picker --show_xcom_lib_location_cpp|grep include|awk '{print $$2}')

LIB_SWIG_DIR	= $(abspath src/test/csrc/plugin/xspdb)
LIB_CXXFILES 	= $(SIM_CXXFILES) $(shell find $(LIB_SWIG_DIR)/cpp -name "*.cpp")
LIB_CXXFLAGS 	+= $(subst \\\",\", $(SIM_CXXFLAGS)) -DNUM_CORES=$(NUM_CORES)

ifeq ($(WITH_DRAMSIM3),1)
LD_LIB  	 = -L $(DRAMSIM3_HOME)/ -ldramsim3 -Wl,-rpath-link=$(DRAMSIM3_HOME)/libdramsim3.so
endif

LIB_CXXFLAGS += -I$(VCS_HOME)/include $(shell python3-config --includes) -I$(LIB_SWIG_DIR)/cpp
LD_LIB       += $(shell python3-config --ldflags)


SWIG_INCLUDE = $(filter -I%, $(LIB_CXXFLAGS)) -I$(picker_include)
ifeq ($(WITH_CHISELDB), 1)
  SWIG_D += -DENABLE_CHISEL_DB
endif

process_DiffTestState:
	rm -rf $(CC_OBJ_DIR) $(PYTHON_DIR)
	mkdir -p $(CC_OBJ_DIR)
	mkdir -p $(PYTHON_DIR)
	cp $(LIB_SWIG_DIR)/swig.i $(CC_OBJ_DIR)/python.i
	echo "%extend DiffTestState {" >> $(CC_OBJ_DIR)/python.i
	cat $(BUILD_DIR)/generated-src/diffstate.h|grep Difftest|grep "\["|sed "s/\[/\ /g"|sed "s/\]/\ /g"|awk '{print $$1 " *get_"$$2"(int index){if(index<"$$3"){return &(self->"$$2"[index]);} return NULL;}"}' >> $(CC_OBJ_DIR)/python.i
	echo "}" >> $(CC_OBJ_DIR)/python.i

swig_export: process_DiffTestState
	swig -c++ -outdir $(PYTHON_DIR) -o $(CC_OBJ_DIR)/difftest_wrap.cpp $(SWIG_INCLUDE) -python $(SWIG_D) $(CC_OBJ_DIR)/python.i
	$(eval LIB_CXXFILES = $(LIB_CXXFILES) $(CC_OBJ_DIR)/difftest_wrap.cpp)

difftest-python: swig_export
	cd $(CC_OBJ_DIR) 					&& \
	$(CC) $(LIB_CXXFLAGS) $(LIB_CXXFILES)			&& \
	$(CC) -o $(PYTHON_DIR)/_difftest.so -m64 -shared *.o $(LD_LIB) $(SIM_LDFLAGS)
	echo $(SIM_LDFLAGS) > $(PYTHON_DIR)/sim_ld_flags.txt
