# DiffTest Hardware Flow

This document describes the hardware-side data flow from DUT probes to the C++ runtime.

## Overview

The hardware transport pipeline is assembled in `GatewayEndpoint` within [`Gateway.scala`](../src/main/scala/Gateway.scala). The data flow order is:

```
DUT Probes (Bundles.scala)
  │
  ▼
Delay ── pipeline alignment
  │
  ▼
Trace ── optional: IOTrace dump/load (T/L)
  │
  ▼
Preprocess ── physical → architectural register conversion
  │
  ▼
Replay ── optional: replay support (R)
  │
  ▼
Validate ── convert to Valid[...] streams
  │
  ▼
Squash ── optional: cross-cycle event compression (S)
  │
  ▼
Delta ── optional: incremental transport (D)
  │
  ▼
Batch ── optional: multi-cycle packaging (B)
  │
  ▼
Sink (DPIC.scala) ── DPI-C write to C++ runtime
```

Not all stages are always enabled. Which stages are present depends on `--difftest-config` letters and backend flags.

## Core Modules

### Preprocess

Implementation: [`Preprocess.scala`](../src/main/scala/Preprocess.scala)

- Converts physical register state + rename tables into architectural register state (when `softArchUpdate` is not enabled)
- Generates `commit_data` and optional `vec_commit_data`
- May remove some load events in single-core scenarios

Automatically enabled when any of these is true: dut zone, batch, squash, or replay is enabled, or softArchUpdate is not enabled.

### Squash

Implementation: [`Squash.scala`](../src/main/scala/Squash.scala), config letter: `S`

- Stamps load/store events so they can be matched against commit order
- Merges compressible events across cycles when bundle rules allow
- Forces flush on non-compressible conditions, timeouts, replay, or group ticks

**Purpose**: Reduces transport pressure but changes when events become visible to software. Critical for load, store, and commit ordering.

### Batch

Implementation: [`Batch.scala`](../src/main/scala/Batch.scala), config letter: `B`

- Clusters same-type bundles within one cycle
- Assembles data and metadata across cycles
- Emits a larger packet with step count and enable signal

**Purpose**: Base transport format for FPGA flow and delta transport. Related options:
- `I`: internal step
- `N`: non-block transport
- `F`: FPGA-mode batch settings

### Delta

Implementation: [`Delta.scala`](../src/main/scala/Delta.scala), config letter: `D` (requires Batch)

- Tracks element-level changes, transmits only changed elements plus `DiffDeltaInfo`
- Queues multi-part updates for in-order delivery

**Purpose**: Significantly reduces transport data volume, but requires an incremental reconstruction step on the software side.

### Validate

Implementation: [`Validate.scala`](../src/main/scala/Validate.scala)

- Converts preprocessed bundle stream into `Valid[...]` events
- Applies gateway-side control rules
- Common handoff point before squash, delta, or batch

### Sink (DPI-C)

Implementation: [`DPIC.scala`](../src/main/scala/DPIC.scala)

- Without batch: each valid bundle is written to C++ via individual DPI-C function calls
- With batch: a packed batch payload is assembled first, then written as a whole
- Auto-generates `v_difftest_*` C functions and `difftest-dpic.h/cpp` headers

## Auxiliary Modules

### Replay

Implementation: [`Replay.scala`](../src/main/scala/Replay.scala), config letter: `R` (requires Squash)

Adds replay trace information so the runtime can revisit or reconstruct previously buffered states.

### Trace (IOTrace)

Implementation: [`Trace.scala`](../src/main/scala/Trace.scala), config letters: `T` (dump) / `L` (load)

- Trace dump: records the delayed bundle stream before preprocess
- Trace load: replaces the normal delayed bundle path with loaded trace data

### Global Enable & Dut Zone

Configured in [`Gateway.scala`](../src/main/scala/Gateway.scala):

- `E`: adds a global DPI-C enable control path
- `Z`: enables dut-zone rotation (software side uses a larger diffstate buffer)

## --difftest-config

Passed via `MILL_ARGS="--difftest-config <letters>"`. Each letter enables a feature:

| Letter | Feature | Letter | Feature |
|--------|---------|--------|---------|
| `E` | global enable | `S` | squash |
| `B` | batch | `D` | delta |
| `I` | internal step | `N` | non-block |
| `R` | replay | `P` | built-in perf counters |
| `Z` | dut zone | `U` | soft arch update |
| `T` | trace dump | `L` | trace load |
| `H` | hierarchical wiring | `F` | FPGA mode |
| `G` | GSIM mode | | |

**Dependencies:**
- Replay requires Squash
- Delta requires Batch
- FPGA mode requires Batch
- Batch and Dut Zone cannot be enabled together
