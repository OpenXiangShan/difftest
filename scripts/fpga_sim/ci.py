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

If the section is missing, we invoke env-scripts make targets:
    make get_impl_log CPU=<cpu>
    (fallback) make get_synth_log CPU=<cpu>
and always exit with code 3 so CI marks the step as failed for visibility.
"""

import argparse
import os
import re
import sys
import subprocess

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

def infer_cpu_from_path(path: str) -> str:
    p = path.lower()
    if 'xiangshan' in p:
        return 'xiangshan'
    return 'nutshell'

def try_print_logs_via_make(start_dir: str, cpu: str) -> bool:
    """Invoke env-scripts make targets to print Vivado logs to stderr.

    Tries get_impl_log first, then get_synth_log. Returns True if any output is printed.
    """
    # Run make inside env-scripts/fpga_diff (the vivado-analyse.txt directory)
    env_root = os.path.abspath(start_dir)
    sys.stderr.write(f"[ci.py] using env-scripts/fpga_diff at {env_root}\n")
    printed_any = False
    for target in ('get_impl_log', 'get_synth_log'):
        try:
            sys.stderr.write(f"[ci.py] invoking make -C {env_root} {target} CPU={cpu}\n")
            proc = subprocess.run(
                ['make', '-C', env_root, target, f'CPU={cpu}'],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding='utf-8',
                errors='ignore',
                timeout=600,
            )
            out = proc.stdout or ''
            if proc.returncode == 0 and out.strip():
                sys.stderr.write(f"[ci.py] ---- BEGIN {target} (CPU={cpu}) ----\n")
                sys.stderr.write(out)
                if not out.endswith('\n'):
                    sys.stderr.write('\n')
                sys.stderr.write(f"[ci.py] ---- END {target} ----\n")
                printed_any = True
                # If impl log printed, we can stop; otherwise try synth as fallback
                if target == 'get_impl_log':
                    return True
            else:
                sys.stderr.write(f"[ci.py] {target} failed (rc={proc.returncode}) or produced no usable output\n")
        except Exception as e:
            sys.stderr.write(f"[ci.py] failed to run {target}: {e}\n")
    return printed_any

# (legacy helper removed; log collection now delegated to make targets)

def try_print_runme_logs_by_path(start_dir: str, cpu: str) -> bool:
    cpu = (cpu or 'nutshell').lower()
    if 'xiangshan' in cpu:
        proj = os.path.join(start_dir, 'fpga_xiangshan')
        runs_base = os.path.join(proj, 'fpga_xiangshan.runs')
    else:
        proj = os.path.join(start_dir, 'fpga_nutshell')
        runs_base = os.path.join(proj, 'fpga_nutshell.runs')

    printed = False
    for stage in ('impl_1', 'synth_1'):
        log_path = os.path.join(runs_base, stage, 'runme.log')
        if os.path.exists(log_path):
            try:
                with open(log_path, 'r', encoding='utf-8', errors='ignore') as lf:
                    content = lf.read()
                sys.stderr.write(f"[ci.py] ---- BEGIN {cpu}:{stage} runme.log ----\n")
                sys.stderr.write(content)
                if not content.endswith('\n'):
                    sys.stderr.write('\n')
                sys.stderr.write(f"[ci.py] ---- END {cpu}:{stage} runme.log ----\n")
                printed = True
                if stage == 'impl_1':
                    # Prefer impl_1; if present, no need to fallback to synth_1
                    return True
            except Exception as e:
                sys.stderr.write(f"[ci.py] failed reading {log_path}: {e}\n")
        else:
            sys.stderr.write(f"[ci.py] log not found: {log_path}\n")
    return printed

def main():
    ap = argparse.ArgumentParser(description='Extract "Hierarchical utilization" section from Vivado report.')
    ap.add_argument('-i', '--input', required=True,
                    help=('Path to vivado-analyse.txt, its basename, or a directory containing it. '
                          'Resolution order if a basename is given: '
                          '$ENV_SCRIPTS_HOME/fpga_diff, $GITHUB_WORKSPACE/../env-scripts/fpga_diff, and the current working directory. '
                          'If a directory is provided, the script will search for vivado-analyse.txt inside it.'))
    ap.add_argument('-o', '--output', help='Optional output file path')
    ap.add_argument('--format', choices=['text', 'markdown'], default='text', help='Output formatting')
    ap.add_argument('--fail-missing', action='store_true',
                    help='Exit with code 3 if section missing (default: do not fail).')
    ap.add_argument('--cpu', help='CPU name for env-scripts log helpers (e.g., nutshell, xiangshan). Optional.')
    args = ap.parse_args()

    def search_vivado_analyse_file(folder: str) -> str | None:
        # Try common exact name first
        candidates = [
            os.path.join(folder, 'vivado-analyse.txt'),
            os.path.join(folder, 'vivado_analyse.txt'),
            os.path.join(folder, 'vivado-analyze.txt'),  # typo variant just in case
        ]
        for c in candidates:
            if os.path.exists(c):
                return c
        # Fallback: heuristic scan
        try:
            for name in os.listdir(folder):
                low = name.lower()
                if low.endswith('.txt') and ('vivado' in low and 'analyse' in low):
                    return os.path.join(folder, name)
        except Exception as e:
            sys.stderr.write(f"[ci.py] failed listing directory {folder}: {e}\n")
        return None

    in_path = args.input
    base_dir_for_logs: str | None = None

    # Case A: input is an existing directory -> try to locate vivado-analyse.txt inside
    if os.path.isdir(in_path):
        base_dir_for_logs = os.path.abspath(in_path)
        resolved = search_vivado_analyse_file(base_dir_for_logs)
        if resolved:
            in_path = resolved
            sys.stderr.write(f"[ci.py] resolved input file in directory: {in_path}\n")
        else:
            # Nothing found; attempt to print logs from this directory and fail clearly
            cpu = args.cpu or infer_cpu_from_path(base_dir_for_logs)
            ok = try_print_logs_via_make(base_dir_for_logs, cpu)
            if not ok:
                try_print_runme_logs_by_path(base_dir_for_logs, cpu)
            sys.stderr.write(f"[ci.py] vivado-analyse.txt not found under {base_dir_for_logs}.\n")
            sys.exit(3)

    # Case B: input is not a directory
    if not os.path.exists(in_path):
        # If user only passed a filename, attempt resolution in typical locations
        if os.path.sep not in in_path:
            candidates = []
            env_scripts = os.environ.get('ENV_SCRIPTS_HOME')
            if env_scripts:
                candidates.append(os.path.join(env_scripts, 'fpga_diff', in_path))
            gw = os.environ.get('GITHUB_WORKSPACE')
            if gw:
                candidates.append(os.path.abspath(os.path.join(gw, '..', 'env-scripts', 'fpga_diff', in_path)))
            candidates.append(os.path.join(os.getcwd(), in_path))
            for c in candidates:
                sys.stderr.write(f'[ci.py] trying candidate: {c}\n')
                if os.path.exists(c):
                    in_path = c
                    sys.stderr.write(f'[ci.py] resolved input path to: {in_path}\n')
                    break
        if not os.path.exists(in_path):
            # If we can infer a base directory, try printing logs to aid debugging
            if os.path.sep in args.input:
                base_dir_for_logs = os.path.dirname(os.path.abspath(args.input))
            elif 'ENV_SCRIPTS_HOME' in os.environ:
                base_dir_for_logs = os.path.join(os.environ['ENV_SCRIPTS_HOME'], 'fpga_diff')
            if base_dir_for_logs and os.path.isdir(base_dir_for_logs):
                cpu = args.cpu or infer_cpu_from_path(base_dir_for_logs)
                ok = try_print_logs_via_make(base_dir_for_logs, cpu)
                if not ok:
                    try_print_runme_logs_by_path(base_dir_for_logs, cpu)
                sys.stderr.write(f'[ci.py] Input file not found after resolution attempts: {args.input}\n')
                sys.exit(3)
            else:
                sys.stderr.write(f'[ci.py] Input file not found after resolution attempts: {args.input}\n')
                sys.exit(2)

    with open(in_path, 'r', encoding='utf-8', errors='ignore') as f:
        raw = f.read()

    section = extract_section(raw)
    if section is None:
        base = os.path.dirname(os.path.abspath(in_path))
        cpu = args.cpu or infer_cpu_from_path(base)
        ok = try_print_logs_via_make(base, cpu)
        if not ok:
            try_print_runme_logs_by_path(base, cpu)
        sys.stderr.write('[ci.py] Hierarchical utilization section not found.\n')
        sys.exit(3)
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