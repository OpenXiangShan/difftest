#!/usr/bin/env python3
"""Compile the real GBus transport against deterministic mocks, without a board."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
PLUGIN = ROOT / 'src/test/csrc/plugin/uvhs-gbus'
with tempfile.TemporaryDirectory(prefix='gbus-host-test-') as directory:
    tmp = Path(directory)
    (tmp / 'common.h').write_text('''#pragma once
#include <cstdint>
extern int signal_num;
#define CONFIG_DIFFTEST_BATCH_BYTELEN 80
#define CONFIG_DIFFTEST_HOST_AXIS_BYTES 32
#define CONFIG_DMA_CHANNELS 1
''')
    for name in ('diffstate.h', 'mpool.h', 'difftest-dpic.h'):
        (tmp / name).write_text('#pragma once\n#include "common.h"\n')
    exe = tmp / 'test'
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pthread',
                    '-I' + str(tmp), '-I' + str(ROOT / 'src/test/csrc/fpga'), '-I' + str(PLUGIN),
                    str(PLUGIN / 'gbus_transport.cpp'),
                    str(Path(__file__).with_name('gbus_dma_window_mock.cpp')), '-o', str(exe)], check=True)
    good = ('normal', 'chunk32', 'chunk512', 'config-window', 'stop-dma', 'signal-dma', 'stop-fill', 'gbs1', 'gbs1-single',
            'bar-fence-ok', 'bar-fence-self-clear', 'fill-echo')
    bad = ('retry', 'short', 'error', 'oversize', 'seq-change', 'second-window-error', 'protocol', 'unknown',
           'bad-chunk', 'chunk1024', 'bad-length', 'bad-base', 'bad-topology', 'bad-ar', 'no-ar', 'mirror-mismatch',
           'pre-ack-seq', 'ack-rejected', 'fill-timeout', 'gbs1-dma', 'mode-typo', 'mode-empty', 'negative-env',
           'overflow-env', 'bad-ddr-base', 'h2c-overlap', 'h2c-overflow', 'h2c-unaligned',
           'gbs1-probe-error', 'gbs1-burst-error', 'finalize-error',
           'bar-fence-write-error', 'bar-fence-read-error', 'bar-fence-read-zero', 'bar-fence-short',
           'bar-fence-oversize', 'bar-fence-mismatch', 'fill-echo-stuck', 'fill-echo-bad-flag')
    env = {key: value for key, value in os.environ.items() if not key.startswith('GBUS_')}
    for case in good + bad:
        expected = 0 if case in good else 1
        done = subprocess.run([str(exe), case], capture_output=True, text=True, timeout=15, env=env)
        if done.returncode != expected or 'MOCK VERIFIED' not in done.stderr:
            raise AssertionError(f'{case}: rc={done.returncode} expected={expected}\n{done.stderr}')
        print('PASS', case)
    print('ALL PASS:', len(good) + len(bad), 'mocked host scenarios')
