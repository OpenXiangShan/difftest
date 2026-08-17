#!/usr/bin/env bash

set -euo pipefail

zig_bin=${ZIG:-zig}
jobs=${JOBS:-2}
target=${MACOS_TARGET:-aarch64-macos-none}
output_dir=${OUTPUT_DIR:-artifact/macos-arm64}
profile=$(mktemp)

cleanup() {
  rm -f "$profile"
}
trap cleanup EXIT

"$zig_bin" version
mill -i design.compile
make
cp build/generated-src/difftest_profile.json "$profile"
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd)

build_variant() {
  local name=$1
  local config=$2
  local generate_args=(PROFILE="$profile" NUM_CORES=4)

  if [[ -n "$config" ]]; then
    generate_args+=(CONFIG="$config")
  fi

  make clean
  make "${generate_args[@]}"
  make emu -j"$jobs" \
    WITH_CHISELDB=0 \
    WITH_CONSTANTIN=0 \
    NO_ZSTD_COMPRESSION=1 \
    IMAGE_GZ_COMPRESS=0 \
    IMAGE_ELF=0 \
    OPT_FAST=-O3 \
    VERILATOR_LDFLAGS=-pthread \
    UNAME_S=Darwin \
    CFG_LDFLAGS_VERILATED= \
    CFG_LDLIBS_THREADS=-pthread \
    LDFLAGS= \
    USER_CPPFLAGS=-DVL_TIME_CONTEXT \
    AR="$zig_bin ar" \
    CC="$zig_bin cc -target $target -Wno-error=date-time" \
    CXX="$zig_bin c++ -target $target -Wno-error=date-time" \
    LINK="$zig_bin c++ -target $target"

  cp build/verilator-compile/emu "$output_dir/difftest-emu-$name"
  file "$output_dir/difftest-emu-$name" | grep -F "Mach-O 64-bit arm64 executable"
}

build_variant default ""
build_variant squash S
build_variant soft-arch-update U
