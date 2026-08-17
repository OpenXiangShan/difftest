/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
***************************************************************************************/
#ifndef __REPLAY_BATCH_H__
#define __REPLAY_BATCH_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_DIFFTEST_REPLAY_BATCH_HEAD
#ifdef __has_include
#if __has_include("difftest-replay-batch.h")
#include "difftest-replay-batch.h"
#endif
#endif
#endif

#ifndef CONFIG_DIFFTEST_REPLAY_BATCH_HEAD
#define CONFIG_DIFFTEST_REPLAY_BATCH_HEAD 7
#define CONFIG_DIFFTEST_REPLAY_BATCH_STEP 8
#endif

#ifndef CONFIG_DIFFTEST_BATCH_HEAD
#define CONFIG_DIFFTEST_BATCH_HEAD 6
#endif

typedef struct {
  uint32_t packets;
  uint32_t heads;
  uint32_t steps;
  uint32_t unknown;
} ReplayBatchStats;

static inline int v_difftest_ReplayBatch(const uint8_t *payload, size_t nbytes, ReplayBatchStats *stats) {
  if (payload == 0 || nbytes == 0) {
    return -1;
  }
  ReplayBatchStats local = {0, 0, 0, 0};
  if (stats) {
    *stats = local;
  }
  const uint8_t head_id = (uint8_t)CONFIG_DIFFTEST_REPLAY_BATCH_HEAD;
  const uint8_t step_id = (uint8_t)CONFIG_DIFFTEST_REPLAY_BATCH_STEP;
  const uint8_t path_a_head = (uint8_t)CONFIG_DIFFTEST_BATCH_HEAD;
  if (head_id == path_a_head) {
    fprintf(stderr, "[fpga-replay] Replay BatchHead id %u collides with Path A\n", head_id);
    return -1;
  }
  /* Packed BatchInfo is {num,id} little-endian as used by Path A parser. */
  for (size_t off = 0; off + 1 < nbytes; off++) {
    uint8_t num = payload[off];
    uint8_t id = payload[off + 1];
    if (id == head_id) {
      local.heads++;
    } else if (id == step_id) {
      local.steps++;
    } else if (id == path_a_head && num != 0) {
      local.unknown++;
    }
  }
  local.packets = 1;
  if (stats) {
    *stats = local;
  }
  if (local.heads == 0) {
    fprintf(stderr, "[fpga-replay] Replay BatchHead %u not found\n", head_id);
    return -1;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif // __REPLAY_BATCH_H__
