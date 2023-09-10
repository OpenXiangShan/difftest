/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "dut.h"

SimStats stats;

extern "C" uint32_t get_cover_number() {
  if (auto c = stats.get_feedback_cover()) {
    return c->get_total_points();
  }
  return 0;
}

extern "C" void update_stats(uint8_t *icover_bitmap) {
  if (auto c = stats.get_feedback_cover()) {
    c->to_covered_bytes(icover_bitmap);
  }
}

extern "C" void display_uncovered_points() {
  stats.display_uncovered_points();
}

extern "C" void set_cover_feedback(const char *name) {
  stats.set_feedback_cover(name);
}
