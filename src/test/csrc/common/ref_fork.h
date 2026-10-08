/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
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
#ifndef DIFFTEST_REF_FORK_H
#define DIFFTEST_REF_FORK_H

class Difftest;
#include <cstdint>

// Configure once in main before receive threads or forked readers exist.
bool difftest_ref_fork_init(uint64_t interval_ms, uint64_t drain_timeout_ms);
bool difftest_ref_fork_enabled();
bool difftest_ref_fork_is_child();
int difftest_ref_fork_prepare(Difftest *self);
int difftest_ref_fork_check(Difftest *self);
void difftest_ref_fork_publish(Difftest *self);
int difftest_ref_fork_finish();
int difftest_ref_fork_idle();
void difftest_ref_fork_abort_child();

#endif
