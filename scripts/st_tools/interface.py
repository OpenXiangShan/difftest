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

        # Replace IO with interface
        if use_core_mode:
            # Check if any "output gateway_*" line exists
            if re.search(r' output\s+.*\s+gateway_\d+', content):
                pattern_found = True
                # Remove existing output gateway line and add the replacement pattern
                content = re.sub(r'  output\s+.*\s+gateway.*\n', '', content)
                new_line = "  core_if.out gateway_out"

                replace_pattern = r'gateway_out.gateway_\1'
                # Ensure "Core_sig.core_out gateway_out" is added only once
                if new_line not in content:
                    # Add new_line before the line containing ");"
                    content = re.sub(r'^(.*\n)(?=.*\);)', lambda m: m.group(1) + new_line + '\n', content, count=1, flags=re.MULTILINE)

                # Perform the replacement pattern
                content = re.sub(r'gateway_(\d+)', replace_pattern, content)
                print("'output gateway_*' found and pattern replaced.")
            else:
                print("No 'output gateway_*' found. No changes made.")

        else:
            # Check if any "in_*" pattern exists
            if re.search(r'\bin_\d+', content):
                pattern_found = True
                # Remove existing in_* line and add the replacement pattern
                new_line = "  gateway_if.in gateway_in"
                replace_pattern = r'(gateway_in.gateway_\1'
                # Ensure "Gateway_interface.diff_top gateway_in" is added only once
                content = re.sub(r'^(.*)(?=\n.*\);)', lambda m: re.sub(r'(.*\s)(//.*)', r'\1,\2', m.group(1)), content, count=1, flags=re.MULTILINE)
                if new_line not in content:
                    # Add new_line before the line containing ");"
                    content = re.sub(r'^(.*\n)(?=.*\);)', lambda m: m.group(1) + new_line + '\n', content, count=1, flags=re.MULTILINE)

                content = re.sub(r'\(in_(\d+)', replace_pattern, content)
                print("'in_*' found and pattern replaced.")
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
