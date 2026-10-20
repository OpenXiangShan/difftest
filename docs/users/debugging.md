# Debug a DiffTest Mismatch

Use this guide after a differential test reports a mismatch. Work from the cheapest evidence to the most expensive: console output, Query DB, then waveform.

## 1. Reproduce and Capture the First Failure

Run the failing workload and save the complete output:

```bash
./build/emu -i "$WORKLOAD" --diff "$REF_SO" 2>&1 | tee mismatch.log
```

For simv:

```bash
./build/simv +workload="$WORKLOAD" +diff="$REF_SO" 2>&1 | tee mismatch.log
```

Preserve the first failure. Later output can contain secondary consequences of the same divergence.

## 2. Read the Console Mismatch

A mismatch normally reports:

- The failing checker, such as `InstrCommit` or `CSRState`.
- The cycle or step number.
- DUT and reference values for the compared fields.

Locate the checker in [`src/test/csrc/difftest/checkers/`](../../src/test/csrc/difftest/checkers) and identify which fields the checker compares. Do not change the checker until you understand whether the DUT probe, transport path, or reference state is wrong.

## 3. Compare Query DBs

Query DB records bundle events in SQLite so you can inspect transport behavior after the run.

Build with Query DB support:

```bash
make clean
make emu DIFFTEST_QUERY=1 -j2
```

Run the workload normally. The runtime writes `build/difftest_query.db`. Convert it to readable hexadecimal values:

```bash
python3 scripts/query/convert_hex.py build/difftest_query.db
```

This produces `build/difftest_query_hex.db`.

To compare against a known-good run, first save its database, then attach both databases in SQLite:

```bash
cp build/difftest_query.db ref-microbench.db
python3 scripts/query/convert_hex.py ref-microbench.db

sqlite3 build/difftest_query_hex.db \
  "ATTACH 'ref-microbench-hex.db' AS ref;
   SELECT a.STEP, a.NFUSED AS dut, b.NFUSED AS ref
   FROM main.InstrCommit a
   JOIN ref.InstrCommit b ON a.STEP=b.STEP AND a.MY_INDEX=b.MY_INDEX
   WHERE a.NFUSED != b.NFUSED
   ORDER BY a.STEP LIMIT 20;"
```

Replace `InstrCommit` and `NFUSED` with the checker and field that diverged. The first divergent step is usually the most useful.

### When to Use Query DB

- The console mismatch suggests dropped, duplicated, or reordered events.
- You enabled squash, delta, batch, replay, or a different backend.
- The DUT result changes when the transport configuration changes.

## 4. Capture a Waveform

Build with waveform support:

```bash
make clean
make emu EMU_TRACE=fst -j2
```

Run a focused window around the suspected cycle:

```bash
./build/emu -i "$WORKLOAD" --diff "$REF_SO" \
  -b 1000 -e 1100 --dump-wave --wave-path=debug.fst
```

For VCD output, build with `EMU_TRACE=1`. Open the waveform in a viewer such as GTKWave and inspect:

- The DUT probe valid signals.
- The pipeline ready/valid handshakes.
- Squash, batch, delta, and replay control signals.
- Whether the expected event reached the DPI-C boundary.

Form a specific timing hypothesis before collecting the waveform. A full waveform without a target cycle is expensive to generate and inspect.

## 5. Escalate by Component

| Symptom | Inspect first |
|---------|---------------|
| Wrong instruction or register value | DUT probe wiring, commit ordering, `InstrCommit` checker |
| Wrong CSR or privilege transition | CSR bundles, rename-table update, reference synchronization |
| Load/store mismatch | Load/store event ordering, memory model, checker |
| Only fails with squash/delta/batch | Query DB at each transport stage |
| Only fails on FPGA or emulation backend | Backend transport, clock/reset, handshake behavior |
| Missing reference progress | `refproxy.cpp`, reference-model ABI, and workload loading |

## Related Documentation

- [Software checking flow](../developers/software-check.md)
- [Hardware transport flow](../developers/hardware-flow.md)
- [Build, test, and debug](../developers/build-and-test.md)
