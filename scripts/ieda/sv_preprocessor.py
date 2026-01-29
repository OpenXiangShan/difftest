#!/usr/bin/env python3
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
"""
SystemVerilog Preprocessor - Convert Yosys-incompatible syntax

Main processing:
1. string type declarations - Comment out
2. automatic keyword - Remove
3. Assignment pattern syntax '{...} - Convert to compatible format
"""

import re
import sys
import os

def preprocess_automatic_keyword(content):
    """
    Remove automatic keyword (not supported by Yosys)

    Original: automatic logic func_name();
    Converted: logic func_name();

    Original: function automatic void func_name();
    Converted: function void func_name();
    """
    # Remove automatic keyword (followed by whitespace)
    # Match automatic followed by space or tab
    content = re.sub(r'\bautomatic\b\s+', '', content)

    return content

def preprocess_string_type(content):
    """
    Comment out string type declarations (not supported by Yosys)

    Original: string var_name;
    Converted: // string var_name;  // Commented out for Yosys compatibility
    """
    # Match string type declarations
    pattern = r'^(\s*)string\s+(\w+)\s*(?:\[[^\]]+\])?\s*(?:=\s*[^;]+)?;'

    def replace_func(match):
        indent = match.group(1)
        rest = match.group(0)
        return f"{indent}// {rest}  // Commented out for Yosys compatibility\n"

    return re.sub(pattern, replace_func, content, flags=re.MULTILINE)

def parse_assignment_pattern_values(values_str):
    """
    Intelligently parse assignment pattern value list, correctly handling nested expressions
    Example: "4'h0, 4'h0, in_0_valid ? 9'h101 : 9'h0" should return 3 values
    """
    values = []
    current = []
    depth = 0  # Bracket nesting depth
    i = 0

    while i < len(values_str):
        char = values_str[i]

        if char == '{' or char == '(':
            depth += 1
            current.append(char)
        elif char == '}' or char == ')':
            depth -= 1
            current.append(char)
        elif char == ',' and depth == 0:
            # Only split on comma at top level
            values.append(''.join(current).strip())
            current = []
        else:
            current.append(char)

        i += 1

    # Append the last value
    if current:
        values.append(''.join(current).strip())

    return values

def preprocess_assignment_pattern(content):
    """
    Convert assignment pattern syntax '{...} to compatible format

    Original: wire [15:0][3:0] _GEN = '{4'h0, 4'h0, ...};
    Converted: wire [15:0][3:0] _GEN;
               assign _GEN[0] = 4'h0;
               assign _GEN[1] = 4'h0;
               ...
    """
    # Match assignment pattern: type [msb:lsb] name = '{values};
    # Note: Match is for apostrophe '{ not regular brace {
    pattern = r'(\w+)\s*((?:\[[^\]]+\](?:\s*\[[^\]]+\])?)*)\s*(\w+)\s*=\s*\'\{\s*\n(.*?)\s*\}\s*;'

    def replace_func(match):
        data_type = match.group(1)
        dimensions = match.group(2)
        name = match.group(3)
        values_str = match.group(4)

        # Intelligently parse value list
        values = parse_assignment_pattern_values(values_str)

        # Generate assignment statements
        result = f"{data_type} {dimensions} {name};\n"
        for i, val in enumerate(values):
            if dimensions and val:  # Use index only when dimensions exist and value is present
                result += f"  assign {name}[{i}] = {val};\n"
            elif val:
                result += f"  assign {name} = {val};\n"

        return result

    return re.sub(pattern, replace_func, content, flags=re.DOTALL)

def preprocess_file(input_file, output_file):
    """Preprocess a single file"""
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Remove automatic keyword
        content = preprocess_automatic_keyword(content)

        # Comment out string type declarations
        content = preprocess_string_type(content)

        # Convert assignment patterns
        content = preprocess_assignment_pattern(content)

        # Write output file
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(content)

        return True, None
    except Exception as e:
        return False, str(e)

def main():
    if len(sys.argv) < 3:
        print("Usage: sv_preprocessor.py <input_file> <output_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    # Create output directory
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    success, error = preprocess_file(input_file, output_file)

    if success:
        print(f"[OK] {input_file} -> {output_file}")
        sys.exit(0)
    else:
        print(f"[ERROR] {input_file}: {error}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
