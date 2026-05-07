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

#ifndef __TOPDOWN_ROB_INFO_H
#define __TOPDOWN_ROB_INFO_H

#include "common.h"

struct TopdownRobInfoFrame {
  uint8_t valid;
  uint16_t robIdx;
  uint8_t robFlag;
  uint8_t cancelSource;
  uint8_t issued;
  uint8_t idealIssueTime;
};

void topdown_rob_info_apply(int iq_entries_num, int rob_entries_num, const TopdownRobInfoFrame *in, TopdownRobInfoFrame *out);

#endif // __TOPDOWN_ROB_INFO_H