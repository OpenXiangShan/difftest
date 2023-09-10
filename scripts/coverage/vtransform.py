#!/usr/bin/env python3

import os
import sys


def create_cover_macros(build_path, off_filters):
    all_files = map(lambda f: os.path.join(build_path, f), os.listdir(build_path))
    def is_vfile(f): return os.path.isfile(f) and f.endswith(".v")
    vfiles = [f for f in all_files if is_vfile(f)]
    for filename in vfiles:
        lines = []
        need_update, in_module = False, False
        fh = open(filename, "r")
        for line in fh:
            e = line.strip().split()
            if e and e[0] == "module":
                module_name = e[1].split("//")[0].replace("(", "")
                if True in map(lambda f: f(module_name), off_filters):
                    assert (not in_module)
                    lines.append("/*verilator coverage_off*/\n")
                    in_module = True
                    need_update = True
            elif in_module and e and e[0] == "endmodule":
                lines.append("/*verilator coverage_on*/\n")
                in_module = False
            lines.append(line)
        fh.close()
        if need_update:
            fh = open(filename, "w")
            fh.writelines(lines)
            fh.close()


def is_sram_array(name):
    return name.startswith("array_") and name.endswith("_ext")


def is_difftest(name):
    return name.startswith("Difftest")


def is_helper(name):
    helper_modules = [
        "FBHelper", "SDHelper", "FlashHelper"
    ]
    return name in helper_modules


if __name__ == "__main__":
    build_path = sys.argv[1]
    off_filters = [is_sram_array, is_difftest, is_helper]
    create_cover_macros(build_path, off_filters)
