/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
***************************************************************************************/
#include "replay_range.h"
#include <stdio.h>
#include <stdlib.h>

static void expect_eq(const char *name, uint32_t got, uint32_t want) {
  if (got != want) {
    fprintf(stderr, "FAIL %s: got %u want %u\n", name, got, want);
    exit(1);
  }
}

int main() {
  const uint32_t pack = 768;
  ReplayAxiRange a = replay_axi_range(0, 1536, 0, 1, pack);
  expect_eq("nowrap.start", a.start, 0);
  expect_eq("nowrap.len", a.len, 1536);

  ReplayAxiRange b = replay_axi_range(0, 768, 1, 1, pack);
  expect_eq("wrap.start", b.start, 768);
  expect_eq("wrap.len", b.len, (1048576u / pack) * pack);
  if (b.len % pack != 0) {
    fprintf(stderr, "FAIL wrap.len not pack aligned\n");
    return 1;
  }
  if (replay_aligned_window_bytes(1, pack) != b.len) {
    fprintf(stderr, "FAIL aligned helper mismatch\n");
    return 1;
  }
  printf("replay_range_test PASS start=%u len=%u wrap_start=%u wrap_len=%u\n", a.start, a.len, b.start, b.len);
  return 0;
}
