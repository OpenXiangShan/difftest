/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/
#ifndef __REPLAY_DUMP_H__
#define __REPLAY_DUMP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t replay_dump_head_count(uint32_t len, uint32_t pack_size) {
  if (pack_size == 0 || (len % pack_size) != 0) {
    return 0;
  }
  return len / pack_size;
}

/* Returns 0 on success. first=1 accepts any idx and seeds *expect. */
static inline int replay_dump_check_idx(uint8_t got, uint8_t *expect, int first) {
  if (expect == 0) {
    return -1;
  }
  if (first) {
    *expect = got;
    return 0;
  }
  uint8_t next = (uint8_t)(*expect + 1u);
  if (got != next) {
    return -1;
  }
  *expect = got;
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif // __REPLAY_DUMP_H__
