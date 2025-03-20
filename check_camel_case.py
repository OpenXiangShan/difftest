#!/usr/bin/env python3

import sys
import os
import re

# Color constants
RED = "\033[0;31m"
GREEN = "\033[0;32m"
RESET = "\033[0m"

def print_usage(program_name):
    print(f"{RED}Usage: {program_name} <directory> [--fix]{RESET}")
    sys.exit(1)

def find_snake_case(file_path, fix):
    modified_lines = []
    with open(file_path, "r") as file:
        for line_num, line in enumerate(file, start=1):
            modified_line = line
            for match in re.finditer(r"\b[a-z0-9]+_[a-z0-9]+\b", line):
                snake_case = match.group()
                camel_case = convert_to_camel(snake_case)
                print(f"{RED}{file_path}:{GREEN}{line_num}:{GREEN}{match.start()+1}: {RED}{snake_case} is not in camelCase{RESET}")
                if fix:
                    modified_line = modified_line.replace(snake_case, camel_case)
                    print(f"{GREEN}Fixed: {RED}{snake_case} {GREEN}-> {camel_case}{RESET}")
            modified_lines.append(modified_line)
    return modified_lines

def convert_to_camel(snake_str):
    parts = snake_str.split("_")
    return parts[0] + "".join(part.capitalize() for part in parts[1:])

def main():
    if len(sys.argv) < 2:
        print_usage(sys.argv[0])

    directory = sys.argv[1]
    fix = "--fix" in sys.argv

    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".scala"):
                file_path = os.path.join(root, file)
                modified_content = find_snake_case(file_path, fix)
                if fix:
                    with open(file_path, "w") as file:
                        file.writelines(modified_content)

if __name__ == "__main__":
    main()
