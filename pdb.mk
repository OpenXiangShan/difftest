PYTHON_DIR       = $(abspath $(BUILD_DIR)/xspdb/python)
PDB_OBJ_DI       = $(abspath $(BUILD_DIR)/xspdb/swig_obj)

picker_include   = $(shell picker --show_xcom_lib_location_cpp|grep include|awk '{print $$2}')

PDB_SWIG_DIR     = $(abspath src/test/csrc/plugin/xspdb)
PDB_CXXFILES     = $(SIM_CXXFILES) $(shell find $(PDB_SWIG_DIR)/cpp -name "*.cpp") $(shell find $(SIM_CONFIG_DIR) -name "*.cpp")
PDB_CXXFLAGS    += $(subst \\\",\", $(SIM_CXXFLAGS)) -DNUM_CORES=$(NUM_CORES)

ifeq ($(WITH_DRAMSIM3),1)
PDB_LD_LIB       = -L $(DRAMSIM3_HOME)/ -ldramsim3 -Wl,-rpath-link=$(DRAMSIM3_HOME)/libdramsim3.so
endif

PDB_CXXFLAGS    += -I$(VCS_HOME)/include $(shell python3-config --includes) -I$(LIB_SWIG_DIR)/cpp
PDB_LD_LIB      += $(shell python3-config --ldflags)


SWIG_INCLUDE     = $(filter -I%, $(PDB_CXXFLAGS)) -I$(picker_include)
ifeq ($(WITH_CHISELDB), 1)
  SWIG_D        += -DENABLE_CHISEL_DB
endif

process_DiffTestState:
	rm -rf $(PDB_OBJ_DIR) $(PYTHON_DIR)
	mkdir -p $(PDB_OBJ_DIR)
	mkdir -p $(PYTHON_DIR)
	cp $(PDB_SWIG_DIR)/swig.i $(PDB_OBJ_DIR)/python.i
	echo "%extend DiffTestState {" >> $(PDB_OBJ_DIR)/python.i
	cat $(BUILD_DIR)/generated-src/diffstate.h|grep Difftest|grep "\["|sed "s/\[/\ /g"|sed "s/\]/\ /g"|awk '{print $$1 " *get_"$$2"(int index){if(index<"$$3"){return &(self->"$$2"[index]);} return NULL;}"}' >> $(PDB_OBJ_DIR)/python.i
	echo "}" >> $(PDB_OBJ_DIR)/python.i

swig_export: process_DiffTestState
	swig -c++ -outdir $(PYTHON_DIR) -o $(PDB_OBJ_DIR)/difftest_wrap.cpp $(SWIG_INCLUDE) -python $(SWIG_D) $(PDB_OBJ_DIR)/python.i
	$(eval PDB_CXXFILES = $(PDB_CXXFILES) $(PDB_OBJ_DIR)/difftest_wrap.cpp)

difftest_python: swig_export
	cd $(PDB_OBJ_DIR) 					&& \
	$(CC) $(PDB_CXXFLAGS) $(PDB_CXXFILES)			&& \
	$(CC) -o $(PYTHON_DIR)/_difftest.so -m64 -shared *.o $(PDB_LD_LIB) $(SIM_LDFLAGS)
	echo $(SIM_LDFLAGS) > $(PYTHON_DIR)/sim_ld_flags.txt
