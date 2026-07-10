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

struct __attribute__((packed)) TopdownIQInfoFrame {
  uint8_t valid;
  uint16_t robIdx;
  uint8_t robFlag;
  uint8_t pipeNum;
  uint8_t cancelSource;
  uint8_t srcReady;
  uint8_t futype;
  uint8_t issued;
};
static_assert(sizeof(TopdownIQInfoFrame) == 9);

struct __attribute__((packed)) TopdownExtendedIQInfoFrame {
  uint8_t idealIssueTime;
};
static_assert(sizeof(TopdownExtendedIQInfoFrame) == 1);

static bool get_ideal_issue_time(int idx, int entries_num, const TopdownIQInfoFrame *in) {
  int olderCanIssueNum = 0;
  for (int j = 0; j < entries_num; j++) {
    if (j == idx) {
      continue;
    }
    bool valid_j = in[j].valid != 0;
    bool src_ready = valid_j && in[j].srcReady != 0;
    bool differentFlag = in[j].robFlag != in[idx].robFlag;
    bool compare = in[j].robIdx < in[idx].robIdx;
    bool older = differentFlag ^ compare;
    bool sameFutype = in[j].futype == in[idx].futype;
    olderCanIssueNum += older && src_ready && sameFutype && in[j].issued == 0;
  }

  const bool valid = in[idx].valid != 0;
  const bool src_ready = valid && in[idx].srcReady != 0;
  return valid && ((src_ready && olderCanIssueNum < in[idx].pipeNum) || in[idx].issued);
}

} // namespace

extern "C" void topdown_iq_info_dpic(unsigned int entries_num, const svBitVecVal *in_bits, svBitVecVal *out_bits) {
  const auto *in = reinterpret_cast<const TopdownIQInfoFrame *>(in_bits);
  auto *out = reinterpret_cast<TopdownExtendedIQInfoFrame *>(out_bits);

  topdown_zero_bytes(out_bits, entries_num * sizeof(TopdownExtendedIQInfoFrame));
  for (unsigned int i = 0; i < entries_num; i++) {
    out[i].idealIssueTime = get_ideal_issue_time(i, entries_num, in);
  }
}

#define DEFINE_TOPDOWN_IQ_INFO_DPIC(ENTRIES_NUM)                                                          \
  extern "C" void topdown_iq_info_dpic_##ENTRIES_NUM(const svBitVecVal *in_bits, svBitVecVal *out_bits) { \
    topdown_iq_info_dpic(ENTRIES_NUM, in_bits, out_bits);                                                 \
  }

DEFINE_TOPDOWN_IQ_INFO_DPIC(64)
DEFINE_TOPDOWN_IQ_INFO_DPIC(80)
DEFINE_TOPDOWN_IQ_INFO_DPIC(238)
