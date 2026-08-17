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
#ifndef __REPLAY_RANGE_H__
#define __REPLAY_RANGE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t start;
  uint32_t len;
} ReplayAxiRange;

static inline uint32_t replay_aligned_window_bytes(uint32_t size_mb, uint32_t pack_size) {
  uint32_t window = size_mb << 20;
  if (pack_size == 0) {
    return 0;
  }
  return window - (window % pack_size);
}

static inline uint32_t replay_c2h_range_bytes(uint32_t batch_bits, uint32_t axis_bits) {
  uint32_t axis_bytes = axis_bits / 8;
  uint32_t pkt_axis = (batch_bits + 8 + axis_bits - 1) / axis_bits;
  return 8u * pkt_axis * axis_bytes;
}

static inline ReplayAxiRange replay_axi_range(uint32_t base, uint32_t wr_ptr, uint32_t wrap_cnt, uint32_t size_mb,
                                              uint32_t pack_size) {
  ReplayAxiRange range = {base, 0};
  uint32_t aligned = replay_aligned_window_bytes(size_mb, pack_size);
  if (wrap_cnt == 0) {
    range.start = base;
    range.len = wr_ptr - base;
  } else {
    range.start = wr_ptr;
    range.len = aligned;
  }
  return range;
}

#ifdef __cplusplus
}
#endif

#endif // __REPLAY_RANGE_H__
