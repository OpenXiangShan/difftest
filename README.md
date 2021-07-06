Difftest Submodule
===================

Difftest (差分测试) co-sim framework

# Usage

1. Init this submodule in your design, add it to dependency list.
2. Put difftest blackbox modules into your design.
3. Generate verilog files for simulation.
4. Assign `SIM_TOP`, `DESIGN_DIR` for `difftest/Makefile`
5. `cd difftest` and `make emu`, then start simulating & debugging!

TLDR:
```sh
cd XiangShan
make init
cd difftest
make emu
```

# API

* DifftestArchEvent
* DifftestInstrCommit
* DifftestTrapEvent
* DifftestCSRState
* DifftestArchIntRegState
* DifftestArchFpRegState
* DifftestSbufferEvent
* DifftestStoreEvent
* DifftestLoadEvent
* DifftestAtomicEvent
* DifftestPtwEvent

To use difftest, include all these modules in your design.