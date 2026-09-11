#!/usr/bin/env python3
"""Compile the real transport against deterministic GBus mocks; no runtime/board needed."""
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix="gbus-host-test-") as tmp:
    tmp = pathlib.Path(tmp)
    # Only generated configuration/common dependencies are stubbed. Packet layout
    # and transport implementation are the actual production xdma.h/.cpp headers.
    (tmp / "common.h").write_text('''#pragma once
#include <cstdint>
extern int signal_num;
#define CONFIG_DIFFTEST_BATCH_BYTELEN 80
#define CONFIG_DIFFTEST_HOST_AXIS_BYTES 32
#define CONFIG_DMA_CHANNELS 1
''')
    for name in ("diffstate.h", "mpool.h", "difftest-dpic.h"):
        (tmp / name).write_text('#pragma once\n#include "common.h"\n')
    exe = tmp / "test"
    subprocess.run([
        "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pthread",
        "-I" + str(tmp), "-I" + str(ROOT / "src/test/csrc/fpga"),
        "-I" + str(ROOT / "third_party/gbus_runtime/include"),
        str(ROOT / "src/test/csrc/fpga/gbus_transport.cpp"),
        str(pathlib.Path(__file__).with_name("gbus_dma_window_mock.cpp")),
        "-o", str(exe),
    ], check=True)
    cases = {
        "normal": 0, "retry-short-error": 0, "chunk32": 0, "chunk1024": 0,
        "short": 1, "error": 1, "oversize": 1, "seq-change": 1,
        "second-window-error": 1, "protocol": 1, "unknown": 1,
        "bad-chunk": 1, "bad-length": 1, "bad-base": 1,
        "stop-dma": 0, "signal-dma": 0, "stop-fill": 0,
        "gbs1": 0, "gbs1-single": 0,
        "pre-ack-seq": 1, "pre-ack-status": 1, "ack-rejected": 1, "fill-timeout": 1,
    }
    for case, expected in cases.items():
        result = subprocess.run([str(exe), case], capture_output=True, text=True, timeout=15)
        if result.returncode != expected or "MOCK VERIFIED" not in result.stderr:
            raise AssertionError(f"{case}: rc={result.returncode}, expected={expected}\n{result.stderr}")
        print(f"PASS {case}")
    print(f"PASS {len(cases)} mocked host scenarios")
