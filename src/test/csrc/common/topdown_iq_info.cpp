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

#include "topdown_iq_info.h"

void topdown_iq_info_apply(int entries_num, const TopdownIQInfoFrame *in, TopdownExtendedIQInfoFrame *out) {
  for (int i = 0; i < entries_num; i++) {
    int olderCanIssueNum = 0;
    for (int j = 0; j < entries_num; j++) {
      if (j == i) {
        continue;
      }
      bool valid_j = in[j].valid != 0;
      bool src_ready = valid_j && in[j].srcReady != 0;
      bool differentFlag = in[j].robFlag != in[i].robFlag;
      bool compare = in[j].robIdx < in[i].robIdx;
      bool older = differentFlag ^ compare;
      bool sameFutype = in[j].futype == in[i].futype;
      olderCanIssueNum += older && src_ready && sameFutype && in[j].issued == 0;
      // if (in[i].valid != 0 && (in[i].robIdx == 327 && in[i].robFlag == 0) && (older && src_ready && sameFutype && in[j].issued == 0) && in[i].futype == 16){
      //   printf("i: %d, j: %d, valid_j: %d, src_ready: %d, differentFlag: %d, compare: %d, older: %d, sameFutype: %d, j: issued: %d, robidx: %d, robFlag: %d\n",
      //     i, j, valid_j, src_ready, differentFlag, compare, older, sameFutype, in[j].issued, in[j].robIdx, in[j].robFlag);
      // }
    }


    const bool valid = in[i].valid != 0;
    const bool src_ready = valid && in[i].srcReady != 0;
    out[i].idealIssueTime = valid && ((src_ready && olderCanIssueNum < in[i].pipeNum) || in[i].issued);
    // if (in[i].valid != 0 && (in[i].robIdx == 327) && in[i].futype == 16){
    //   printf("i: %d, olderCanIssueNum: %d, pipeNum: %d out idealIssuetime: %d \n", i, olderCanIssueNum, in[i].pipeNum, out[i].idealIssueTime);
    // }
  }
}