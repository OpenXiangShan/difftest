# Compile packaged RTL without running the CPU generator.
NOOP_HOME := $(CURDIR)
export NOOP_HOME
NUM_CORES ?= 1
SYNTHESIS ?= 0

.PHONY: pldm-build pldm-lib pldm-run pldm-debug pldm-clean
pldm-build pldm-lib pldm-run pldm-debug pldm-clean:
	$(MAKE) -C difftest $@ NUM_CORES=$(NUM_CORES) SYNTHESIS=$(SYNTHESIS)
