# Yosys Synthesis and Static Timing Analysis Tool

A comprehensive toolchain for RTL synthesis and static timing analysis using Yosys and iEDA/iSTA.

## Overview

This tool provides an automated workflow for:
- RTL synthesis using Yosys
- Static timing analysis using iEDA/iSTA
- SystemVerilog preprocessing for Yosys compatibility
- Timing report extraction and analysis

## Directory Structure

```
yosys_synth_tool/
├── run_sta.sh              # Main script for synthesis and timing analysis
├── sv_preprocessor.py      # SystemVerilog preprocessor
├── extract_timing_report.sh # Timing report extraction script
├── README.md               # This file
└── sta_results/            # Generated results directory

yosys-sta/ (installed separately, location specified via YOSYS_STA_DIR)
├── scripts/
│   ├── yosys.tcl          # Yosys synthesis script
│   ├── sta.tcl            # iSTA timing analysis script
│   └── default.sdc        # Default SDC constraints
├── pdk/
│   ├── nangate45/         # Nangate 45nm PDK
│   └── icsprout55/        # ICSPROUT 55nm PDK
└── bin/
    └── iEDA               # iEDA binary
```

## Prerequisites

- Yosys (installed locally and available in PATH)
- Python 3.x
- Bash shell
- iEDA/iSTA (yosys-sta installation)
- Yosys-sta directory set via `YOSYS_STA_DIR` environment variable

## Quick Start

### 1. Set up yosys-sta Location

Set the `YOSYS_STA_DIR` environment variable to point to your yosys-sta installation:

```bash
# Add to your ~/.bashrc or ~/.zshrc for persistent configuration
export YOSYS_STA_DIR=/path/to/yosys-sta
```

Or specify it inline when running the script.

### 2. Basic Usage

```bash
./run_sta.sh <rtl_directory> [top_module_name]
```

**Example:**
```bash
./run_sta.sh /path/to/rtl/files MyTopModule
```

### 3. With Environment Variables

```bash
YOSYS_STA_DIR=/path/to/yosys-sta CLK_PORT_NAME=clock CLK_FREQ_MHZ=1000 ./run_sta.sh /path/to/rtl/files MyTopModule
```

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `YOSYS_STA_DIR` | Path to yosys-sta installation directory | `./yosys-sta` |
| `CLK_FREQ_MHZ` | Clock frequency in MHz | `500` |
| `CLK_PORT_NAME` | Clock port name | `clock` |
| `SDC_FILE` | Custom SDC constraints file | `$YOSYS_STA_DIR/scripts/default.sdc` |
| `PDK` | Process design kit | `nangate45` |

## Supported File Types

- **`.v`** - Verilog design files
- **`.vh`** - Verilog header files
- **`.sv`** - SystemVerilog design files
- **`.svh`** - SystemVerilog header files (automatically handled)

## Features

### 1. Automatic RTL File Collection

The script automatically discovers RTL files in the input directory and all subdirectories, excluding test directories:
- `test/`
- `sim/`
- `tb/`
- `verification/`
- `bench/`

### 2. SystemVerilog Preprocessing

The tool includes a preprocessor that converts Yosys-incompatible SystemVerilog syntax:

#### Supported Conversions

| Original Syntax | Converted Syntax |
|----------------|------------------|
| `string var_name;` | `// string var_name;` (commented out) |
| `automatic logic func();` | `logic func();` (keyword removed) |
| `wire [15:0][3:0] var = '{val1, val2, ...};` | `wire [15:0][3:0] var; assign var[0] = val1; ...` |

### 3. Header File Handling

`.svh` header files are automatically:
- Discovered in the RTL directory
- Copied to the preprocessing directory
- Added to Yosys include path

### 4. Duplicate File Detection

Files with identical names are automatically deduplicated (first occurrence is kept).

### 5. Comprehensive Timing Reports

The tool generates multiple output files:

#### Synthesis Outputs
- `*.netlist.v` - Synthesized netlist
- `yosys.log` - Yosys synthesis log
- `synth_stat.txt` - Synthesis statistics
- `synth_check.txt` - Design check results

#### Timing Analysis Outputs
- `*.rpt` - Main timing report
- `sta.log` - iSTA analysis log
- `*_setup.skew` - Setup skew report
- `*_hold.skew` - Hold skew report
- `*.cap` - Capacitance report
- `*.fanout` - Fanout report
- `*.trans` - Transition report

## Output Directory

Results are stored in a fixed directory:
```
sta_results/
├── GatewayEndpoint.netlist.v
├── GatewayEndpoint.rpt
├── yosys.log
├── sta.log
└── ...
```

## Timing Report Extraction

Extract key timing information from the generated report:

```bash
./extract_timing_report.sh sta_results/GatewayEndpoint.rpt
```

### Report Sections

1. **TNS Summary** - Total Negative Slack by clock
2. **Setup Critical Paths** - Top 10 worst setup paths
3. **Hold Critical Paths** - Top 10 worst hold paths
4. **Path Details** - Detailed timing breakdown
5. **Statistics** - Path counts and violation detection

## Examples

### Example 1: Basic Synthesis

```bash
./run_sta.sh /home/user/project/rtl TopModule
```

### Example 1: Basic Synthesis

```bash
# Set yosys-sta location
export YOSYS_STA_DIR=/opt/yosys-sta

# Run synthesis and timing analysis
./run_sta.sh /home/user/project/rtl TopModule
```

### Example 2: Custom Clock Frequency

```bash
YOSYS_STA_DIR=/opt/yosys-sta CLK_FREQ_MHZ=1000 ./run_sta.sh /home/user/project/rtl TopModule
```

### Example 3: Custom Clock Port

```bash
YOSYS_STA_DIR=/opt/yosys-sta CLK_PORT_NAME=clk ./run_sta.sh /home/user/project/rtl TopModule
```

### Example 4: Custom SDC File

```bash
YOSYS_STA_DIR=/opt/yosys-sta SDC_FILE=/path/to/custom.sdc ./run_sta.sh /home/user/project/rtl TopModule
```

### Example 5: Different PDK

```bash
YOSYS_STA_DIR=/opt/yosys-sta PDK=icsprout55 ./run_sta.sh /home/user/project/rtl TopModule
```

## Troubleshooting

### Common Issues

#### 1. "yosys-sta directory not found"

**Cause:** `YOSYS_STA_DIR` environment variable not set or incorrect

**Solution:** Set the `YOSYS_STA_DIR` environment variable:
```bash
export YOSYS_STA_DIR=/path/to/yosys-sta
```

#### 2. "Can't open include file"

**Cause:** `.svh` header files not found

**Solution:** Ensure header files are in the RTL directory or its subdirectories

#### 3. "syntax error, unexpected TOK_AUTOMATIC"

**Cause:** SystemVerilog `automatic` keyword not supported

**Solution:** The preprocessor automatically removes this keyword

#### 4. "get_ports clk was not found"

**Cause:** Clock port name mismatch

**Solution:** Set the correct clock port name:
```bash
CLK_PORT_NAME=your_clock_name ./run_sta.sh ...
```

#### 5. "Re-definition of module"

**Cause:** Multiple files with the same module definition

**Solution:** The script automatically deduplicates files by name

## Workflow

```
┌─────────────────────┐
│   Input RTL Files   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  SV Preprocessing   │  ← Convert incompatible syntax
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Yosys Synthesis    │  ← Generate netlist
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  iSTA Timing Analysis│  ← Static timing analysis
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Timing Reports     │  ← Extract critical paths
└─────────────────────┘
```

## License

This tool uses:
- Yosys (https://github.com/YosysHQ/yosys)
- iEDA/iSTA (https://github.com/OSCC-Project/iEDA)

Please refer to their respective licenses.

## Contributing

Contributions are welcome! Please ensure:
- All scripts use POSIX-compatible bash
- Python scripts follow PEP 8 style guidelines
- Comments and documentation are in English

## Contact

For issues or questions, please refer to the project documentation or create an issue in the project repository.
