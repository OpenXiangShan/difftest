/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#ifndef __REFDRIVE_TRACE_H__
#define __REFDRIVE_TRACE_H__

#include "args.h"
#include "difftrace.h"
#include "fork_ref/ref_drive.h"

struct RefDriveTraceEntry {
  RefDriveState state{};
  uint8_t terminal = 0;
};

void difftest_refdrive_trace_configure(const CommonArgs &args);
void difftest_refdrive_trace_record(const DiffTestState &dut);
bool difftest_refdrive_trace_pending();
void difftest_refdrive_trace_finish();
void difftest_refdrive_trace_check_load_correction();

#endif
