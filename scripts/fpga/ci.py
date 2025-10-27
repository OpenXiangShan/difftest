#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extract the 'Hierarchical utilization' section from a Vivado utilisation report.
The section in your sample starts with:
---- Hierarchical utilization ----
and ends just before:
==================== Timing Summaries (end) ====================
We also stop at any other delimiter line made of ---- / ==== that is not itself
a Hierarchical utilization header.
"""

import argparse
import os
import re
import sys
import subprocess
from datetime import datetime
from glob import glob

# Header patterns that indicate the beginning of the hierarchical utilization block.
HIER_HEADER_PATTERNS = [
    re.compile(r'^\s*[-=]{2,}\s*Hierarchical utilization\b.*[-=]{2,}\s*$', re.I),
    re.compile(r'^\s*Hierarchical utilization\b.*$', re.I),
]

# Generic delimiter (other section headings) – a line of dashes or equals enclosing text.
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
    if GENERIC_HEADER.match(line):
        # If all chars are = or -, this is not a generic header
        if len(set(list(line.strip()))) == 1:
            return False
        else:
            if not is_hier_header(line):
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
            sys.stderr.write(f"[ci.py] invoking (cd {env_root} && make {target} CPU={cpu})\n")
            proc = subprocess.run(
                ['make', target, f'CPU={cpu}'],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding='utf-8',
                errors='ignore',
                cwd=env_root,
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

def try_print_runme_logs_by_path(start_dir: str, cpu: str) -> bool:
    cpu = (cpu or 'nutshell').lower()
    proj = os.path.join(start_dir, 'fpga_xiangshan' if 'xiangshan' in cpu else 'fpga_nutshell')
    runs_base = os.path.join(proj, f'{proj.split("/")[-1]}.runs')

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
    ap.add_argument('-i', '--input', required=True, help='Path to vivado-analyse.txt or directory containing it')
    ap.add_argument('--csv-output', help='Optional CSV output file')
    ap.add_argument('--filter-instance-root', default='U_CPU_TOP', help='Instance root name for CSV filtering (default: U_CPU_TOP)')
    ap.add_argument('--filter-modules', default='HostEndpoint,GatewayEndpoint,nutcore', help='Comma-separated modules to keep (case-insensitive)')
    ap.add_argument('--csv-md-output', help='Markdown table output path')
    ap.add_argument('--format', choices=['text', 'markdown'], default='text', help='Output formatting')
    ap.add_argument('--cpu', help='CPU name for log helpers (e.g., nutshell, xiangshan)')
    args = ap.parse_args()

    def search_vivado_analyse_file(folder: str) -> str | None:
        # Try common exact name first
        candidates = [os.path.join(folder, 'vivado-analyse.txt')]
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

    if not os.path.isdir(in_path):
        sys.stderr.write(f"[ci.py] Input path is not a directory: {in_path}\n")
        sys.exit(2)
        
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
        if args.format == 'markdown' or (args.csv_md_output and args.csv_md_output.endswith('.md')):
            out_text = 'Vivado Hierarchical utilization:\n\n```txt\n' + section.rstrip('\n') + '\n```\n'
        else:
            out_text = section
    # Derive cpu_name early for summary usage
    cpu_name_for_summary = args.cpu or infer_cpu_from_path(os.path.dirname(os.path.abspath(in_path)))
    extracted_rows_count = None

    # Optional: emit filtered CSV according to rules
    if args.csv_output or args.csv_md_output or os.environ.get('DIFFTEST_LOG'):
        try:
            import csv

            def parse_table(section_text):
                lines = [ln for ln in section_text.splitlines() if ln.strip()]
                # Detect header: line that contains both 'Instance' and 'Module'
                header_idx = None
                headers = None
                for i, ln in enumerate(lines):
                    if ln.strip().startswith('|') and ln.strip().endswith('|'):
                        cols = [c for c in ln.split('|')[1:-1]]
                        norm = [c.strip() for c in cols]
                        if any(h.lower() == 'instance' for h in norm) and any(h.lower() == 'module' for h in norm):
                            headers = norm
                            header_idx = i
                            break
                # If not found, assume first data row shape and synthesize headers
                data_start = header_idx + 1 if header_idx is not None else 0
                data_rows = []
                raw_instance_cells = []
                # Try determine column count from first data row
                for ln in lines[data_start:]:
                    if not ln.strip().startswith('|'):
                        continue
                    cols = [c for c in ln.split('|')[1:-1]]
                    # keep raw instance cell for indent analysis (do not strip left spaces)
                    instance_raw = cols[0]
                    row = [c.strip() for c in cols]
                    data_rows.append(row)
                    raw_instance_cells.append(instance_raw)
                if not data_rows:
                    return [], [], []
                if headers is None:
                    # Synthesize generic headers with first two as Instance/Module by convention
                    n = len(data_rows[0])
                    headers = ['Instance', 'Module'] + [f'Col{i}' for i in range(3, n + 1)]
                return headers, data_rows, raw_instance_cells

            def leading_spaces(s: str) -> int:
                return len(s) - len(s.lstrip(' '))

            headers, rows, raw_instances = parse_table(section)
            if headers and rows:
                # Find column indices
                try:
                    inst_idx = next(i for i, h in enumerate(headers) if h.lower() == 'instance')
                except StopIteration:
                    inst_idx = 0
                try:
                    mod_idx = next(i for i, h in enumerate(headers) if h.lower() == 'module')
                except StopIteration:
                    mod_idx = 1 if len(headers) > 1 else 0

                # Locate root row
                root_name = (args.filter_instance_root or '').strip()
                root_pos = None
                for i, r in enumerate(rows):
                    if i < len(raw_instances):
                        name = r[inst_idx]
                        if name.strip() == root_name:
                            root_pos = i
                            break
                filtered_rows = []
                if root_pos is not None:
                    base_indent = leading_spaces(raw_instances[root_pos])
                    # include root
                    filtered_rows.append(rows[root_pos])
                    # include subtree until indent <= base_indent and not the same row
                    for i in range(root_pos + 1, len(rows)):
                        ind = leading_spaces(raw_instances[i]) if i < len(raw_instances) else 9999
                        if ind <= base_indent:
                            break
                        filtered_rows.append(rows[i])
                else:
                    # If not found, keep all as a fallback
                    filtered_rows = rows[:]

                # Module filtering within subtree
                keep_modules = [s.strip().lower() for s in (args.filter_modules or '').split(',') if s.strip()]

                def keep_row(r):
                    # Always keep the root row if present
                    if r[inst_idx].strip() == root_name:
                        return True
                    inst_val = r[inst_idx].strip()
                    # Exclude purely parenthesized instance variants like '(nutcore)' to avoid duplicate internal rows
                    if inst_val.startswith('(') and inst_val.endswith(')'):
                        return False
                    m = r[mod_idx].strip().lower() if mod_idx < len(r) else ''
                    # Only allow exact (case-insensitive) module name matches
                    return any(m == km for km in keep_modules)

                final_rows = [r for r in filtered_rows if keep_row(r)]
                extracted_rows_count = len(final_rows)

                # Augment Total LUTs/LUTs cell with percentage of VU19P capacity (4,000,000 LUTs).
                # Example: "113409" -> "113409 (2.8%)" (one decimal place).
                def _parse_luts(val: str) -> int:
                    if not val:
                        return 0
                    # Remove commas and surrounding spaces
                    s = val.replace(',', ' ').strip()
                    # Extract leading integer sequence
                    m = re.match(r'(\d+)', s)
                    if not m:
                        return 0
                    try:
                        return int(m.group(1))
                    except Exception:
                        return 0

                def _norm(h: str) -> str:
                    return re.sub(r'[^a-z0-9]', '', (h or '').lower())

                def _find_total_luts_index(hdrs):
                    nh = [_norm(h) for h in hdrs]
                    # 1) Prefer exact "totalluts"
                    for i, x in enumerate(nh):
                        if x == 'totalluts':
                            return i
                    # 2) Accept exact "luts"
                    for i, x in enumerate(nh):
                        if x == 'luts':
                            return i
                    # 3) Fallback: any header containing 'luts' but not 'logic'/'ram'/'lutram'/'srl'
                    for i, x in enumerate(nh):
                        if 'luts' in x and all(k not in x for k in ('logic', 'ram', 'lutram', 'srl')):
                            return i
                    return None

                luts_idx = _find_total_luts_index(headers)

                if luts_idx is not None:
                    for r in final_rows:
                        lut_val = r[luts_idx] if luts_idx < len(r) else ''
                        lut_num = _parse_luts(lut_val)
                        pct_4m = (lut_num / 4_000_000.0) * 100.0
                        # Preserve original text and append percentage in parentheses with one decimal
                        display_val = lut_val.strip()
                        # Avoid double-adding if already contains parentheses
                        if '(' not in display_val and ')' not in display_val:
                            r[luts_idx] = f"{display_val} ({pct_4m:.1f}%)"
                        else:
                            # If already formatted, replace content inside or append safely
                            r[luts_idx] = f"{display_val}"

                # Prepare markdown table for current run
                def build_md_table(headers_in, rows_in):
                    lines = []
                    lines.append('| ' + ' | '.join(headers_in) + ' |')
                    lines.append('| ' + ' | '.join(['---'] * len(headers_in)) + ' |')
                    for rr in rows_in:
                        if len(rr) < len(headers_in):
                            rr = rr + [''] * (len(headers_in) - len(rr))
                        lines.append('| ' + ' | '.join(rr) + ' |')
                    return lines

                # Archive management in DIFFTEST_LOG (keep last 5 days, overwrite same-day)
                cpu_name = (args.cpu or infer_cpu_from_path(os.path.dirname(os.path.abspath(in_path)))).lower()
                difftest_log_root = os.environ.get('DIFFTEST_LOG')
                archive_csv_path = None
                archive_md_path = None
                now = datetime.now()
                date_str = now.strftime('%Y%m%d')
                ts_human = now.strftime('%Y-%m-%d %H:%M')
                if difftest_log_root:
                    archive_dir = os.path.join(difftest_log_root, 'vivado_hier', cpu_name)
                    try:
                        os.makedirs(archive_dir, exist_ok=True)
                    except Exception as e:
                        sys.stderr.write(f"[ci.py] failed to create archive dir {archive_dir}: {e}\n")
                    archive_csv_path = os.path.join(archive_dir, f'vivado_hier_{cpu_name}_{date_str}.csv')
                    archive_md_path = os.path.join(archive_dir, f'vivado_hier_{cpu_name}_{date_str}.md')
                    try:
                        with open(archive_csv_path, 'w', newline='', encoding='utf-8') as cf:
                            writer = csv.writer(cf)
                            writer.writerow(headers)
                            writer.writerows(final_rows)
                    except Exception as e:
                        sys.stderr.write(f"[ci.py] failed writing archive csv {archive_csv_path}: {e}\n")

                    # Retention: keep last 5 daily files
                    try:
                        pattern = os.path.join(archive_dir, f'vivado_hier_{cpu_name}_*.csv')
                        files = sorted(glob(pattern))
                        # sort by filename which contains YYYYMMDD; keep last 5
                        if len(files) > 5:
                            to_delete = files[0:len(files)-5]
                            for p in to_delete:
                                try:
                                    os.remove(p)
                                    sys.stderr.write(f"[ci.py] pruned old archive: {p}\n")
                                except Exception as e:
                                    sys.stderr.write(f"[ci.py] failed to remove old archive {p}: {e}\n")
                                # try remove matching md as well
                                try:
                                    md_p = p[:-4] + '.md' if p.endswith('.csv') else p + '.md'
                                    if os.path.exists(md_p):
                                        os.remove(md_p)
                                        sys.stderr.write(f"[ci.py] pruned old archive md: {md_p}\n")
                                except Exception as e:
                                    sys.stderr.write(f"[ci.py] failed to remove old archive md {md_p}: {e}\n")
                    except Exception as e:
                        sys.stderr.write(f"[ci.py] retention check failed: {e}\n")

                # Also write CSV to explicitly requested path (if provided)
                if args.csv_output:
                    try:
                        os.makedirs(os.path.dirname(os.path.abspath(args.csv_output)), exist_ok=True)
                        with open(args.csv_output, 'w', newline='', encoding='utf-8') as cf:
                            writer = csv.writer(cf)
                            writer.writerow(headers)
                            writer.writerows(final_rows)
                    except Exception as e:
                        sys.stderr.write(f"[ci.py] failed writing csv_output {args.csv_output}: {e}\n")

                # Build combined markdown (current + latest archive from previous day)
                if args.csv_md_output:
                    try:
                        os.makedirs(os.path.dirname(os.path.abspath(args.csv_md_output)), exist_ok=True)
                    except Exception:
                        pass

                    current_md_lines = build_md_table(headers, final_rows)
                    combined_lines = []
                    combined_lines.append(f"Current result ({cpu_name}) - {ts_human}")
                    combined_lines.append('')
                    combined_lines.extend(current_md_lines)

                    # Find previous day's archive for comparison
                    prev_archive_md_lines = None
                    prev_label = None
                    if difftest_log_root:
                        try:
                            archive_dir = os.path.join(difftest_log_root, 'vivado_hier', cpu_name)
                            pattern = os.path.join(archive_dir, f'vivado_hier_{cpu_name}_*.csv')
                            files = sorted(glob(pattern))
                            # pick the latest file with date < today
                            prev_files = [p for p in files if os.path.basename(p).split('_')[-1].split('.')[0] < date_str]
                            if prev_files:
                                prev_path = prev_files[-1]
                                prev_date = os.path.basename(prev_path).split('_')[-1].split('.')[0]
                                prev_label = f"Archive (latest, {prev_date})"
                                try:
                                    with open(prev_path, 'r', encoding='utf-8') as pf:
                                        import csv as _csv
                                        reader = list(_csv.reader(pf))
                                        if reader:
                                            prev_headers = reader[0]
                                            prev_rows = reader[1:]
                                            prev_archive_md_lines = build_md_table(prev_headers, prev_rows)
                                except Exception as e:
                                    sys.stderr.write(f"[ci.py] failed to read previous archive {prev_path}: {e}\n")
                        except Exception as e:
                            sys.stderr.write(f"[ci.py] failed to locate previous archive: {e}\n")

                    combined_lines.extend(['', '---', ''])
                    if prev_archive_md_lines:
                        combined_lines.append(prev_label or 'Archive (latest)')
                        combined_lines.append('')
                        combined_lines.extend(prev_archive_md_lines)
                    else:
                        combined_lines.append('Archive: no previous record found (first run or less than one day)')

                    try:
                        with open(args.csv_md_output, 'w', encoding='utf-8') as mf:
                            mf.write('\n'.join(combined_lines) + '\n')
                    except Exception as e:
                        sys.stderr.write(f"[ci.py] failed writing markdown output {args.csv_md_output}: {e}\n")

                    # Also archive markdown under DIFFTEST_LOG (same date-based name)
                    if archive_md_path:
                        try:
                            with open(archive_md_path, 'w', encoding='utf-8') as amf:
                                amf.write('\n'.join(combined_lines) + '\n')
                        except Exception as e:
                            sys.stderr.write(f"[ci.py] failed writing archive md {archive_md_path}: {e}\n")
            else:
                sys.stderr.write('[ci.py] CSV: No rows parsed from utilization section.\n')
        except Exception as e:
            sys.stderr.write(f'[ci.py] CSV generation failed: {e}\n')

    if extracted_rows_count is not None:
        sys.stdout.write(f"[ci.py] extraction succeeded: {extracted_rows_count} filtered rows for CPU {cpu_name_for_summary}\n")
    else:
        sys.stdout.write(f"[ci.py] extraction succeeded for CPU {cpu_name_for_summary}\n")

if __name__ == '__main__':
    main()
