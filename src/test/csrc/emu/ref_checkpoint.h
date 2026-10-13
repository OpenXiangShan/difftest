/***************************************************************************************
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

#ifndef __REF_CHECKPOINT_H
#define __REF_CHECKPOINT_H

#include "args.h"
#include <cstdint>
#include <string>

#ifndef CONFIG_NO_DIFFTEST
class Difftest;
#endif

// Periodic REF slices. Independent of fork and difftest mismatches.
// Every auto_checkpoint_interval instructions, skip the point if the DUT is
// in M-mode; otherwise write one slice.
class RefCheckpointScheduler {
public:
  void arm(const CommonArgs &args);
  void disarm() {
    armed_ = false;
  }
  bool armed() const {
    return armed_;
  }
#ifndef CONFIG_NO_DIFFTEST
  bool maybe_take(Difftest *diff);
#endif

private:
  bool armed_ = false;
  bool output_failed_ = false;
  uint64_t interval_ = 0;
  uint64_t next_instr_ = 0;
  uint64_t seq_ = 0;
  std::string base_;

#ifndef CONFIG_NO_DIFFTEST
  bool take(Difftest *diff, uint64_t instr, uint64_t pc, uint64_t mode);
#endif
};

#endif
