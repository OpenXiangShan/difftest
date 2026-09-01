#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
runtime_root=${GBUS_RUNTIME_ROOT:-$repo/../env-scripts/fpga_diff/third_party/gbus_runtime}
build_dir=${GBUS_TRANSPORT_TEST_BUILD_DIR:-/tmp/difftest-gbus-transport-mock}
cxx=${CXX:-g++}

test -f "$runtime_root/include/uvaps_gbus_runtime.h"
rm -rf "$build_dir"
mkdir -p "$build_dir"

"$cxx" -std=c++20 -O2 -pthread \
  -DCONFIG_DMA_CHANNELS=1 \
  -DCONFIG_DIFFTEST_BATCH_BYTELEN=64 \
  -DCONFIG_DIFFTEST_HOST_AXIS_BYTES=32 \
  -I"$repo/src/test/csrc/fpga" \
  -I"$repo/src/test/csrc/common" \
  -I"$repo/src/test/csrc/difftest" \
  -I"$repo/config" \
  -I"$repo/build/generated-src" \
  -I"$runtime_root/include" \
  "$repo/src/test/csrc/fpga/gbus_transport.cpp" \
  "$repo/scripts/fpga/tests/gbus_transport_mock_test.cpp" \
  -o "$build_dir/gbus_transport_mock_test"

"$build_dir/gbus_transport_mock_test"
