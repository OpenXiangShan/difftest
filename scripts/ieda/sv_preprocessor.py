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
4. procedural logic declaration initialization - Split declaration and assignment
5. packed array concat temporaries - Flatten 2-D logic and rewrite reads
"""

import re
import sys
import os

PROCEDURAL_START_RE = re.compile(r'^\s*(always(?:_\w+)?|initial|final)\b')
LOGIC_INIT_DECL_RE = re.compile(
    r'^(\s*)logic\s+'
    r'((?:signed\s+)?(?:\[[^\]]+\]\s*)*)'
    r'([A-Za-z_][A-Za-z0-9_$]*)'
    r'(\s*(?:\[[^\]]+\]\s*)*)'
    r'\s*=\s*(.*?);\s*(//.*)?$'
)
PACKED_ARRAY_DECL_RE = re.compile(
    r'^(\s*)logic\s+'
    r'(signed\s+)?'
    r'\[(\d+)\s*:\s*(\d+)\]\s*'
    r'\[(\d+)\s*:\s*(\d+)\]\s*'
    r'([A-Za-z_][A-Za-z0-9_$]*)\s*;\s*(//.*)?$'
)
CONCAT_ASSIGN_RE = re.compile(
    r'^(\s*)([A-Za-z_][A-Za-z0-9_$]*)\s*=\s*\{(.*)\}\s*;\s*$'
)

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

def preprocess_procedural_logic_initialization(content):
    """
    Split initialized logic declarations inside procedural blocks.

    Yosys accepts local declarations in procedural blocks, but rejects
    declaration-time initialization there.

    Original:
      always_comb begin
        logic tmp = in_valid & in_ready;
      end

    Converted:
      always_comb begin
        logic tmp;
        tmp = in_valid & in_ready;
      end
    """
    lines = content.splitlines()
    result = []
    procedural_depth = 0

    def count_keyword(line, keyword):
        code = line.split('//', 1)[0]
        return len(re.findall(rf'\b{keyword}\b', code))

    for line in lines:
        in_procedural = procedural_depth > 0
        match = LOGIC_INIT_DECL_RE.match(line) if in_procedural else None

        if match:
            indent, packed_dims, name, unpacked_dims, initializer, comment = match.groups()
            packed_dims = ' '.join(packed_dims.split())
            unpacked_dims = ''.join(unpacked_dims.split())
            dims = f' {packed_dims}' if packed_dims else ''
            comment = f' {comment}' if comment else ''
            result.append(f"{indent}logic{dims} {name}{unpacked_dims};")
            result.append(f"{indent}{name} = {initializer};{comment}")
        else:
            result.append(line)

        begins = count_keyword(line, 'begin')
        ends = count_keyword(line, 'end')
        if PROCEDURAL_START_RE.match(line):
            procedural_depth += max(begins - ends, 1)
        elif procedural_depth > 0:
            procedural_depth += begins - ends
            if procedural_depth < 0:
                procedural_depth = 0

    return '\n'.join(result) + ('\n' if content.endswith('\n') else '')

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

def preprocess_packed_arrays(content):
    """
    Flatten 2-D packed logic concat temporaries and rewrite indexed reads.

    This is different from preprocess_assignment_pattern(), which keeps the
    packed array shape and emits "assign name[i] = value". Here the Yosys
    failure is caused by a procedural local 2-D packed logic, so the array is
    flattened to 1-D bit ranges first. Reads from name[idx] are then rewritten
    to a mux selecting those ranges.

    Original:
      logic [3:0][31:0] _GEN;
      _GEN = {{a3}, {a2}, {a1}, {a0}};
      data = _GEN[idx];
      const_data = _GEN[1];

    Converted:
      logic [127:0] _GEN;
      _GEN[127:96] = a3;
      _GEN[95:64] = a2;
      _GEN[63:32] = a1;
      _GEN[31:0] = a0;
      data = ((idx) == 2'd0) ? _GEN[31:0] : ...;
      const_data = _GEN[63:32];
    """
    lines = content.splitlines()
    arrays = {}

    for line in lines:
        code = line.split('//', 1)[0]
        match = PACKED_ARRAY_DECL_RE.match(code)
        if not match:
            continue

        signed = match.group(2) or ''
        outer_left = int(match.group(3))
        outer_right = int(match.group(4))
        inner_left = int(match.group(5))
        inner_right = int(match.group(6))
        name = match.group(7)
        outer_count = abs(outer_left - outer_right) + 1
        inner_width = abs(inner_left - inner_right) + 1
        total_width = outer_count * inner_width
        outer_step = 1 if outer_right >= outer_left else -1
        outer_indices = list(range(outer_left, outer_right + outer_step, outer_step))
        bit_ranges = [
            ((outer_count - pos) * inner_width - 1, (outer_count - pos - 1) * inner_width)
            for pos in range(outer_count)
        ]
        arrays[name] = {
            'signed': signed,
            'total_width': total_width,
            'assign_ranges': bit_ranges,
            'read_ranges': dict(zip(outer_indices, bit_ranges)),
        }

    for line in lines:
        code = line.split('//', 1)[0]
        match = CONCAT_ASSIGN_RE.match(code)
        if not match:
            continue

        name = match.group(2)
        if name not in arrays:
            continue

        values = [strip_single_concat(v) for v in parse_assignment_pattern_values(match.group(3))]
        if len(values) == len(arrays[name]['assign_ranges']):
            arrays[name]['values'] = values

    arrays = {
        name: info for name, info in arrays.items()
        if 'values' in info and not has_packed_array_write(lines, name)
    }

    if not arrays:
        return content

    result = []
    for line in lines:
        decl = PACKED_ARRAY_DECL_RE.match(line)
        if decl and decl.group(7) in arrays:
            indent, name = decl.group(1), decl.group(7)
            signed, total_width = arrays[name]['signed'], arrays[name]['total_width']
            comment = f"\t{decl.group(8)}" if decl.group(8) else ''
            result.append(f"{indent}logic {signed}[{total_width - 1}:0] {name};{comment}")
            continue

        code, sep, comment = line.partition('//')
        assign = CONCAT_ASSIGN_RE.match(code)
        if assign and assign.group(2) in arrays:
            indent, name = assign.group(1), assign.group(2)
            suffix = f' {sep + comment}' if sep else ''
            for i, ((msb, lsb), value) in enumerate(zip(arrays[name]['assign_ranges'], arrays[name]['values'])):
                result.append(f"{indent}{name}[{msb}:{lsb}] = {value};{suffix if i == 0 else ''}")
            continue

        for name, info in arrays.items():
            code = replace_packed_array_reads(code, name, info['read_ranges'])
        result.append(code + (sep + comment if sep else ''))

    return '\n'.join(result) + ('\n' if content.endswith('\n') else '')

def strip_single_concat(value):
    value = value.strip()
    if value.startswith('{') and value.endswith('}'):
        inner = value[1:-1].strip()
        if len(parse_assignment_pattern_values(inner)) == 1:
            return inner
    return value

def has_packed_array_write(lines, name):
    for line in lines:
        code = line.split('//', 1)[0]
        if any(is_lhs_array_write(code, end) for _, end, _ in packed_array_refs(code, name)):
            return True
    return False

def replace_packed_array_reads(code, name, index_ranges):
    result, pos = [], 0
    for start, end, expr in packed_array_refs(code, name):
        result.append(code[pos:start])
        if has_top_level_colon(expr) or is_lhs_array_write(code, end):
            result.append(code[start:end + 1])
        elif expr.strip().isdigit() and int(expr) in index_ranges:
            msb, lsb = index_ranges[int(expr)]
            result.append(f'{name}[{msb}:{lsb}]')
        else:
            result.append(build_index_mux(name, expr, index_ranges))
        pos = end + 1
    result.append(code[pos:])
    return ''.join(result)

def packed_array_refs(code, name):
    pos, token = 0, f'{name}['
    while True:
        start = code.find(token, pos)
        if start < 0:
            return
        pos = start + len(token)
        if start > 0 and re.match(r'[A-Za-z0-9_$]', code[start - 1]):
            continue
        end = find_matching_bracket(code, pos)
        if end < 0:
            return
        yield start, end, code[pos:end]
        pos = end + 1

def find_matching_bracket(text, start):
    depth = 1
    pos = start
    while pos < len(text):
        if text[pos] == '[':
            depth += 1
        elif text[pos] == ']':
            depth -= 1
            if depth == 0:
                return pos
        pos += 1
    return -1

def has_top_level_colon(expr):
    depth = 0
    for char in expr:
        if char in '[({':
            depth += 1
        elif char in '])}':
            depth -= 1
        elif char == ':' and depth == 0:
            return True
    return False

def is_lhs_array_write(code, expr_end):
    suffix = code[expr_end + 1:].lstrip()
    return suffix.startswith('<=') or (suffix.startswith('=') and not suffix.startswith('=='))

def build_index_mux(name, expr, index_ranges):
    expr = expr.strip()
    indices = sorted(index_ranges)
    index_width = max(1, max(indices).bit_length())
    terms = []
    for index in indices[:-1]:
        msb, lsb = index_ranges[index]
        terms.append(f"(({expr}) == {index_width}'d{index}) ? {name}[{msb}:{lsb}] : ")
    last_msb, last_lsb = index_ranges[indices[-1]]
    terms.append(f'{name}[{last_msb}:{last_lsb}]')
    return '(' + ''.join(terms) + ')'

def preprocess_file(input_file, output_file):
    """Preprocess a single file"""
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Remove automatic keyword
        content = preprocess_automatic_keyword(content)

        # Split initialized procedural logic declarations
        content = preprocess_procedural_logic_initialization(content)

        # Comment out string type declarations
        content = preprocess_string_type(content)

        # Convert assignment patterns
        content = preprocess_assignment_pattern(content)

        # Flatten packed arrays and rewrite accesses
        content = preprocess_packed_arrays(content)

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
