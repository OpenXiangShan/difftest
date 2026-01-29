#!/bin/bash
#***************************************************************************************
# Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
#
# DiffTest is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************

###############################################################################
# run_sta.sh - RTL Static Timing Analysis Script (using yosys-sta toolchain)
#
# Usage: ./run_sta.sh <RTL_directory_path> [top_module_name]
#
# Parameters:
#   RTL_directory_path - Directory containing RTL design files
#   top_module_name    - (Optional) Top module name of the design, auto-detected if not provided
#
# Environment Variables:
#   YOSYS_STA_DIR      - Path to yosys-sta installation directory (required if not in ./yosys-sta)
#   CLK_FREQ_MHZ       - Clock frequency (MHz), default 500
#   CLK_PORT_NAME      - Clock port name, default clock
#   SDC_FILE           - SDC constraints file path
#   PDK                - Process design kit, default nangate45
#
# Examples:
#   # Use yosys-sta from environment variable
#   export YOSYS_STA_DIR=/path/to/yosys-sta
#   ./run_sta.sh /path/to/rtl/files my_top_module
#
#   # Or specify inline
#   YOSYS_STA_DIR=/path/to/yosys-sta CLK_FREQ_MHZ=1000 ./run_sta.sh /path/to/rtl/files
###############################################################################

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print help information
print_help() {
    echo "Usage: $0 <RTL_directory_path> [top_module_name]"
    echo ""
    echo "Parameters:"
    echo "  RTL_directory_path - Directory containing RTL design files (required)"
    echo "                       Supported file types: .v, .vh, .sv, .svh"
    echo "  top_module_name    - Top module name of the design (optional)"
    echo ""
    echo "Environment Variables:"
    echo "  YOSYS_STA_DIR   - Path to yosys-sta installation (required if not in script dir)"
    echo "                    Default: ./yosys-sta"
    echo "  CLK_FREQ_MHZ    - Clock frequency (MHz), default 500"
    echo "  CLK_PORT_NAME   - Clock port name, default clock"
    echo "  SDC_FILE        - SDC constraints file path"
    echo "  PDK             - Process design kit, default nangate45"
    echo ""
    echo "Notes:"
    echo "  - .v, .vh, .sv files will be processed as design files"
    echo "  - .svh files will be automatically added to include path"
    echo "  - Test directories are automatically excluded (test, sim, tb, verification, bench)"
    echo ""
    echo "Examples:"
    echo "  # Use default yosys-sta location (./yosys-sta)"
    echo "  $0 /path/to/rtl/files"
    echo ""
    echo "  # Specify yosys-sta location via environment variable"
    echo "  export YOSYS_STA_DIR=/path/to/yosys-sta"
    echo "  $0 /path/to/rtl/files my_top_module"
    echo ""
    echo "  # Specify top module and clock settings"
    echo "  CLK_FREQ_MHZ=1000 CLK_PORT_NAME=core_clk $0 /path/to/rtl/files"
    exit 0
}

# Check parameters
if [ $# -lt 1 ]; then
    echo -e "${RED}Error: Missing RTL directory path parameter${NC}"
    print_help
fi

# Default excluded directories (testbench, simulation code, etc.)
EXCLUDE_DIRS="test|sim|tb|verification|bench"

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RTL_DIR="$1"
TOP_MODULE="${2:-}"

# Set yosys-sta directory from environment variable or use default
# User can set YOSYS_STA_DIR environment variable to point to yosys-sta installation
if [ -z "$YOSYS_STA_DIR" ]; then
    YOSYS_STA_DIR="${SCRIPT_DIR}/yosys-sta"
fi

# Set environment variables
export CLK_FREQ_MHZ="${CLK_FREQ_MHZ:-500}"
export CLK_PORT_NAME="${CLK_PORT_NAME:-clock}"

# Set PDK
PDK="${PDK:-nangate45}"

# Set result directory (fixed location, no timestamp)
RESULT_DIR="${SCRIPT_DIR}/sta_results"

# Set ABC threads
ABC_THREADS="${ABC_THREADS:-16}"
[ "$STA_VERBOSE" = "1" ] && echo "ABC threads: ${ABC_THREADS}"

# Set synthesized netlist path
NETLIST_SYN_V="${RESULT_DIR}/${TOP_MODULE}.netlist.v"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   RTL Timing Analysis (using yosys-sta)${NC}"
echo -e "${GREEN}========================================${NC}"
echo "yosys-sta directory: ${YOSYS_STA_DIR}"
echo "RTL directory:       ${RTL_DIR}"
echo "Top module:          ${TOP_MODULE}"
echo "Clock frequency:     ${CLK_FREQ_MHZ} MHz"
echo "Clock port:          ${CLK_PORT_NAME}"
echo "PDK:                 ${PDK}"
echo "Result directory:    ${RESULT_DIR}"
echo ""

# Check yosys-sta directory
if [ ! -d "${YOSYS_STA_DIR}" ]; then
    echo -e "${RED}Error: yosys-sta directory not found: ${YOSYS_STA_DIR}${NC}"
    echo -e "${YELLOW}Please set YOSYS_STA_DIR environment variable to point to yosys-sta installation${NC}"
    echo -e "${YELLOW}Example: export YOSYS_STA_DIR=/path/to/yosys-sta${NC}"
    exit 1
fi

# Check iEDA executable
if [ ! -f "${YOSYS_STA_DIR}/bin/iEDA" ]; then
    echo -e "${RED}Error: iEDA executable not found: ${YOSYS_STA_DIR}/bin/iEDA${NC}"
    exit 1
fi

# Check yosys
if ! command -v yosys &> /dev/null; then
    echo -e "${RED}Error: yosys command not found${NC}"
    echo "Please install yosys (version >= 0.48) and add to PATH"
    exit 1
fi

# Check PDK files
PDK_DIR="${YOSYS_STA_DIR}/pdk/${PDK}"
if [ ! -d "${PDK_DIR}" ]; then
    echo -e "${RED}Error: PDK directory not found: ${PDK_DIR}${NC}"
    exit 1
fi

# Create result directory (must be done before using RESULT_DIR for file output)
mkdir -p "${RESULT_DIR}"

# Collect RTL files
echo -e "${YELLOW}Collecting RTL files...${NC}"
echo -e "${YELLOW}Search path: ${RTL_DIR}${NC}"
RTL_FILES=""

if [ -d "${RTL_DIR}" ]; then
    # Find all Verilog/SystemVerilog design files (exclude svh headers and test directories)
    # Use find -prune option to properly exclude test directories
    RTL_FILES=$(find "${RTL_DIR}" \
        -type d \( -name test -o -name sim -o -name tb -o -name verification -o -name bench \) -prune -o \
        -type f \( -name "*.v" -o -name "*.vh" -o -name "*.sv" \) \
        ! -name "*.svh" -print 2>/dev/null | tr '\n' ' ')

    # Display file count
    FILE_COUNT=$(echo "$RTL_FILES" | wc -w)
    echo -e "${YELLOW}Found ${FILE_COUNT} RTL files${NC}"

    # Check and remove duplicate file names (keep first occurrence only)
    echo -e "${YELLOW}Checking for duplicate files...${NC}"
    SEEN_FILES=""
    UNIQUE_FILES=""
    DUPLICATE_COUNT=0

    for file in $RTL_FILES; do
        basename=$(basename "$file")
        if echo "$SEEN_FILES" | grep -qF "$basename"; then
            echo -e "${YELLOW}  Skipping duplicate file: $basename ($(basename $(dirname $file)))${NC}"
            DUPLICATE_COUNT=$((DUPLICATE_COUNT + 1))
        else
            UNIQUE_FILES="$UNIQUE_FILES $file"
            SEEN_FILES="$SEEN_FILES $basename"
        fi
    done

    RTL_FILES="$UNIQUE_FILES"

    if [ $DUPLICATE_COUNT -gt 0 ]; then
        echo -e "${YELLOW}Skipped ${DUPLICATE_COUNT} duplicate files${NC}"
    fi

    if [ -z "$RTL_FILES" ]; then
        echo -e "${RED}Error: No Verilog files found in RTL directory (.v, .vh, .sv)${NC}"
        echo -e "${YELLOW}Note: .svh files are header files and need to be used with .sv files${NC}"
        echo -e "${YELLOW}Excluded directories: $EXCLUDE_DIRS${NC}"
        exit 1
    fi

    # Save file list to result directory instead of printing
    echo -e "${GREEN}Found ${FILE_COUNT} RTL design files (list saved to ${RESULT_DIR}/rtl_files.txt)${NC}"
    find "${RTL_DIR}" \
        -type d \( -name test -o -name sim -o -name tb -o -name verification -o -name bench \) -prune -o \
        -type f \( -name "*.v" -o -name "*.vh" -o -name "*.sv" \) \
        ! -name "*.svh" -print 2>/dev/null > "${RESULT_DIR}/rtl_files.txt"
else
    # If file is passed directly
    RTL_FILES="${RTL_DIR}"
fi

# Infer top module name (if not provided)
if [ -z "$TOP_MODULE" ]; then
    echo -e "${YELLOW}Top module not specified, attempting auto-detection...${NC}"

    # Method 1: Search for module definition
    TOP_MODULE=$(grep -h "^module " ${RTL_DIR}/*.v 2>/dev/null | head -1 | sed 's/module //' | sed 's/ .*//' | sed 's/(.*//')

    if [ -z "$TOP_MODULE" ]; then
        echo -e "${YELLOW}Warning: Unable to auto-detect top module name${NC}"
        TOP_MODULE="top"
    fi
    echo -e "${GREEN}Using top module name: ${TOP_MODULE}${NC}"
fi

# Set SDC file
if [ -z "$SDC_FILE" ]; then
    SDC_FILE="${YOSYS_STA_DIR}/scripts/default.sdc"
fi

if [ ! -f "$SDC_FILE" ]; then
    echo -e "${RED}Error: SDC file not found: $SDC_FILE${NC}"
    exit 1
fi

echo -e "${YELLOW}Using SDC file: $SDC_FILE${NC}"

# Preprocess RTL files (convert incompatible SV syntax)
echo -e "${YELLOW}Preprocessing RTL files...${NC}"
PREPROCESSED_RTL_DIR="${RESULT_DIR}/rtl_preprocessed"
mkdir -p "${PREPROCESSED_RTL_DIR}"

PREPROCESSOR_SCRIPT="${SCRIPT_DIR}/sv_preprocessor.py"
PREPROCESSED_FILES=""

for rtl_file in $RTL_FILES; do
    basename=$(basename "$rtl_file")
    output_file="${PREPROCESSED_RTL_DIR}/${basename}"

    # Use preprocessor to convert file
    python3 "${PREPROCESSOR_SCRIPT}" "$rtl_file" "$output_file"

    if [ $? -eq 0 ]; then
        PREPROCESSED_FILES="$PREPROCESSED_FILES $output_file"
    else
        # If preprocessing fails, copy original file
        echo -e "${YELLOW}  Warning: Preprocessing failed, using original file: $basename${NC}"
        cp "$rtl_file" "$output_file"
        PREPROCESSED_FILES="$PREPROCESSED_FILES $output_file"
    fi
done

# Use preprocessed file list
RTL_FILES="$PREPROCESSED_FILES"

# Copy .svh header files to preprocessed directory (Yosys needs them)
echo -e "${YELLOW}Copying header files...${NC}"
SVH_FILES=$(find "${RTL_DIR}" \
    -type d \( -name test -o -name sim -o -name tb -o -name verification -o -name bench \) -prune -o \
    -type f -name "*.svh" -print 2>/dev/null)

for svh_file in $SVH_FILES; do
    cp "$svh_file" "${PREPROCESSED_RTL_DIR}/"
done

# Display copied header file count
SVH_COUNT=$(echo "$SVH_FILES" | wc -w)
if [ $SVH_COUNT -gt 0 ]; then
    echo -e "${YELLOW}Copied ${SVH_COUNT} header files${NC}"
fi

# Step 1: Run Yosys synthesis using yosys-sta's yosys.tcl
echo -e "${GREEN}========================================${NC}"
echo -e "${YELLOW}Step 1: Running Yosys synthesis...${NC}"
echo -e "${GREEN}========================================${NC}"

cd "${YOSYS_STA_DIR}"

# Set Yosys include path (points to preprocessed directory containing .svh headers)
# Use read_verilog options to avoid redefinition issues
YOSYS_INC_CMD="verilog_defaults -add -I${PREPROCESSED_RTL_DIR}"

echo "${YOSYS_INC_CMD}; tcl scripts/yosys.tcl ${TOP_MODULE} ${PDK} \"${RTL_FILES}\" ${NETLIST_SYN_V}" | \
    yosys -g -l "${RESULT_DIR}/yosys.log" -s -

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Yosys synthesis completed!${NC}"
    echo -e "${GREEN}Netlist file: ${NETLIST_SYN_V}${NC}"
else
    echo -e "${RED}Yosys synthesis failed!${NC}"
    echo -e "${YELLOW}Check log: ${RESULT_DIR}/yosys.log${NC}"
    exit 1
fi

# Step 2: Run iSTA timing analysis
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${YELLOW}Step 2: Running iSTA timing analysis...${NC}"
echo -e "${GREEN}========================================${NC}"

set -o pipefail

# Run iEDA without pipe to ensure proper exit detection
./bin/iEDA -script scripts/sta.tcl "$SDC_FILE" "$NETLIST_SYN_V" "$TOP_MODULE" "$PDK" > "${RESULT_DIR}/sta.log" 2>&1

STA_EXIT_CODE=$?
if [ $STA_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}Timing analysis completed!${NC}"
else
    echo -e "${YELLOW}Timing analysis completed with warnings (exit code: ${STA_EXIT_CODE})${NC}"
fi

# Debug: List files generated by STA
echo -e "${YELLOW}Files in ${RESULT_DIR} after STA:${NC}"
ls -la "${RESULT_DIR}" | grep -E '\.(rpt|log|txt)$' || echo "No report files found"

# Step 3: Generate summary report
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${YELLOW}Generating summary report...${NC}"
echo -e "${GREEN}========================================${NC}"

SUMMARY="${RESULT_DIR}/sta_summary.txt"
cat > "${SUMMARY}" << EOF
========================================
       Timing Analysis Summary Report
========================================

Analysis time: $(date)
RTL directory:  ${RTL_DIR}
Top module:     ${TOP_MODULE}
PDK:            ${PDK}
Clock frequency: ${CLK_FREQ_MHZ} MHz
Clock port:     ${CLK_PORT_NAME}

----------------------------------------
Synthesis Results:
----------------------------------------

EOF

# Add synthesis statistics
if [ -f "${RESULT_DIR}/synth_stat.txt" ]; then
    echo "" >> "${SUMMARY}"
    echo "Synthesis Statistics:" >> "${SUMMARY}"
    echo "----------------------------------------" >> "${SUMMARY}"
    cat "${RESULT_DIR}/synth_stat.txt" >> "${SUMMARY}"
fi

# Add timing information
if [ -f "${RESULT_DIR}/${TOP_MODULE}.rpt" ]; then
    echo "" >> "${SUMMARY}"
    echo "Timing Analysis Report:" >> "${SUMMARY}"
    echo "----------------------------------------" >> "${SUMMARY}"
    head -50 "${RESULT_DIR}/${TOP_MODULE}.rpt" >> "${SUMMARY}"
fi

# List generated files
echo "" >> "${SUMMARY}"
echo "----------------------------------------" >> "${SUMMARY}"
echo "Generated Files:" >> "${SUMMARY}"
echo "----------------------------------------" >> "${SUMMARY}"
ls -lh "${RESULT_DIR}" >> "${SUMMARY}"

echo -e "${GREEN}Timing analysis completed!${NC}"
echo ""
echo -e "${YELLOW}Results saved in: ${RESULT_DIR}${NC}"
echo ""
echo -e "${YELLOW}Main report files:${NC}"
echo "  - ${RESULT_DIR}/sta_summary.txt           - Summary report"
echo "  - ${RESULT_DIR}/${TOP_MODULE}.netlist.v   - Synthesized netlist"
echo "  - ${RESULT_DIR}/${TOP_MODULE}.rpt         - Timing analysis report"
echo "  - ${RESULT_DIR}/synth_stat.txt            - Synthesis statistics"
echo "  - ${RESULT_DIR}/yosys.log                 - Yosys log"
echo "  - ${RESULT_DIR}/sta.log                   - iSTA log"
echo ""

# Display summary
if [ -f "${SUMMARY}" ]; then
    echo -e "${YELLOW}===== Timing Analysis Summary =====${NC}"
    cat "${SUMMARY}"
fi

# Step 4: Extract critical path information
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${YELLOW}Extracting Critical Path Information...${NC}"
echo -e "${GREEN}========================================${NC}"

CRITICAL_PATH_REPORT="${RESULT_DIR}/critical_paths.txt"
EXTRACTOR_SCRIPT="${SCRIPT_DIR}/extract_timing_report.sh"
RPT_FILE="${RESULT_DIR}/${TOP_MODULE}.rpt"

# Check if required files exist
if [ ! -f "${EXTRACTOR_SCRIPT}" ]; then
    echo -e "${RED}Error: Extractor script not found: ${EXTRACTOR_SCRIPT}${NC}"
    exit 1
fi

if [ ! -f "${RPT_FILE}" ]; then
    echo -e "${RED}Error: Timing report not found: ${RPT_FILE}${NC}"
    echo -e "${RED}Available .rpt files in ${RESULT_DIR}:${NC}"
    ls -la "${RESULT_DIR}"/*.rpt 2>/dev/null || echo "  (No .rpt files found)"
    echo -e "${RED}All files in ${RESULT_DIR}:${NC}"
    ls -la "${RESULT_DIR}"
    exit 1
fi

# Extract critical path information
echo -e "${YELLOW}Running timing report extraction...${NC}"
bash "${EXTRACTOR_SCRIPT}" "${RPT_FILE}" > "${CRITICAL_PATH_REPORT}"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Critical path extraction completed!${NC}"
    echo -e "${GREEN}Report saved to: ${CRITICAL_PATH_REPORT}${NC}"
    echo ""
    echo -e "${YELLOW}===== Critical Path Analysis =====${NC}"
    cat "${CRITICAL_PATH_REPORT}"
else
    echo -e "${RED}Error: Critical path extraction failed${NC}"
    exit 1
fi

# Append area/utilization information to critical_paths.txt
if [ -f "${RESULT_DIR}/synth_stat.txt" ]; then
    echo "" >> "${CRITICAL_PATH_REPORT}"
    echo "========================================" >> "${CRITICAL_PATH_REPORT}"
    echo "Area/Utilization Statistics" >> "${CRITICAL_PATH_REPORT}"
    echo "========================================" >> "${CRITICAL_PATH_REPORT}"
    cat "${RESULT_DIR}/synth_stat.txt" >> "${CRITICAL_PATH_REPORT}"
    echo -e "${GREEN}Area statistics appended to critical_paths.txt${NC}"
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Analysis Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${YELLOW}All report files:${NC}"
echo "  - ${RESULT_DIR}/sta_summary.txt           - Summary report"
echo "  - ${RESULT_DIR}/critical_paths.txt        - Critical path analysis"
echo "  - ${RESULT_DIR}/${TOP_MODULE}.netlist.v   - Synthesized netlist"
echo "  - ${RESULT_DIR}/${TOP_MODULE}.rpt         - Timing analysis report"
echo "  - ${RESULT_DIR}/synth_stat.txt            - Synthesis statistics"
echo "  - ${RESULT_DIR}/yosys.log                 - Yosys log"
echo "  - ${RESULT_DIR}/sta.log                   - iSTA log"
echo ""
