/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
***************************************************************************************/
#include "replay_batch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expect_eq(const char *name, uint32_t got, uint32_t want) {
  if (got != want) {
    fprintf(stderr, "FAIL %s: got %u want %u\n", name, got, want);
    exit(1);
  }
}

int main() {
  expect_eq("ids.distinct", CONFIG_DIFFTEST_REPLAY_BATCH_HEAD == CONFIG_DIFFTEST_BATCH_HEAD ? 1 : 0, 0);
  uint8_t payload[16] = {0};
  payload[0] = 3;
  payload[1] = CONFIG_DIFFTEST_REPLAY_BATCH_HEAD;
  payload[4] = 2;
  payload[5] = CONFIG_DIFFTEST_REPLAY_BATCH_STEP;
  ReplayBatchStats st;
  if (v_difftest_ReplayBatch(payload, sizeof(payload), &st) != 0) {
    fprintf(stderr, "FAIL parse replay payload\n");
    return 1;
  }
  expect_eq("heads", st.heads, 1);
  expect_eq("steps", st.steps, 1);

  uint8_t path_a[8] = {3, CONFIG_DIFFTEST_BATCH_HEAD, 0, 0, 0, 0, 0, 0};
  if (v_difftest_ReplayBatch(path_a, sizeof(path_a), &st) == 0) {
    fprintf(stderr, "FAIL Path A payload should not parse as Replay\n");
    return 1;
  }
  printf("replay_batch_test PASS head=%u step=%u\n", CONFIG_DIFFTEST_REPLAY_BATCH_HEAD,
         CONFIG_DIFFTEST_REPLAY_BATCH_STEP);
  return 0;
}
