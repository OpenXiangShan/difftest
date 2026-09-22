# Readable Reference Trace Progress

| Phase | Change summary | Build | `_10018_` runtime |
|-------|----------------|-------|--------------------|
| 1 | Trace/build/integration discovery | Complete | Passed |
| 2 | Spike and NEMU trace implementations | Complete | Passed |
| 3 | emu integration and `--diff` override | Complete | Passed |
| 4 | End-to-end comparison | Complete | Passed |

## Phase 1

### Changes made

- Added the task plan and this progress log.
- Confirmed `1.png` shows Spike's native commit-log layout: hart/mode, PC,
  instruction bits, disassembly, and a right-hand architectural side-effect field.

### Test results

- Spike `make -C riscv-isa-sim/difftest CPU=ROCKET -j16` and NEMU
  `make riscv64-rocket-ref_defconfig && make -j16` both passed.
- Rocket `make emu REF=Spike -j16` and `make emu REF=NEMU -j16` both passed;
  the generated Verilator directories are reference-specific.

### Issues and debugging

- `/home/xuyinan/rocket` is a workspace containing separate Git repositories,
  so changes and verification must be tracked independently in each repository.

## Phase 2

### Changes made

- Spike exposes `difftest_set_ref_trace(bool)` and formats native commit logs
  as aligned hart/mode/PC/encoding/disassembly/effect records.
- NEMU exposes the same API and records decoded instructions, GPR/FPR writes,
  vector/CSR writes, memory direction/width/address/data, mode changes, and
  traps.
- NEMU presents standard RISC-V mnemonics rather than internal helper names and
  records architectural CSR values, including synthesized `mstatus.SD`.
- NEMU has a `riscv64-rocket-ref_defconfig` with Rocket's 664-byte state ABI.

### Test results

- Both shared libraries export `difftest_set_ref_trace` and pass dynamic
  load/init/toggle smoke tests.

### Issues and debugging

- None recorded.

## Phase 3

### Changes made

- `--diff <path>` overrides the build-time reference and prints the resolved
  path at startup. Legacy references still use the old debug switch.
- Trace is enabled before the first reference step when `-b 0` is used.

### Test results

- Explicit NEMU override ran to 100000 cycles and produced 59551 instruction
  trace lines.

### Issues and debugging

- None recorded.

## Phase 4

### Changes made

- Rocket scalar store events are normalized to aligned reference beats,
  including cross-8-byte stores.
- Spike CPU build variants are isolated by canonical CPU name, fixing the
  previous `CPU=ROCKET`/`CPU_ROCKET_CHIP` mismatch.

### Test results

- Spike and NEMU each ran `_10018_` for 100000 cycles with `--dump-ref-trace`:
  59551 instructions, no mismatch, readable aligned traces.
- A line-by-line audit found identical PC/instruction streams, GPR/FPR effects,
  and memory direction/width/address/data across all 59551 Spike and NEMU
  records; the format checker found zero malformed records.
- Runs without `--dump-ref-trace` emitted zero reference trace lines.
- Runtime `--diff /home/xuyinan/rocket/NEMU/build/riscv64-nemu-interpreter-so`
  selected NEMU and completed the same run.

### Issues and debugging

- NEMU initially inferred writebacks only from before/after value differences,
  which hid architectural writes when the new value equaled the synchronized
  old value (including the first `x5 = 1`). Destination-register metadata now
  preserves such idempotent writes.
- NEMU load/store logging originally re-read address and data pointers after
  execution. When a destination aliased the base register, this printed the
  writeback value as the address. Effective address and store data are now
  captured before the memory operation.
- The initial Rocket NEMU configuration exposed HPM counter-enable bits that
  the default Rocket core does not implement. The Rocket reference config now
  matches Rocket's basic-counter-only CSR behavior.
