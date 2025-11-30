/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
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

#include "checker.h"

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
bool MhpmeventOverflowChecker::get_valid(const DifftestMhpmeventOverflowEvent &probe) {
  return probe.valid;
}

void MhpmeventOverflowChecker::clear_valid(DifftestMhpmeventOverflowEvent &probe) {
  probe.valid = 0;
}

int MhpmeventOverflowChecker::check(const DifftestMhpmeventOverflowEvent &probe) {
  proxy->mhpmevent_overflow(probe.mhpmeventOverflow);
  return 0;
}
#endif // CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
