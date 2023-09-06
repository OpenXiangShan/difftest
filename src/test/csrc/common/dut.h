/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifndef __DUT_H__
#define __DUT_H__

#include <vector>

#include "common.h"

class DUT {
public:
  DUT() { };
  DUT(int argc, const char *argv[]) { };
  virtual int tick() = 0;
  virtual int is_finished() = 0;
  virtual int is_good() = 0;
};

#define simstats_display(s, ...) \
  eprintf(ANSI_COLOR_GREEN s ANSI_COLOR_RESET, ##__VA_ARGS__)

enum class SimExitCode {
  good_trap,
  exceed_limit,
  bad_trap,
  exception_loop,
  sim_exit,
  difftest,
  unknown
};

class SimStats {
public:
  // simulation exit code
  SimExitCode exit_code;

  SimStats() {
    reset();
  };

  void reset() {
    exit_code = SimExitCode::unknown;
  }

  void update(DiffTestState *state) {
  }

  void display() {
    simstats_display("ExitCode: %d\n", exit_code);
  }
};

extern SimStats stats;

#endif
