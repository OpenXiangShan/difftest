/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
***************************************************************************************/
#include "replay_dump.h"
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
  expect_eq("heads.ok", replay_dump_head_count(1536, 768), 2);
  expect_eq("heads.bad", replay_dump_head_count(1000, 768), 0);
  ReplayAxiRange range = replay_axi_range(0, 1536, 0, 1, 768);
  expect_eq("heads.from.range", replay_dump_head_count(range.len, 768), 2);

  uint8_t idx = 0;
  if (replay_dump_check_idx(7, &idx, 1) != 0 || idx != 7) {
    fprintf(stderr, "FAIL first idx\n");
    return 1;
  }
  if (replay_dump_check_idx(8, &idx, 0) != 0 || idx != 8) {
    fprintf(stderr, "FAIL next idx\n");
    return 1;
  }
  if (replay_dump_check_idx(10, &idx, 0) == 0) {
    fprintf(stderr, "FAIL gap should be rejected\n");
    return 1;
  }
  if (replay_dump_check_idx(255, &idx, 1) != 0) {
    fprintf(stderr, "FAIL wrap seed\n");
    return 1;
  }
  if (replay_dump_check_idx(0, &idx, 0) != 0) {
    fprintf(stderr, "FAIL wrap increment\n");
    return 1;
  }
  uint8_t mixed[8] = {71, 71, 71, 242, 242, 242, 242, 242};
  uint8_t same[8] = {243, 243, 243, 243, 243, 243, 243, 243};
  expect_eq("head.mixed", replay_dump_head_aligned(mixed, 8), 0);
  expect_eq("head.same", replay_dump_head_aligned(same, 8), 1);
  printf("replay_dump_test PASS heads=%u last=%u\n", replay_dump_head_count(1536, 768), idx);
  return 0;
}
