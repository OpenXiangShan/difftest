#/usr/bin/python3
# -*- coding: UTF-8 -*-

#***************************************************************************************
# Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2020-2021 Peng Cheng Laboratory
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


import os
import pprint
import re
import sys

LINE_COVERED = "LINE_COVERED"
NOT_LINE_COVERED = "NOT_LINE_COVERED"
TOGGLE_COVERED = "TOGGLE_COVERED"
NOT_TOGGLE_COVERED = "NOT_TOGGLE_COVERED"
DONTCARE = "DONTCARE"

BEGIN = "BEGIN"
END = "END"
CHILDREN = "CHILDREN"
MODULE = "MODULE"
INSTANCE = "INSTANCE"
TYPE = "TYPE"
ROOT = "ROOT"
NODE = "NODE"
SELFCOVERAGE = "SELFCOVERAGE"
TREECOVERAGE = "TREECOVERAGE"
LINECOVERAGE = 0
TOGGLECOVERAGE = 1

def check_one_hot(l):
    cnt = 0
    for e in l:
        if e:
            cnt += 1
    return cnt <= 1

def get_lines(input_dir):
    lines = []
    for filename in os.listdir(input_dir):
        if filename.endswith('_annotated.v') or filename.endswith('_annotated.sv'):
            input_file = os.path.join(input_dir, filename)
            with open(input_file) as f:
                for line in f:
                    lines.append(line)
    return lines

def get_line_annotation(lines):
    line_annotations = []
    # pattern_1: 040192     if(array_0_MPORT_en & array_0_MPORT_mask) begin
    # pattern_2: 2218110        end else if (_T_30) begin // @[Conditional.scala 40:58]
    # pattern_2: 000417     end else begin
    line_covered_pattern_1 = re.compile(r'^\s*(\d+)\s+if')
    line_covered_pattern_2 = re.compile(r'^\s*(\d+)\s+end else')
    not_line_covered_pattern_1 = re.compile(r'^\s*(%0+)\s+if')
    not_line_covered_pattern_2 = re.compile(r'^\s*(%0+)\s+end else')

    toggle_covered_pattern_1 = re.compile(r'^\s*(\d+)\s+reg')
    toggle_covered_pattern_2 = re.compile(r'^\s*(\d+)\s+wire')
    toggle_covered_pattern_3 = re.compile(r'^\s*(\d+)\s+input')
    toggle_covered_pattern_4 = re.compile(r'^\s*(\d+)\s+output')

    not_toggle_covered_pattern_1 = re.compile(r'^\s*(%0+)\s+reg')
    not_toggle_covered_pattern_2 = re.compile(r'^\s*(%0+)\s+wire')
    not_toggle_covered_pattern_3 = re.compile(r'^\s*(%0+)\s+input')
    not_toggle_covered_pattern_4 = re.compile(r'^\s*(%0+)\s+output')

    line_cnt = 0

    for line in lines:
        line_covered_match = line_covered_pattern_1.search(line) or line_covered_pattern_2.search(line)
        not_line_covered_match = not_line_covered_pattern_1.search(line) or not_line_covered_pattern_2.search(line)

        assert not (line_covered_match and not_line_covered_match)

        toggle_covered_match = toggle_covered_pattern_1.search(line) or toggle_covered_pattern_2.search(line) or \
                toggle_covered_pattern_3.search(line) or toggle_covered_pattern_4.search(line)
        not_toggle_covered_match = not_toggle_covered_pattern_1.search(line) or not_toggle_covered_pattern_2.search(line) or \
                not_toggle_covered_pattern_3.search(line) or not_toggle_covered_pattern_4.search(line)

        assert not (toggle_covered_match and not_toggle_covered_match)

        all_match = (line_covered_match, not_line_covered_match,
                toggle_covered_match, not_toggle_covered_match)
        if not check_one_hot(all_match):
            print("not_one_hot")
            print(line_cnt)
            print(all_match)
            assert False, "This line matches multiple patterns"
        if line_covered_match:
            line_annotations.append(LINE_COVERED)
        elif not_line_covered_match:
            line_annotations.append(NOT_LINE_COVERED)
        elif toggle_covered_match:
            line_annotations.append(TOGGLE_COVERED)
        elif not_toggle_covered_match:
            line_annotations.append(NOT_TOGGLE_COVERED)
        else:
            line_annotations.append(DONTCARE)
        line_cnt += 1
    return line_annotations

# get the line coverage statistics in line range [start, end)
def get_coverage_statistics(line_annotations, start, end):
    line_covered = 0
    not_line_covered = 0
    toggle_covered = 0
    not_toggle_covered = 0
    for i in range(start, end):
        if line_annotations[i] == LINE_COVERED:
            line_covered += 1

        if line_annotations[i] == NOT_LINE_COVERED:
            not_line_covered += 1

        if line_annotations[i] == TOGGLE_COVERED:
            toggle_covered += 1

        if line_annotations[i] == NOT_TOGGLE_COVERED:
            not_toggle_covered += 1

    # deal with divide by zero
    line_coverage = 1.0
    if line_covered + not_line_covered != 0:
        line_coverage = float(line_covered) / (line_covered + not_line_covered)

    toggle_coverage = 1.0
    if toggle_covered + not_toggle_covered != 0:
        toggle_coverage = float(toggle_covered) / (toggle_covered + not_toggle_covered)
    return ((line_covered, not_line_covered, line_coverage),
            (toggle_covered, not_toggle_covered, toggle_coverage))

# get modules and all it's submodules
def get_modules(lines):
    modules = {}

    module_pattern = re.compile(r"module (\w+)\s*(#|)\s*\(")
    endmodule_pattern = re.compile("endmodule")
    submodule_pattern = re.compile(r"(\w+) (\w+) \( // @\[\w+.scala \d+:\d+\]")

    line_count = 0

    name = "ModuleName"

    for line in lines:
        module_match = module_pattern.search(line)
        endmodule_match = endmodule_pattern.search(line)
        submodule_match = submodule_pattern.search(line)

        assert not (module_match and endmodule_match)

        if module_match:
            name = module_match.group(1)
            print("module_match: module", name, modules)
            assert name not in modules
            # [begin
            modules[name] = {}
            modules[name][BEGIN] = line_count
            # the first time we see a module, we treat as a root node
            modules[name][TYPE] = ROOT

        if endmodule_match:
            print("endmodule_match: module:", name, modules)
            assert name in modules
            assert END not in modules[name]
            # end)
            modules[name][END] = line_count + 1
            # reset module name to invalid
            name = "ModuleName"

        if submodule_match:
            # submodule must be inside hierarchy
            assert name != "ModuleName"
            submodule_type = submodule_match.group(1)
            submodule_instance = submodule_match.group(2)
            # print("submodule_match: type: %s instance: %s" % (submodule_type, submodule_instance))

            # submodules should be defined first
            # if we can not find it's definition
            # we consider it a black block module
            if submodule_type not in modules:
                print("Module %s is a Blackbox" % submodule_type)
            else:
                # mark submodule as a tree node
                # it's no longer root any more
                modules[submodule_type][TYPE] = NODE

                if CHILDREN not in modules[name]:
                    modules[name][CHILDREN] = []
                submodule = {MODULE: submodule_type, INSTANCE: submodule_instance}
                modules[name][CHILDREN].append(submodule)

        line_count += 1
    return modules

# we define two coverage metrics:
# self coverage: coverage results of this module(excluding submodules)
# tree coverage: coverage results of this module(including submodules)
def get_tree_coverage(modules, coverage):
    def dfs(module):
        if TREECOVERAGE not in modules[module]:
            self_coverage = modules[module][SELFCOVERAGE]
            if CHILDREN not in modules[module]:
                modules[module][TREECOVERAGE] = self_coverage
            else:
                line_covered = self_coverage[LINECOVERAGE][0]
                not_line_covered = self_coverage[LINECOVERAGE][1]
                toggle_covered = self_coverage[TOGGLECOVERAGE][0]
                not_toggle_covered = self_coverage[TOGGLECOVERAGE][1]
                # the dfs part
                for child in modules[module][CHILDREN]:
                    child_coverage = dfs(child[MODULE])
                    line_covered += child_coverage[LINECOVERAGE][0]
                    not_line_covered += child_coverage[LINECOVERAGE][1]
                    toggle_covered += child_coverage[TOGGLECOVERAGE][0]
                    not_toggle_covered += child_coverage[TOGGLECOVERAGE][1]
                # deal with divide by zero
                line_coverage = 1.0
                if line_covered + not_line_covered != 0:
                    line_coverage = float(line_covered) / (line_covered + not_line_covered)
                toggle_coverage = 1.0
                if toggle_covered + not_toggle_covered != 0:
                    toggle_coverage = float(toggle_covered) / (toggle_covered + not_toggle_covered)
                modules[module][TREECOVERAGE] = ((line_covered, not_line_covered, line_coverage),
                        (toggle_covered, not_toggle_covered, toggle_coverage))
        return modules[module][TREECOVERAGE]

    for module in modules:
        modules[module][SELFCOVERAGE] = coverage[module]

    for module in modules:
        modules[module][TREECOVERAGE] = dfs(module)
    return modules

# arg1: tree coverage results
# arg2: coverage type
def sort_coverage(coverage, self_or_tree, coverage_type):
    l = [(module, coverage[module][self_or_tree][coverage_type])for module in coverage]
    l.sort(key=lambda x:x[1][2])
    return l

def print_tree_coverage(tree_coverage):
    def dfs(module, level):
        # print current node
        tree = tree_coverage[module][TREECOVERAGE]
        self = tree_coverage[module][SELFCOVERAGE]
        print("  " * level + "- " + module)
        print("  " * level + "  tree_line", end="")
        print("(%d, %d, %.2f)" % (tree[LINECOVERAGE][0], tree[LINECOVERAGE][1], tree[LINECOVERAGE][2] * 100.0))
        print("  " * level + "  self_line", end="")
        print("(%d, %d, %.2f)" % (self[LINECOVERAGE][0], self[LINECOVERAGE][1], self[LINECOVERAGE][2] * 100.0))

        print("  " * level + "  tree_toggle", end="")
        print("(%d, %d, %.2f)" % (tree[TOGGLECOVERAGE][0], tree[TOGGLECOVERAGE][1], tree[TOGGLECOVERAGE][2] * 100.0))
        print("  " * level + "  self_toggle", end="")
        print("(%d, %d, %.2f)" % (self[TOGGLECOVERAGE][0], self[TOGGLECOVERAGE][1], self[TOGGLECOVERAGE][2] * 100.0))

        # print children nodes
        if CHILDREN in modules[module]:
                # the dfs part
                for child in modules[module][CHILDREN]:
                    dfs(child[MODULE], level + 1)

    for module in tree_coverage:
        if tree_coverage[module][TYPE] == ROOT:
            dfs(module, 0)

if __name__ == "__main__":
    assert len(sys.argv) == 2, "Expect input_dir"
    input_dir = sys.argv[1]
    pp = pprint.PrettyPrinter(indent=4)

    lines = get_lines(input_dir)
    # print("lines:")
    # pp.pprint(lines)

    annotations = get_line_annotation(lines)
    # print("annotations:")
    # pp.pprint(annotations)

    modules = get_modules(lines)
    # print("modules:")
    # pp.pprint(modules)

    self_coverage = {module: get_coverage_statistics(annotations, modules[module][BEGIN], modules[module][END])
            for module in modules}
    # print("self_coverage:")
    # pp.pprint(self_coverage)

    tree_coverage = get_tree_coverage(modules, self_coverage)
    # print("tree_coverage:")
    # pp.pprint(tree_coverage)

    print("LineSelfCoverage:")
    pp.pprint(sort_coverage(tree_coverage, SELFCOVERAGE, LINECOVERAGE))
    print("LineTreeCoverage:")
    pp.pprint(sort_coverage(tree_coverage, TREECOVERAGE, LINECOVERAGE))

    print("ToggleSelfCoverage:")
    pp.pprint(sort_coverage(tree_coverage, SELFCOVERAGE, TOGGLECOVERAGE))
    print("ToggleTreeCoverage:")
    pp.pprint(sort_coverage(tree_coverage, TREECOVERAGE, TOGGLECOVERAGE))

    print("AllCoverage:")
    print_tree_coverage(tree_coverage)
