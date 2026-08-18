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
#include <string.h>

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

#ifndef CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES
#define CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES 32
#endif

#ifndef CONFIG_DIFFTEST_REPLAY_BATCH_INFO_BYTES
#define CONFIG_DIFFTEST_REPLAY_BATCH_INFO_BYTES 32
#endif

typedef struct {
  uint32_t packets;
  uint32_t heads;
  uint32_t steps;
  uint32_t unknown;
  uint32_t skipped;
} ReplayBatchStats;

typedef struct __attribute__((packed)) {
  uint8_t num;
  uint8_t id;
} ReplayBatchInfo;

typedef struct {
  int synced;
  uint32_t info_num;
  uint32_t info_len;
  uint32_t info_bytes;
  uint32_t remain_chunks;
  uint8_t info_buf[CONFIG_DIFFTEST_REPLAY_BATCH_INFO_BYTES];
  ReplayBatchStats stats;
} ReplayBatchParser;

static inline uint32_t replay_batch_align_chunk(uint32_t bytes) {
  const uint32_t chunk = CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES;
  return ((bytes + chunk - 1) / chunk) * chunk;
}

static inline int replay_batch_chunk_zero(const uint8_t *chunk) {
  for (uint32_t i = 0; i < CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES; i++) {
    if (chunk[i] != 0) {
      return 0;
    }
  }
  return 1;
}

static inline void replay_batch_parser_init(ReplayBatchParser *p) {
  memset(p, 0, sizeof(*p));
}

static inline int replay_batch_recv_info(ReplayBatchParser *p, const uint8_t *chunk) {
  const uint32_t chunk_bytes = CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES;
  const uint8_t head_id = (uint8_t)CONFIG_DIFFTEST_REPLAY_BATCH_HEAD;
  const uint8_t step_id = (uint8_t)CONFIG_DIFFTEST_REPLAY_BATCH_STEP;
  const uint8_t path_a_head = (uint8_t)CONFIG_DIFFTEST_BATCH_HEAD;

  if (p->info_len + chunk_bytes > sizeof(p->info_buf)) {
    fprintf(stderr, "[fpga-replay] Replay info overflow\n");
    return -1;
  }
  memcpy(p->info_buf + p->info_len, chunk, chunk_bytes);
  p->info_len += chunk_bytes;
  ReplayBatchInfo *entries = (ReplayBatchInfo *)p->info_buf;

  if (p->info_bytes == 0) {
    if (entries[0].id == path_a_head) {
      p->stats.unknown++;
      fprintf(stderr, "[fpga-replay] Path A BatchHead %u in Replay dump\n", entries[0].id);
      return -1;
    }
    if (entries[0].id != head_id) {
      fprintf(stderr, "[fpga-replay] Replay BatchHead %u not found, got %u\n", head_id, entries[0].id);
      return -1;
    }
    p->info_num = entries[0].num;
    p->info_bytes = replay_batch_align_chunk((p->info_num + 2) * (uint32_t)sizeof(ReplayBatchInfo));
    if (p->info_bytes == 0 || p->info_bytes > sizeof(p->info_buf)) {
      fprintf(stderr, "[fpga-replay] Replay info too large: %u\n", p->info_bytes);
      return -1;
    }
    p->stats.heads++;
    p->synced = 1;
  }

  if (p->info_len < p->info_bytes) {
    return 0;
  }
  if (p->info_len != p->info_bytes) {
    fprintf(stderr, "[fpga-replay] Replay info size mismatch: %u vs %u\n", p->info_len, p->info_bytes);
    return -1;
  }

  ReplayBatchInfo last = entries[p->info_num + 1];
  if (last.id != step_id) {
    fprintf(stderr, "[fpga-replay] Replay BatchStep %u not found, got %u\n", step_id, last.id);
    return -1;
  }
  uint32_t info_chunks = p->info_bytes / chunk_bytes;
  if (last.num < info_chunks) {
    fprintf(stderr, "[fpga-replay] Replay BatchStep.num %u < info chunks %u\n", last.num, info_chunks);
    return -1;
  }
  p->remain_chunks = (uint32_t)last.num - info_chunks;
  p->info_len = 0;
  p->info_bytes = 0;
  p->stats.steps++;
  return 0;
}

static inline int replay_batch_feed_chunk(ReplayBatchParser *p, const uint8_t *chunk) {
  const uint8_t head_id = (uint8_t)CONFIG_DIFFTEST_REPLAY_BATCH_HEAD;
  const ReplayBatchInfo *head = (const ReplayBatchInfo *)chunk;

  if (p->remain_chunks > 0) {
    p->remain_chunks--;
    return 0;
  }

  if (p->info_bytes == 0) {
    if (replay_batch_chunk_zero(chunk) || head->id != head_id) {
      if (!p->synced) {
        p->stats.skipped++;
        return 0;
      }
      if (replay_batch_chunk_zero(chunk)) {
        p->stats.skipped++;
        return 0;
      }
      fprintf(stderr, "[fpga-replay] expected Replay BatchHead %u, got id %u\n", head_id, head->id);
      return -1;
    }
  }
  return replay_batch_recv_info(p, chunk);
}

static inline int v_difftest_ReplayBatch(const uint8_t *payload, size_t nbytes, ReplayBatchStats *stats,
                                         ReplayBatchParser *parser) {
  if (payload == 0 || nbytes == 0 || parser == 0) {
    return -1;
  }
  const uint8_t head_id = (uint8_t)CONFIG_DIFFTEST_REPLAY_BATCH_HEAD;
  const uint8_t path_a_head = (uint8_t)CONFIG_DIFFTEST_BATCH_HEAD;
  if (head_id == path_a_head) {
    fprintf(stderr, "[fpga-replay] Replay BatchHead id %u collides with Path A\n", head_id);
    return -1;
  }
  if ((nbytes % CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES) != 0) {
    fprintf(stderr, "[fpga-replay] Replay beat length %zu is not a chunk multiple\n", nbytes);
    return -1;
  }
  parser->stats.packets++;
  for (size_t off = 0; off < nbytes; off += CONFIG_DIFFTEST_REPLAY_BATCH_CHUNK_BYTES) {
    if (replay_batch_feed_chunk(parser, payload + off) != 0) {
      if (stats) {
        *stats = parser->stats;
      }
      return -1;
    }
  }
  if (stats) {
    *stats = parser->stats;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif // __REPLAY_BATCH_H__
