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

#include "common.h"
#include "coverage.h"
#include <vector>

class DUT {
public:
  DUT() {};
  DUT(int argc, const char *argv[]) {};
  virtual int tick() = 0;
  virtual int is_finished() = 0;
  virtual int is_good() = 0;
};

#define simstats_display(s, ...) eprintf(ANSI_COLOR_GREEN s ANSI_COLOR_RESET, ##__VA_ARGS__)

enum class SimExitCode {
  good_trap,
  exceed_limit,
  bad_trap,
  exception_loop,
  ambiguous,
  sim_exit,
  difftest,
  unknown
};

class SimStats {
public:
  // coverage statistics
  std::vector<Coverage *> cover;
  // simulation exit code
  SimExitCode exit_code;

  SimStats() {
#ifdef CONFIG_DIFFTEST_INSTRCOVER
    auto c_instr = new InstrCoverage;
    cover.push_back(c_instr);
#endif // CONFIG_DIFFTEST_INSTRCOVER
#ifdef CONFIG_DIFFTEST_INSTRIMMCOVER
    auto c_instrimm = new InstrImmCoverage;
    cover.push_back(c_instrimm);
#endif // CONFIG_DIFFTEST_INSTRIMMCOVER
#ifdef FIRRTL_COVER
    auto c_firrtl = new FIRRTLCoverage;
    cover.push_back(c_firrtl);
#endif // FIRRTL_COVER
#ifdef LLVM_COVER
    auto c_llvm = new LLVMSanCoverage;
    cover.push_back(c_llvm);
#endif // LLVM_COVER

    // #if defined(CONFIG_DIFFTEST_INSTRCOVER) && defined(FIRRTL_COVER)
    //     auto c_union_instr_firrtl = new UnionCoverage(c_instr, c_firrtl);
    //     cover.push_back(c_union_instr_firrtl);
    // #endif // CONFIG_DIFFTEST_INSTRCOVER && FIRRTL_COVER
    // #if defined(CONFIG_DIFFTEST_INSTRIMMCOVER) && defined(FIRRTL_COVER)
    //     auto c_union_instrimm_firrtl = new UnionCoverage(c_instrimm, c_firrtl);
    //     cover.push_back(c_union_instrimm_firrtl);
    // #endif // CONFIG_DIFFTEST_INSTRIMMCOVER && FIRRTL_COVER
    reset();
  };

  void reset() {
    for (auto cov: cover) {
      cov->reset();
    }
    exit_code = SimExitCode::unknown;
  }

  void update(DiffTestState *state) {
    for (auto cov: cover) {
      cov->update(state);
    }
  }

  void display() {
    for (auto cov: cover) {
      cov->display();
    }
    // simstats_display("ExitCode: %d\n", (int)exit_code);
  }

  void display_uncovered_points() {
    for (auto cov: cover) {
      cov->display_uncovered_points();
    }
    fflush(stdout);
  }

  void accumulate() {
    for (auto cov: cover) {
      cov->accumulate();
    }
  }

  void set_feedback_cover(const char *name) {
    for (auto cov: cover) {
      cov->update_is_feedback(name);
    }
  }

  Coverage *get_feedback_cover() {
    for (auto cov: cover) {
      if (cov->is_feedback) {
        return cov;
      }
    }
    printf("Failed to find any feedback coverage.\n");
    return nullptr;
  }
};

extern SimStats stats;

#endif
