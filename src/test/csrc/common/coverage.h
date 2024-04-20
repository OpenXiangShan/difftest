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

#ifndef __COVERAGE_H
#define __COVERAGE_H

#include "common.h"
#include <string>
#include <vector>
#ifdef FIRRTL_COVER
#include "firrtl-cover.h"
#endif // FIRRTL_COVER

#define coverage_display(s, ...) eprintf(ANSI_COLOR_GREEN s ANSI_COLOR_RESET, ##__VA_ARGS__)

class Coverage {
public:
  Coverage() {};
  virtual const char *get_name() = 0;
  virtual const char *get_cover_name(uint32_t i) {
    return "unknown";
  }
  virtual void reset() = 0;
  virtual void update(DiffTestState *state) {}

  // coverage figures
  virtual uint32_t get_total_points() = 0;
  virtual uint32_t get_covered_points() {
    return get_acc_covered_points();
  }
  inline double get_value() {
    return 100.0 * get_covered_points() / get_total_points();
  }

  // accumulative coverage
  virtual void accumulate() = 0;
  virtual bool is_accumulated(uint32_t i) {
    return false;
  }
  virtual uint32_t get_acc_covered_points() = 0;
  inline double get_acc_value() {
    return 100.0 * get_acc_covered_points() / get_total_points();
  }

  virtual void display() {
    display(get_name(), get_total_points(), get_covered_points(), get_acc_covered_points());
  }
  virtual void display_uncovered_points();

  // fuzzer feedback
  bool is_feedback = false;
  virtual void update_is_feedback(const char *cover_name) {
    is_feedback = !cover_name_cmp(cover_name, get_name());
  }
  virtual void to_covered_bytes(uint8_t *bytes) = 0;

protected:
  static int cover_name_cmp(const char *s1, const char *s2) {
    for (int i = 0; s1[i] || s2[i]; i++) {
      char a = (s1[i] >= 'A' && s1[i] <= 'Z') ? s1[i] + ('a' - 'A') : s1[i];
      char b = (s2[i] >= 'A' && s2[i] <= 'Z') ? s2[i] + ('a' - 'A') : s2[i];
      if (a != b) {
        return i + 1;
      }
    }
    return 0;
  }

  inline void display(const char *name, uint32_t total, uint32_t covered, uint32_t acc) {
    coverage_display("COVERAGE: %s, %d, %d, %d\n", name, total, covered, acc);
  }
};

#define BASIC_COVER_FROM_DIFF(cov_name, cov_class_name, diff_class_name, diff_var_name, diff_width_macro) \
  class cov_class_name##Coverage : public Coverage {                                                      \
  public:                                                                                                 \
    const char *get_name() {                                                                              \
      return cov_name;                                                                                    \
    }                                                                                                     \
    cov_class_name##Coverage() {                                                                          \
      reset();                                                                                            \
      memset(acc, 0, sizeof(acc));                                                                        \
    }                                                                                                     \
    void reset() {                                                                                        \
      memset(info, 0, sizeof(info));                                                                      \
    }                                                                                                     \
    void update(DiffTestState *s) {                                                                       \
      memcpy(info, s->diff_var_name, sizeof(s->diff_var_name));                                           \
    }                                                                                                     \
                                                                                                          \
    uint32_t get_total_points() {                                                                         \
      return diff_width_macro;                                                                            \
    }                                                                                                     \
    uint32_t get_covered_points() {                                                                       \
      return sum(info);                                                                                   \
    }                                                                                                     \
                                                                                                          \
    void accumulate() {                                                                                   \
      for (auto i = 0; i < get_total_points(); i++) {                                                     \
        if (info[i].covered) {                                                                            \
          acc[i].covered = 1;                                                                             \
        }                                                                                                 \
      }                                                                                                   \
    }                                                                                                     \
    bool is_accumulated(uint32_t i) {                                                                     \
      return acc[i].covered;                                                                              \
    }                                                                                                     \
    uint32_t get_acc_covered_points() {                                                                   \
      return sum(acc);                                                                                    \
    }                                                                                                     \
                                                                                                          \
    void to_covered_bytes(uint8_t *bytes) {                                                               \
      for (uint32_t i = 0; i < get_total_points(); i++) {                                                 \
        if (info[i].covered) {                                                                            \
          bytes[i] = 1;                                                                                   \
        }                                                                                                 \
      }                                                                                                   \
    }                                                                                                     \
                                                                                                          \
  private:                                                                                                \
    diff_class_name info[diff_width_macro];                                                               \
    diff_class_name acc[diff_width_macro];                                                                \
                                                                                                          \
    uint32_t sum(diff_class_name *cover) {                                                                \
      uint32_t result = 0;                                                                                \
      for (int i = 0; i < get_total_points(); i++) {                                                      \
        if (cover[i].covered) {                                                                           \
          result += 1;                                                                                    \
        }                                                                                                 \
      }                                                                                                   \
      return result;                                                                                      \
    }                                                                                                     \
  };

#ifdef CONFIG_DIFFTEST_INSTRCOVER
BASIC_COVER_FROM_DIFF("Instruction", Instr, DifftestInstrCover, icover, CONFIG_DIFF_ICOVER_WIDTH)
#endif // CONFIG_DIFFTEST_INSTRCOVER

#ifdef CONFIG_DIFFTEST_INSTRIMMCOVER
BASIC_COVER_FROM_DIFF("Instr-Imm", InstrImm, DifftestInstrImmCover, instr_imm_cover, CONFIG_DIFF_INSTR_IMM_COVER_WIDTH)
#endif // CONFIG_DIFFTEST_INSTRIMMCOVER

#ifdef FIRRTL_COVER
class FIRRTLCoverage : public Coverage {
public:
  FIRRTLCoverage();
  ~FIRRTLCoverage();
  const char *get_name() {
    return "FIRRTL";
  };
  void reset();

  // coverage figures
  uint32_t get_total_points();
  uint32_t get_covered_points();
  void accumulate();

  uint32_t get_acc_covered_points();

  void display();
  void display_uncovered_points();

  void update_is_feedback(const char *cover_name);
  void to_covered_bytes(uint8_t *bytes);

private:
  const static int n_cover = sizeof(firrtl_cover) / sizeof(FIRRTLCoverPointParam);
  uint8_t *acc[n_cover];

  const FIRRTLCoverPoint *get();
  uint32_t cover_sum(const FIRRTLCoverPoint *cover);
  uint32_t cover_sum(uint8_t *points, uint32_t total);
  void display(int i);
};
#endif // FIRRTL_COVER

#ifdef LLVM_COVER
class LLVMSanCovData {
public:
  std::vector<bool> points;
  uint32_t reach;
  std::vector<std::string> info;

  LLVMSanCovData() : points{}, reach(0), info{} {};
};
extern LLVMSanCovData *llvm_sancov;

class LLVMSanCoverage : public Coverage {
public:
  LLVMSanCoverage() {};
  inline const char *get_name() {
    return "llvm.branch";
  }
  inline const char *get_cover_name(uint32_t i) {
    return llvm_sancov->info[i].c_str();
  }
  inline bool is_accumulated(uint32_t i) {
    return llvm_sancov->points[i];
  }
  inline void reset() {}

  // coverage figures
  inline uint32_t get_total_points() {
    return llvm_sancov->points.size();
  }
  inline void accumulate() {}

  // accumulative coverage
  inline uint32_t get_acc_covered_points() {
    return llvm_sancov->reach;
  }

  // fuzzer feedback
  inline void to_covered_bytes(uint8_t *bytes) {
    for (auto i = 0; i < llvm_sancov->points.size(); i++) {
      if (llvm_sancov->points[i]) {
        bytes[i] = 1;
      }
    }
  }
};
#endif // LLVM_COVER

class UnionCoverage : public Coverage {
public:
  UnionCoverage(Coverage *_c1, Coverage *_c2);
  const char *get_name() {
    return "union";
  }
  void reset();

  // coverage figures
  uint32_t get_total_points();
  uint32_t get_covered_points();
  void accumulate();

  // accumulative coverage
  uint32_t get_acc_covered_points();

  void display_uncovered_points();

  // fuzzer feedback
  void update_is_feedback(const char *cover_name);
  void to_covered_bytes(uint8_t *bytes);

private:
  Coverage *c1, *c2;
};

#endif // __COVERAGE_H
