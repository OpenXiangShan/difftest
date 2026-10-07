"""Run software-only FAST/fork checks against four compatible NEMU libraries."""

import argparse
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
for name in ["perf", "plain", "old", "hash-disabled"]:
    parser.add_argument("--" + name, required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2] / "build/fast-ref-tests"
refs = {
    "perf": args.perf.resolve(),
    "no-perf": args.plain.resolve(),
    "baseline": args.old.resolve(),
    "default": args.hash_disabled.resolve(),
}
cases = [
    ('slow-old-ref', 'baseline', 'normal', {}, 0),
    ('fast-perf', 'perf', 'normal', {'DIFFTEST_FAST_ONLY': '1'}, 0),
    ('fast-plain', 'no-perf', 'normal', {'DIFFTEST_FAST_ONLY': '1'}, 0),
    ('fast-skip', 'perf', 'skip', {'DIFFTEST_FAST_ONLY': '1'}, 0),
    ('missing-api', 'baseline', 'normal', {'DIFFTEST_FAST_ONLY': '1'}, 2),
    ('missing-hash', 'default', 'normal', {'DIFFTEST_RAW_FORK': '1'}, 2),
    ('fork-perf', 'perf', 'normal', {'DIFFTEST_RAW_FORK': '1'}, 0),
    ('fork-plain', 'no-perf', 'normal', {'DIFFTEST_RAW_FORK': '1'}, 0),
    ('fork-hash', 'perf', 'hash', {'DIFFTEST_RAW_FORK': '1'}, 2),
    ('fork-stamp', 'perf', 'stamp', {'DIFFTEST_RAW_FORK': '1'}, 2),
    ('fork-killed', 'perf', 'killed', {'DIFFTEST_RAW_FORK': '1'}, 2),
    ('fork-hang', 'perf', 'hang', {'DIFFTEST_RAW_FORK': '1', 'DIFFTEST_RAW_FORK_DRAIN_TIMEOUT_MS': '100'}, 2),
    ('fork-segments', 'perf', 'segments', {'DIFFTEST_RAW_FORK': '1', 'DIFFTEST_RAW_FORK_INTERVAL_MS': '3000'}, 0),
    ('fast-short', 'perf', 'stall', {'DIFFTEST_FAST_ONLY': '1'}, 2),
    ('fork-prefix', 'perf', 'prefix', {'DIFFTEST_RAW_FORK': '1', 'DIFFTEST_RAW_FORK_INTERVAL_MS': '3000'}, 2),
    ('fork-out-of-order', 'perf', 'out-of-order', {'DIFFTEST_RAW_FORK': '1', 'DIFFTEST_RAW_FORK_INTERVAL_MS': '3000'}, 0),
]
results = []
for name, config, scenario, settings, expected in cases:
    env = dict(os.environ)
    for key in list(env):
        if key.startswith(("DIFFTEST_FAST", "DIFFTEST_RAW_FORK")):
            del env[key]
    env.update(settings)
    log_path = root / (name + ".log")
    with log_path.open("w") as log:
        run = subprocess.run(
            [str(root / "runtime-test"), str(refs[config]), scenario],
            cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=20,
        )
    passed = run.returncode == expected
    output = log_path.read_text()
    if name == "fork-prefix":
        passed &= output.count(" UNTRUSTED ") == 2 and "TrustedWindows=0" in output
    if name == "fast-short":
        passed &= "FAST short execution" in output
    results.append(dict(name=name, actual=run.returncode, expected=expected, passed=passed))
    print(results[-1], flush=True)
(root / "runtime-results.json").write_text(json.dumps(results, indent=2) + "\n")
raise SystemExit(0 if all(r["passed"] for r in results) else 1)
