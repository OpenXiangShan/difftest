# Generate DiffTest Interfaces for Non-Chisel Designs

DiffTest is designed around Chisel bundles, but it can generate Verilog probe interfaces and matching C++ bridge code for a non-Chisel DUT.

## Generate Interfaces

From the DiffTest repository root:

```bash
make clean
make
```

The default target runs `DifftestMain` and writes generated RTL and C++ support files under `build/`, primarily:

- `build/rtl/`
- `build/generated-src/`

## Select Probes with a Profile

The example interface set is defined in [`DifftestMain.scala`](../../src/test/scala/DifftestMain.scala) and [`DifftestTop.scala`](../../src/test/scala/DifftestTop.scala). Adjust it to match the events your design can produce.

A generated DiffTest profile records bundle names, parameters, delays, and core count. Generate from a profile with:

```bash
make MILL_ARGS="--profile /path/to/profile.json --num-cores 1"
```

Profiles are useful when the same interface schema must be regenerated for a Verilog design or shared across cores.

## Configure Transport

Set the DiffTest configuration with `CONFIG`:

```bash
make CONFIG=ESB
```

Or pass the Mill argument explicitly:

```bash
make MILL_ARGS="--difftest-config ESB"
```

Start with no optimized stages, then enable squash (`S`), batch (`B`), delta (`D`), replay (`R`), or FPGA mode (`F`) one at a time. See [Hardware transport flow](../developers/hardware-flow.md) for dependencies between stages.

## Connect the Generated Interface

After generation:

1. Connect the generated probe ports to the corresponding signals in the Verilog DUT.
2. Preserve the generated bundle width, ordering, and valid semantics.
3. Reuse the generated DPI-C headers and sources in the host simulator build.
4. Keep the generated interface and C++ runtime from the same DiffTest revision.
5. Validate with MicroBench before adding optional probes.

The non-Chisel flow does not remove the need for architectural semantics. Each bundle must still be driven at the correct commit, trap, memory, or architectural-state event.

## Limitations

- Chisel integrations can use the advanced transport pipeline and automatic wrapper more directly.
- Non-Chisel integrations must maintain the generated port connections when the profile changes.
- Generated files must not be edited manually. Change the probe definition or profile and regenerate them.

## Related Documentation

- [Integrate DiffTest with a Chisel CPU](chisel.md)
- [Probe bundle reference](interfaces.md)
- [Repository layout](../developers/repository-layout.md)
