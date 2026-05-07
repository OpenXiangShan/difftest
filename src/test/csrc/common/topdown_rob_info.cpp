/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include "topdown_rob_info.h"

void topdown_rob_info_apply(int iq_entries_num, int rob_entries_num, const TopdownRobInfoFrame *in,
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
