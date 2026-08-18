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

  ReplayBatchParser parser;
  replay_batch_parser_init(&parser);

  uint8_t payload[64] = {0};
  payload[0] = 1;
  payload[1] = CONFIG_DIFFTEST_REPLAY_BATCH_HEAD;
  payload[2] = 1;
  payload[3] = 0;
  payload[4] = 2;
  payload[5] = CONFIG_DIFFTEST_REPLAY_BATCH_STEP;
  ReplayBatchStats st;
  if (v_difftest_ReplayBatch(payload, sizeof(payload), &st, &parser) != 0) {
    fprintf(stderr, "FAIL parse replay payload\n");
    return 1;
  }
  expect_eq("heads", st.heads, 1);
  expect_eq("steps", st.steps, 1);
  expect_eq("remain.after.step", parser.remain_chunks, 0);

  uint8_t cont[64] = {0};
  if (v_difftest_ReplayBatch(cont, sizeof(cont), &st, &parser) != 0) {
    fprintf(stderr, "FAIL parse zero padding beat\n");
    return 1;
  }
  expect_eq("heads.pad", st.heads, 1);

  uint8_t mid[64];
  memset(mid, 0xa5, sizeof(mid));
  ReplayBatchParser sync_parser;
  replay_batch_parser_init(&sync_parser);
  if (v_difftest_ReplayBatch(mid, sizeof(mid), &st, &sync_parser) != 0) {
    fprintf(stderr, "FAIL unsynced mid-step beat should be skipped\n");
    return 1;
  }
  if (v_difftest_ReplayBatch(payload, sizeof(payload), &st, &sync_parser) != 0) {
    fprintf(stderr, "FAIL sync on later BatchHead\n");
    return 1;
  }
  expect_eq("sync.heads", st.heads, 1);

  ReplayBatchParser path_a_parser;
  replay_batch_parser_init(&path_a_parser);
  path_a_parser.synced = 1;
  uint8_t path_a[64] = {0};
  path_a[0] = 3;
  path_a[1] = CONFIG_DIFFTEST_BATCH_HEAD;
  if (v_difftest_ReplayBatch(path_a, sizeof(path_a), &st, &path_a_parser) == 0) {
    fprintf(stderr, "FAIL Path A payload should not parse as Replay\n");
    return 1;
  }
  printf("replay_batch_test PASS head=%u step=%u\n", CONFIG_DIFFTEST_REPLAY_BATCH_HEAD,
         CONFIG_DIFFTEST_REPLAY_BATCH_STEP);
  return 0;
}
