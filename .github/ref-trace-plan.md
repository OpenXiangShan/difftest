# Readable Reference Trace Plan

## Scope and target

- Repositories: `rocket-chip/difftest`, `riscv-isa-sim/difftest`, and `NEMU`.
- Target workload: `/home/xuyinan/rocket/rocket-chip/_10018_`.
- Target command: `build/emu -i _10018_ -C 100000 --dump-ref-trace`, with an optional `--diff <reference.so>` override.
- In scope: reference shared-library builds, trace-control API plumbing, instruction/side-effect formatting, command-line integration, and focused documentation/tests.
- Out of scope: RTL behavior, architectural comparison policy, and unrelated simulator logging.

## Design rationale

`--dump-ref-trace` is a debug view of what the selected reference model actually
executed. The reference model therefore owns instruction and architectural
side-effect rendering; the emu host only enables it through an optional difftest
API. Spike should reuse its native commit-log path so instruction decoding,
privilege state, register writes, memory accesses, and exceptional events stay
consistent with Spike. NEMU should expose the same stable column layout and
equivalent architectural information using its native decoder and execution
state. References that do not implement the optional API must continue to run.

The common visual contract is one primary line per retired instruction:

```text
hart: mode pc (encoding) disassembly                         [ side effects ]
```

The trace must remain useful when redirected to a file: no color-only meaning,
fixed-width major columns, full 64-bit addresses/values, and explicit register,
memory, trap, and privilege information where the model provides it.

## Prerequisites and common commands

- Spike build: `make -C /home/xuyinan/rocket/riscv-isa-sim/difftest CPU=ROCKET -j$(nproc)`
- NEMU config/build: `make -C /home/xuyinan/rocket/NEMU riscv64-xs-ref_defconfig && make -C /home/xuyinan/rocket/NEMU -j$(nproc)`
- Emu build: `make -C /home/xuyinan/rocket/rocket-chip emu REF=Spike -j$(nproc)` (repeat with `REF=NEMU`)
- Runtime override: `/home/xuyinan/rocket/rocket-chip/build/emu -i /home/xuyinan/rocket/rocket-chip/_10018_ --diff <reference.so> -C 100000 --dump-ref-trace`
- Debug loop: inspect the first/last trace lines and any mismatch, then reduce the cycle bound around the first unexpected instruction.

## Phase 1: discover and define the trace contract

- Inspect Spike's commit-log implementation and current difftest execution path.
- Inspect NEMU's decoder/itrace and difftest execution path.
- Inspect emu argument parsing and dynamic-symbol loading.
- Preserve existing behavior when tracing is disabled.

Pass: the call sites and available architectural side-effect data are identified,
and the interface can be optional without breaking older reference libraries.

## Phase 2: reference-model implementation

- Add a trace-control export to Spike and route enabled difftest execution through
  Spike's native commit-log formatter.
- Add the same export to NEMU and render an aligned, Spike-like record from NEMU's
  decoder plus post-execution architectural state.
- Keep both shared-library build flows self-contained and document output paths.

Pass: both shared objects build and export the trace-control symbol; tracing is
silent by default and emits one readable record per executed instruction when enabled.

## Phase 3: emu integration

- Resolve and invoke the optional reference trace-control symbol after `dlopen`.
- Ensure `--diff <path>` takes precedence over the build-time default reference.
- Keep `--dump-ref-trace` accepted for both build-time reference choices.

Pass: emu starts with the default and explicitly supplied Spike/NEMU libraries,
and reports actionable errors for an invalid `--diff` path.

## Phase 4: end-to-end verification

- Run `_10018_` for 100000 cycles with the Spike library and save its output.
- Run the same workload and limit with the NEMU library and save its output.
- Check column alignment, disassembly, privilege mode, register writes, memory
  effects, and absence of trace output when the flag is omitted.

Pass: both traces are readable and structurally aligned, no difftest regression is
introduced before the cycle limit, and explicit `--diff` selection is visible in
the startup log or otherwise proven by model-specific trace output.

## Final verification

Rebuild both libraries and both emu configurations from their supported entry
points, then repeat the two 100000-cycle runs. Exit only when every requested
build and runtime path is proven by current command output and representative
trace excerpts.
