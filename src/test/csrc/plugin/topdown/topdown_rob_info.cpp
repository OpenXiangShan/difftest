/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip
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

#include "topdown_dpi.h"

namespace {

struct __attribute__((packed)) TopdownRobInfoFrame {
  uint8_t valid;
  uint16_t robIdx;
  uint8_t robFlag;
  uint8_t cancelSource;
  uint8_t issued;
  uint8_t idealIssueTime;
};
static_assert(sizeof(TopdownRobInfoFrame) == 7);

static void topdown_rob_info_apply(int iq_entries_num, int rob_entries_num, const TopdownRobInfoFrame *in,
                                   TopdownRobInfoFrame *out) {
  for (int i = 0; i < rob_entries_num; i++) {
    out[i].valid = 0;
    out[i].robIdx = i;
    out[i].robFlag = 0;
    out[i].cancelSource = 0;
    out[i].issued = 0;
    out[i].idealIssueTime = 0;
  }

  for (int i = 0; i < iq_entries_num; i++) {
    if (in[i].valid == 0) {
      continue;
    }

    TopdownRobInfoFrame &entry = out[in[i].robIdx];
    entry.valid = 1;
    entry.robIdx = in[i].robIdx;
    entry.robFlag = in[i].robFlag;
    entry.cancelSource = in[i].cancelSource;
    entry.issued = in[i].issued;
    entry.idealIssueTime = in[i].idealIssueTime;
  }
}

} // namespace

extern "C" void topdown_rob_info_dpic(unsigned int iq_entries_num, unsigned int rob_entries_num,
                                      const svBitVecVal *in_bits, svBitVecVal *out_bits) {
  const auto *in = reinterpret_cast<const TopdownRobInfoFrame *>(in_bits);
  auto *out = reinterpret_cast<TopdownRobInfoFrame *>(out_bits);

  topdown_zero_bytes(out_bits, rob_entries_num * sizeof(TopdownRobInfoFrame));
  topdown_rob_info_apply(iq_entries_num, rob_entries_num, in, out);
}

#define DEFINE_TOPDOWN_ROB_INFO_DPIC(IQ_ENTRIES_NUM, ROB_ENTRIES_NUM)                                    \
  extern "C" void topdown_rob_info_dpic_##IQ_ENTRIES_NUM##_##ROB_ENTRIES_NUM(const svBitVecVal *in_bits, \
                                                                             svBitVecVal *out_bits) {    \
    topdown_rob_info_dpic(IQ_ENTRIES_NUM, ROB_ENTRIES_NUM, in_bits, out_bits);                           \
  }

DEFINE_TOPDOWN_ROB_INFO_DPIC(382, 352)
