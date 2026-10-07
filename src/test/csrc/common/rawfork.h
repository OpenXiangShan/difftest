/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
#ifndef DIFFTEST_RAWFORK_H
#define DIFFTEST_RAWFORK_H

class Difftest;
bool difftest_raw_fork_enabled();
bool difftest_raw_fork_is_child();
int difftest_raw_fork_prepare(Difftest *self);
int difftest_raw_fork_check(Difftest *self);
void difftest_raw_fork_publish(Difftest *self);
int difftest_raw_fork_finish();
int difftest_raw_fork_poll();
int difftest_raw_fork_idle();
void difftest_raw_fork_abort_child();

#endif
