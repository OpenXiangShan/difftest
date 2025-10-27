#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extract the 'Hierarchical utilization' section from a Vivado utilisation report.
The section in your sample starts with:
---- Hierarchical utilization (filtered, max depth: 4) ----
and ends just before:
==================== Timing Summaries (end) ====================
We also stop at any other delimiter line made of ---- / ==== that is not itself
a Hierarchical utilization header.
"""

import argparse
import os
import re
import sys

# Header patterns that indicate the beginning of the hierarchical utilization block.
HIER_HEADER_PATTERNS = [
    re.compile(r'^\s*[-=]{2,}\s*Hierarchical utilization\b.*[-=]{2,}\s*$', re.I),
    re.compile(r'^\s*Hierarchical utilization\b.*$', re.I),
]

# Generic delimiter (other section headings) ¨C a line of dashes or equals enclosing text.
GENERIC_HEADER = re.compile(r'^\s*(?:[-=]{4,}).*(?:[-=]{4,})\s*$')

# Explicit stop keywords (Timing section).
EXPLICIT_STOP_KEYWORDS = [
    'Timing Summaries',
]

def find_start(lines):
    """Locate first line index matching hierarchical utilization header."""
    for i, line in enumerate(lines):
        for pat in HIER_HEADER_PATTERNS:
            if pat.match(line):
                return i
    return None

def is_hier_header(line):
    """True if line itself is a hierarchical utilization header."""
    return any(pat.match(line) for pat in HIER_HEADER_PATTERNS)

def should_stop(line):
    """Decide if we reached end of the section."""
    # Explicit keyword stop
    for kw in EXPLICIT_STOP_KEYWORDS:
        if kw.lower() in line.lower():
            return True
    # Generic header that is NOT the same hierarchical header
    if GENERIC_HEADER.match(line) and not is_hier_header(line):
        return True
    return False

def extract_section(text):
    """Return the hierarchical utilization section or None."""
    lines = text.splitlines()
    start = find_start(lines)
    if start is None:
        return None
    # Collect lines until stop condition
    collected = [lines[start]]
    for j in range(start + 1, len(lines)):
        if should_stop(lines[j]):
            break
        collected.append(lines[j])
    # Trim trailing blank lines
    while collected and collected[-1].strip() == '':
        collected.pop()
    return '\n'.join(collected) + '\n'

def main():
    ap = argparse.ArgumentParser(description='Extract "Hierarchical utilization" section from Vivado report.')
    ap.add_argument('-i', '--input', required=True, help='Path to vivado-analyse.txt')
    ap.add_argument('-o', '--output', help='Optional output file path')
    ap.add_argument('--format', choices=['text', 'markdown'], default='text', help='Output formatting')
    ap.add_argument('--fail-missing', action='store_true',
                    help='Exit with code 3 if section missing (default: do not fail).')
    args = ap.parse_args()

    if not os.path.exists(args.input):
        sys.stderr.write(f'[ci.py] Input file not found: {args.input}\n')
        sys.exit(2)

    with open(args.input, 'r', encoding='utf-8', errors='ignore') as f:
        raw = f.read()

    section = extract_section(raw)
    if section is None:
        msg = 'Hierarchical utilization section not found.'
        if args.fail_missing:
            sys.stderr.write('[ci.py] ' + msg + '\n')
            sys.exit(3)
        # Graceful: print note and exit 0
        out_text = msg + '\n'
    else:
        if args.format == 'markdown':
            out_text = 'Vivado Hierarchical utilization:\n\n```txt\n' + section.rstrip('\n') + '\n```\n'
        else:
            out_text = section

    if args.output:
        os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
        with open(args.output, 'w', encoding='utf-8') as wf:
            wf.write(out_text)
    else:
        sys.stdout.write(out_text)

if __name__ == '__main__':
    main()