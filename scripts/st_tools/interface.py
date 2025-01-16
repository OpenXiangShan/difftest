#/usr/bin/python3
# -*- coding: UTF-8 -*-

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


import re
import sys
import argparse
import os

def process_file(filename, use_core_mode):
    try:
        with open(filename, 'r') as file:
            content = file.read()

        content = '`include "gateway_interface.svh"\n\n' + content
        pattern_found = False  # Flag to track if a matching pattern is detected
        port_pattern = r'module\s+[^\(]*\(([^)]*)\)'
        org_port_content = re.search(port_pattern, content, flags=re.DOTALL).group(1)
        port_content = org_port_content
        # Replace IO with interface
        if use_core_mode:
            # Check if any "output gateway_*" line exists
            pattern = r'.*\s+gateway_\d+.*?\n'
            if re.search(pattern, port_content):
                pattern_found = True
                # trailing comma if pattern not found in last line
                trail_comma = re.search(pattern, port_content.splitlines()[-1]+'\n') is None
                new_line = "  core_if.out gateway_out" + (',' if trail_comma else '') + '\n'
                # Replace first existing output gateway line and with replacement pattern, keep trailing comma if any
                port_content = re.sub(pattern, new_line, port_content, count = 1)
                # Remove remaining lines
                port_content = re.sub(pattern, '', port_content)
                content = content.replace(org_port_content, port_content)
                # Modify usage
                content = re.sub(r'(?<!\.)gateway_(\d+)', r'gateway_out.gateway_\1', content)
                print("'output gateway_*' found and pattern replaced.")
            else:
                print("No 'output gateway_*' found. No changes made.")

        else:
            # Check if any "in_*" pattern exists
            pattern = r'.*in_\d+.*?\n'
            if re.search(pattern, port_content):
                pattern_found = True
                trail_comma = re.search(pattern, port_content.splitlines()[-1]+'\n') is None
                new_line = "  gateway_if.in gateway_in" + (',' if trail_comma else '') + '\n'
                # Replace first existing in_* line and with replacement pattern, keep trailing comma if any
                port_content = re.sub(pattern, new_line, port_content, count = 1)
                # Remove remaining lines
                port_content = re.sub(pattern, '', port_content)
                content = content.replace(org_port_content, port_content)
                # Modify usage
                content = re.sub(r'(?<!\.)in_(\d+)', r'gateway_in.gateway_\1', content)
                print("'input in_*' found and pattern replaced.")
            else:
                print("No 'in_*' found. No changes made.")

        # Only write the file if the pattern was found and replaced
        if pattern_found:
            with open(filename, 'w') as file:
                file.write(content)
            mode = "core_out" if use_core_mode else "gateway_in"
            print("File {} has been successfully processed ({} mode).".format(filename, mode))
        else:
            print("File {} was not modified.".format(filename))

    except Exception as e:
        print("Error processing file {}: {}".format(filename, e), file=sys.stderr)

def remove_file(file_path):
    if os.path.exists(file_path):
        os.remove(file_path)

def rmprocess_file(file_path, string_to_delete=None):
    if os.path.exists(file_path):
        if string_to_delete is not None:
            with open(file_path, 'r') as file:
                lines = file.readlines()
            new_lines = [line for line in lines if string_to_delete not in line]
            if len(new_lines) < len(lines):
                with open(file_path, 'w') as file:
                    file.writelines(new_lines)

                lines_deleted = len(lines) - len(new_lines)
                print(f" {file_path}  {lines_deleted}  '{string_to_delete}' ")
            else:
                print(f" {file_path}  '{string_to_delete}' ")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Process Verilog files with different modes.")
    parser.add_argument("filename", help="The Verilog file to process")
    parser.add_argument("--filelist", type=str, help="The filelist file to process")
    parser.add_argument("--simtop", type=str, help="The Verilog file to process")
    parser.add_argument("--core", action="store_true", help="Use core_out mode (default is gateway_in mode)")

    args = parser.parse_args()

    process_file(args.filename, args.core)

    if args.simtop:
        remove_file(args.simtop)
    if args.filelist:
        rmprocess_file(args.filelist, "SimTop.sv")
