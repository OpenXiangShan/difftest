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
# Extract Critical Path Information from Timing Reports
# Compatible with iEDA/iSTA generated timing report format
###############################################################################

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

print_usage() {
    echo "Usage: $0 <timing_report_file>"
    echo ""
    echo "Parameters:"
    echo "  timing_report_file - iEDA generated timing report file (.rpt)"
    echo ""
    echo "Example:"
    echo "  $0 sta_results/GatewayEndpoint.rpt"
    echo ""
    exit 1
}

# Check parameters
if [ $# -lt 1 ]; then
    echo -e "${RED}Error: Missing timing report file parameter${NC}"
    print_usage
fi

RPT_FILE="$1"

# Check if file exists
if [ ! -f "$RPT_FILE" ]; then
    echo -e "${RED}Error: File not found: $RPT_FILE${NC}"
    exit 1
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   Timing Report Critical Path Analysis${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "Report file: ${CYAN}$RPT_FILE${NC}"
echo ""

# 1. Extract TNS (Total Negative Slack)
echo -e "${YELLOW}===========================================${NC}"
echo -e "${YELLOW}1. Timing Violation Summary (TNS)${NC}"
echo -e "${YELLOW}===========================================${NC}"

# Find TNS table
echo -e "${CYAN}--- TNS (Total Negative Slack) ---${NC}"
awk '
/^\|\s+Clock\s+\|\s+Delay Type\s+\|\s+TNS/ {
    in_tns = 1
    next
}
in_tns && /^\+/ {
    next
}
in_tns && /^\|/ {
    print $0
    if (++count >= 10) exit
}
in_tns && /^$/ {
    exit
}
' "$RPT_FILE"

echo ""

# 2. Extract Top Critical Paths (Setup - max)
echo -e "${YELLOW}===========================================${NC}"
echo -e "${YELLOW}2. Setup Critical Paths (Top 10)${NC}"
echo -e "${YELLOW}===========================================${NC}"

# Find Setup (max) paths and sort by Slack, take top 10
echo -e "${CYAN}--- Worst Setup Slack (Critical Paths) ---${NC}"
awk '
BEGIN { in_table = 0; count = 0 }
/^\| Endpoint.*\| Clock Group.*\| Delay Type.*\| Path Delay.*\| Path Required.*\| CPPR.*\| Slack.*\| Freq/ {
    in_table = 1
    print $0
    next
}
in_table && /^\+/ {
    next
}
in_table && /^\|/ {
    # Check if it is max type (Setup)
    for (i = 1; i <= NF; i++) {
        if ($i == "max") {
            print $0
            count++
            break
        }
    }
    if (count >= 10) exit
}
in_table && /^$/ {
    exit
}
' "$RPT_FILE" | head -15

echo ""

# 3. Extract Hold Critical Paths (min)
echo -e "${YELLOW}===========================================${NC}"
echo -e "${YELLOW}3. Hold Critical Paths (Top 10)${NC}"
echo -e "${YELLOW}===========================================${NC}"

echo -e "${CYAN}--- Worst Hold Slack (Critical Paths) ---${NC}"
awk '
BEGIN { in_table = 0; count = 0 }
/^\| Endpoint.*\| Clock Group.*\| Delay Type.*\| Path Delay.*\| Path Required.*\| CPPR.*\| Slack.*\| Freq/ {
    in_table = 1
    next
}
in_table && /^\+/ {
    next
}
in_table && /^\|/ {
    # Check if it is min type (Hold)
    for (i = 1; i <= NF; i++) {
        if ($i == "min") {
            print $0
            count++
            break
        }
    }
    if (count >= 10) exit
}
in_table && /^$/ {
    exit
}
' "$RPT_FILE" | head -15

echo ""

# 4. Extract worst path detailed timing
echo -e "${YELLOW}===========================================${NC}"
echo -e "${YELLOW}4. Worst Setup Path Detailed Timing${NC}"
echo -e "${YELLOW}===========================================${NC}"

# Extract detailed timing for first max path
awk '
BEGIN { in_path = 0; count = 0; max_lines = 50 }
/Point.*Fanout.*Capacitance.*Resistance.*Transition.*Delta Delay.*Incr.*Path/ {
    in_path = 1
    print $0
    next
}
in_path && /^\+/ {
    next
}
in_path && /^\|/ {
    print $0
    count++
    if (count >= max_lines) exit
}
in_path && /^$/ {
    if (count > 5) exit
}
' "$RPT_FILE" | head -60

echo ""

# 5. Statistics
echo -e "${YELLOW}===========================================${NC}"
echo -e "${YELLOW}5. Path Statistics${NC}"
echo -e "${YELLOW}===========================================${NC}"

# Count max type paths
MAX_COUNT=$(awk '
BEGIN { in_table = 0; count = 0 }
/^\| Endpoint.*\| Clock Group.*\| Delay Type.*\| Path Delay.*\| Path Required.*\| CPPR.*\| Slack.*\| Freq/ {
    in_table = 1
    next
}
in_table && /^\+/ {
    next
}
in_table && /^\|/ {
    for (i = 1; i <= NF; i++) {
        if ($i == "max") {
            count++
            break
        }
    }
}
in_table && /^$/ {
    exit
}
END { print count }
' "$RPT_FILE")

# Count min type paths
MIN_COUNT=$(awk '
BEGIN { in_table = 0; count = 0 }
/^\| Endpoint.*\| Clock Group.*\| Delay Type.*\| Path Delay.*\| Path Required.*\| CPPR.*\| Slack.*\| Freq/ {
    in_table = 1
    next
}
in_table && /^\+/ {
    next
}
in_table && /^\|/ {
    for (i = 1; i <= NF; i++) {
        if ($i == "min") {
            count++
            break
        }
    }
}
in_table && /^$/ {
    exit
}
END { print count }
' "$RPT_FILE")

echo -e "${CYAN}Setup (max) path count: ${MAX_COUNT}${NC}"
echo -e "${CYAN}Hold (min) path count: ${MIN_COUNT}${NC}"

# Check for negative Slack
VIOLATE_MAX=$(awk '
BEGIN { in_table = 0; count = 0 }
/^\| Endpoint.*\| Clock Group.*\| Delay Type.*\| Path Delay.*\| Path Required.*\| CPPR.*\| Slack.*\| Freq/ {
    in_table = 1
    next
}
in_table && /^\+/ {
    next
}
in_table && /^\|/ {
    for (i = 1; i <= NF; i++) {
        if ($i == "max") {
            # Find Slack column value (second to last column)
            slack = $(NF-1)
            # Remove possible pipe characters
            gsub(/\|/, "", slack)
            # Check if negative
            if (slack + 0 < 0) {
                count++
            }
            break
        }
    }
}
in_table && /^$/ {
    exit
}
END { print count }
' "$RPT_FILE")

VIOLATE_MIN=$(awk '
BEGIN { in_table = 0; count = 0 }
/^\| Endpoint.*\| Clock Group.*\| Delay Type.*\| Path Delay.*\| Path Required.*\| CPPR.*\| Slack.*\| Freq/ {
    in_table = 1
    next
}
in_table && /^\+/ {
    next
}
in_table && /^\|/ {
    for (i = 1; i <= NF; i++) {
        if ($i == "min") {
            slack = $(NF-1)
            gsub(/\|/, "", slack)
            if (slack + 0 < 0) {
                count++
            }
            break
        }
    }
}
in_table && /^$/ {
    exit
}
END { print count }
' "$RPT_FILE")

echo ""
if [ "$VIOLATE_MAX" -gt 0 ]; then
    echo -e "${RED}⚠️  Setup violation paths: ${VIOLATE_MAX}${NC}"
else
    echo -e "${GREEN}✓ Setup timing met (no violations)${NC}"
fi

if [ "$VIOLATE_MIN" -gt 0 ]; then
    echo -e "${RED}⚠️  Hold violation paths: ${VIOLATE_MIN}${NC}"
else
    echo -e "${GREEN}✓ Hold timing met (no violations)${NC}"
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   Analysis Complete${NC}"
echo -e "${GREEN}========================================${NC}"
