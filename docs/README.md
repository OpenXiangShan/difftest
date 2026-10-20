# DiffTest Documentation

The documentation is organized by the reader's goal. Start with the section that matches what you need to do.

## Run an Existing Emulator

Use these guides when the CPU project already integrates DiffTest and produces a DiffTest-enabled `emu` or `simv` executable.

| Document | Contents |
|----------|----------|
| [running-emu.md](users/running-emu.md) | Run workloads, select a reference model, and interpret results |
| [runtime-options.md](users/runtime-options.md) | Runtime options for execution limits, logging, waveforms, snapshots, and memory |
| [debugging.md](users/debugging.md) | Reproduce a mismatch, inspect Query DB data, and capture waveforms |
| [plugins.md](users/plugins.md) | LightSSS snapshots and `spike-dasm` disassembly |

## Integrate DiffTest with a CPU

Use these guides when connecting DiffTest to a new CPU or generating interfaces for a non-Chisel design.

| Document | Contents |
|----------|----------|
| [chisel.md](integration/chisel.md) | Current Chisel integration flow and wrapper API |
| [interfaces.md](integration/interfaces.md) | Probe bundles, top-level IOs, and memory interfaces |
| [non-chisel.md](integration/non-chisel.md) | Generate Verilog and C++ interfaces for a non-Chisel DUT |

## Develop DiffTest

Use these guides when changing DiffTest internals, transport behavior, checkers, reference-model integration, or build backends.

| Document | Contents |
|----------|----------|
| [repository-layout.md](developers/repository-layout.md) | Source tree, key files, and modification map |
| [hardware-flow.md](developers/hardware-flow.md) | Probe-to-DPI-C hardware transport pipeline |
| [software-check.md](developers/software-check.md) | DPI-C landing, reference model, and software checkers |
| [build-and-test.md](developers/build-and-test.md) | EMU, simv, FPGA Sim, Palladium, Query DB, and waveforms |
| [workflow.md](developers/workflow.md) | Development planning, progress tracking, and debugging escalation |
