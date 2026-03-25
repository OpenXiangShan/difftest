# DiffTest Layout

## Directory Structure

```
difftest/
├── Makefile, *.mk              # Build entry and backend Makefiles (emu.mk, vcs.mk, fpga.mk, etc.)
├── build.sc                    # Mill build definition (Scala/Chisel)
├── config/                     # C++ compile-time config macros (config.h, config.cpp)
│
├── src/
│   ├── main/scala/             # [Hardware side] Chisel sources (see Key Files below)
│   │   ├── Bundles.scala       #   Probe bundle definitions
│   │   ├── Difftest.scala      #   Bundle registration, API, and C++ struct generation
│   │   ├── Gateway.scala       #   Hardware transport pipeline & --difftest-config parsing
│   │   ├── Preprocess.scala    #   Physical-to-architectural register conversion
│   │   ├── Squash.scala        #   Cross-cycle event compression and ordering
│   │   ├── Batch.scala         #   Multi-cycle batch packaging
│   │   ├── Delta.scala         #   Delta (incremental) transport
│   │   ├── Validate.scala      #   Valid stream conversion
│   │   ├── DPIC.scala          #   DPI-C function generation (HW → SW bridge)
│   │   ├── Replay.scala        #   Replay support
│   │   ├── Trace.scala         #   IOTrace dump/load
│   │   ├── common/             #   Shared modules (RAM, Flash, SD, logging, perf)
│   │   ├── fpga/               #   FPGA-side interfaces
│   │   └── util/               #   Profile loading & utilities
│   │
│   └── test/
│       ├── scala/              # Chisel generation entry points
│       │   ├── DifftestMain.scala  # Entry used by `make`
│       │   └── DifftestTop.scala   # Standalone generation top
│       │
│       ├── csrc/               # [Software side] C++ runtime (see Key Files below)
│       │   ├── difftest/       #   Core checking logic
│       │   │   ├── difftest.cpp    # difftest_init/step main flow
│       │   │   ├── diffstate.cpp   # DUT state buffer
│       │   │   ├── refproxy.cpp    # Reference model proxy (dlopen/dlsym)
│       │   │   ├── goldenmem.cpp   # Golden memory
│       │   │   ├── difftrace.cpp   # DiffTrace & IOTrace
│       │   │   └── checkers/       # Checkers (instruction, memory, TLB, etc.)
│       │   ├── common/         #   Shared runtime support
│       │   │   ├── args.cpp        # Command-line argument parsing
│       │   │   ├── ram.cpp         # Memory management
│       │   │   ├── dut.cpp         # DUT abstraction
│       │   │   ├── coverage.cpp    # Coverage
│       │   │   └── query.cpp       # Query DB
│       │   ├── emu/            #   EMU backend entry (main.cpp, emu.cpp)
│       │   ├── verilator/      #   Verilator waveform & snapshot
│       │   ├── vcs/            #   VCS backend wrapper
│       │   ├── gsim/           #   GSIM backend wrapper
│       │   ├── fpga/           #   FPGA host side
│       │   ├── fpga_sim/       #   FPGA simulation
│       │   └── plugin/         #   Optional plugins (spikedasm, xspdb, etc.)
│       │
│       └── vsrc/               # Verification-side Verilog
│           ├── common/
│           ├── vcs/
│           └── fpga_sim/
│
├── scripts/                    # Script tools
│   ├── coverage/               #   Coverage post-processing
│   ├── fpga/, fpga_sim/        #   FPGA related
│   └── query/, st_tools/       #   Other tools
│
└── build/                      # [Generated artifacts — DO NOT edit manually]
    ├── rtl/                    #   Generated RTL
    ├── generated-src/          #   Generated C++ headers and source files
    ├── *-compile/              #   Backend compilation directories
    └── emu, simv               #   Generated executables
```

## Key Files

### Hardware Side (Chisel)

| File | Role |
|------|------|
| [`Bundles.scala`](../src/main/scala/Bundles.scala) | Defines all DiffTest probe bundles (`InstrCommit`, `CSRState`, `StoreEvent`, etc.) |
| [`Difftest.scala`](../src/main/scala/Difftest.scala) | `DifftestModule.apply()` registers bundles with Gateway; generates corresponding C/C++ structs |
| [`Gateway.scala`](../src/main/scala/Gateway.scala) | Hardware transport pipeline controller; `GatewayEndpoint` chains all processing stages; parses `--difftest-config` letters |
| [`DPIC.scala`](../src/main/scala/DPIC.scala) | Generates DPI-C functions (`v_difftest_*`), the HW-to-SW data bridge |
| [`Squash.scala`](../src/main/scala/Squash.scala) | Cross-cycle event compression, reduces transport volume |
| [`Batch.scala`](../src/main/scala/Batch.scala) | Multi-cycle batch packaging |
| [`Delta.scala`](../src/main/scala/Delta.scala) | Transmits only changed elements for further compression |

### Software Side (C++)

| File | Role |
|------|------|
| [`difftest.cpp`](../src/test/csrc/difftest/difftest.cpp) | `difftest_init()` initialization, `difftest_step()` drives each checking step |
| [`refproxy.cpp`](../src/test/csrc/difftest/refproxy.cpp) | Loads reference model `.so` via `dlopen`, exposes `ref_init/step/regcpy` interfaces |
| [`checkers/`](../src/test/csrc/difftest/checkers) | Checkers: instruction commit, memory access, TLB, traps, etc. |
| [`diffstate.cpp`](../src/test/csrc/difftest/diffstate.cpp) | Software-side landing point for DPI-C data, manages DUT state buffer |
| [`args.cpp`](../src/test/csrc/common/args.cpp) | Runtime command-line argument parsing |
| [`emu/main.cpp`](../src/test/csrc/emu/main.cpp) | EMU backend entry point |

### Build System

| File | Role |
|------|------|
| [`Makefile`](../Makefile) | Main entry, standalone interface generation |
| [`emu.mk`](../emu.mk) | EMU (Verilator) build flow |
| [`vcs.mk`](../vcs.mk) | VCS / VCS-top build flow |
| [`fpga.mk`](../fpga.mk) | FPGA host build and release |
| [`verilator.mk`](../verilator.mk) | Verilator compilation and coverage |

## Modification Guide

Before modifying code, identify the task type and follow the corresponding mapping:

| Task | Files to Modify | Notes |
|------|----------------|-------|
| Add/modify probe bundle | `Bundles.scala` → `Difftest.scala` | Keep C++ checkers in sync |
| Modify transport pipeline | `Gateway.scala` + corresponding stage file | Ensure Scala-generated definitions match C++ consumers |
| Modify checking logic | Corresponding file under `checkers/` | Do not modify DPI-C generated code |
| Modify reference model interface | `refproxy.cpp` / `refproxy.h` | Must be compatible with multiple REF implementations |
| Modify runtime arguments | `args.cpp` | May require syncing backend wrappers |
| Modify build flow | `Makefile` / corresponding `.mk` file | Validate against CI workflows |

**Prohibited:**

- **Do not manually edit any file under `build/`** — they are generated artifacts, overwritten on each build.
- **Do not modify test environment configurations** (CI workflows, container images, etc.) unless explicitly requested.
- **Do not modify `difftest-dpic.h/cpp`** — they are auto-generated by `DPIC.scala`.
- When changing interfaces, Scala-generated definitions and C++ runtime consumers must stay aligned.
