Difftest Submodule
===================

Difftest (差分测试) co-sim framework

# Usage

1. Init this submodule in your design, add it to dependency list.
2. Add difftest to your design.
3. Generate verilog files for simulation.
4. Assign `SIM_TOP`, `DESIGN_DIR` for `difftest/Makefile`
5. `cd difftest` and `make emu`, then start simulating & debugging!

To use difftest in XiangShan project:
```sh
cd XiangShan
make init
# cd difftest
make emu
```

# API

Difftest functions:

* DifftestArchEvent (essential)
* DifftestInstrCommit (essential)
* DifftestTrapEvent (essential)
* DifftestCSRState (essential)
* DifftestArchIntRegState (essential)
* DifftestArchFpRegState
* DifftestSbufferEvent
* DifftestStoreEvent
* DifftestLoadEvent
* DifftestAtomicEvent
* DifftestPtwEvent

Simulation top:

* LogCtrlIO
* PerfInfoIO
* UARTIO

Simulation memory:

* RAMHelper (essential)

To use `difftest`, include all these modules / simtopIO in your design.

# Further reference

* [Difftest: detailed usage (Chinese)](./doc/usage.md)
* [Example: difftest in XiangShan project (Chinese) ](./doc/example-xiangshan.md)
* [Example: difftest in NutShell project (Chinese) ](./doc/example-nutshell.md)
* [SMP-Difftest 支持多处理器的差分测试方法](https://github.com/OpenXiangShan/XiangShan-doc/blob/main/slides/20210624-RVWC-SMP-Difftest%20%E6%94%AF%E6%8C%81%E5%A4%9A%E5%A4%84%E7%90%86%E5%99%A8%E7%9A%84%E5%B7%AE%E5%88%86%E6%B5%8B%E8%AF%95%E6%96%B9%E6%B3%95.pdf)