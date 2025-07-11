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

#include "coverage.h"

void Coverage::display_uncovered_points() {
  printf("Uncovered %s coverage points:\n", get_name());
  for (auto i = 0; i < get_total_points(); i++) {
    if (!is_accumulated(i)) {
      printf("  [%d] %s\n", i, get_cover_name(i));
    }
  }
}

#ifdef FIRRTL_COVER
FIRRTLCoverage::FIRRTLCoverage() {
  for (int i = 0; i < n_cover; i++) {
    acc[i] = new uint8_t[firrtl_cover[i].cover.total];
  }
};

FIRRTLCoverage::~FIRRTLCoverage() {
  for (int i = 0; i < n_cover; i++) {
    delete acc[i];
  }
}

void FIRRTLCoverage::reset() {
  for (auto c: firrtl_cover) {
    memset(c.cover.points, 0, c.cover.total);
  }
}

uint32_t FIRRTLCoverage::get_total_points() {
  return get()->total;
}

uint32_t FIRRTLCoverage::get_covered_points() {
  return cover_sum(get());
}

void FIRRTLCoverage::accumulate() {
  for (int i = 0; i < n_cover; i++) {
    for (auto j = 0; j < firrtl_cover[i].cover.total; j++) {
      if (firrtl_cover[i].cover.points[j]) {
        acc[i][j] = 1;
      }
    }
  }
}

uint32_t FIRRTLCoverage::get_acc_covered_points() {
  auto target = get();
  auto i = (FIRRTLCoverPointParam *)target - firrtl_cover;
  return cover_sum(acc[i], firrtl_cover[i].cover.total);
}

void FIRRTLCoverage::display() {
  for (int i = 0; i < n_cover; i++) {
    display(i);
  }
}

void FIRRTLCoverage::display(int i) {
  uint32_t covered = cover_sum(&(firrtl_cover[i].cover));
  uint32_t acc_value = cover_sum(acc[i], firrtl_cover[i].cover.total);
  Coverage::display(firrtl_cover[i].cover.name, firrtl_cover[i].cover.total, covered, acc_value);
}

void FIRRTLCoverage::display_uncovered_points() {
  for (int i = 0; i < n_cover; i++) {
    printf("Uncovered %s coverage points:\n", firrtl_cover[i].cover.name);
    for (auto j = 0; j < firrtl_cover[i].cover.total; j++) {
      if (!acc[i][j]) {
        printf("  [%d] %s\n", j, firrtl_cover[i].cover.point_names[j]);
      }
    }
  }
}

void FIRRTLCoverage::update_is_feedback(const char *cover_name) {
  // cover_name should be get_name().firrtl_cover_name
  auto name_len = strlen(get_name());
  auto cmp = cover_name_cmp(cover_name, get_name());
  is_feedback = cmp > name_len || !cmp;
  if (is_feedback && cover_name[name_len]) {
    // skip the name and dot (.)
    auto found = false;
    auto subname = cover_name + name_len + 1;
    for (auto &c: firrtl_cover) {
      c.is_feedback = !cover_name_cmp(subname, c.cover.name);
      found = found || c.is_feedback;
    }
    if (!found) {
      printf("Unknown subtype of FIRRTLCoverage: %s\n", cover_name);
      assert(0);
    }
  }
}

void FIRRTLCoverage::to_covered_bytes(uint8_t *bytes) {
  auto target = get();
  memcpy(bytes, target->points, target->total);
}

const FIRRTLCoverPoint *FIRRTLCoverage::get() {
  for (auto &c: firrtl_cover) {
    if (c.is_feedback) {
      return &(c.cover);
    }
  }
  return nullptr;
}

uint32_t FIRRTLCoverage::cover_sum(uint8_t *points, uint32_t total) {
  uint32_t result = 0;
  for (int i = 0; i < total; i++) {
    result += points[i];
  }
  return result;
}

uint32_t FIRRTLCoverage::cover_sum(const FIRRTLCoverPoint *cover) {
  return cover_sum(cover->points, cover->total);
}
#endif // FIRRTL_COVER

#ifdef LLVM_COVER
typedef struct {
  void *pc;
  uint64_t tag;
} llvm_sancov_pc_t;

LLVMSanCovData *llvm_sancov = nullptr;

extern "C" void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
  static uint32_t count = 0;
  if (start == stop || *start)
    return;
  if (!llvm_sancov) {
    llvm_sancov = new LLVMSanCovData();
  }
  auto n_cover = stop - start;
  auto s = llvm_sancov->points.size();
  llvm_sancov->points.resize(llvm_sancov->points.size() + n_cover, false);
  for (uint32_t *x = start; x < stop; x++) {
    *x = ++count;
  }
}

extern "C" void __sanitizer_symbolize_pc(uintptr_t pc, const char *fmt, char *out, size_t out_size);
extern "C" void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
  if (!*guard)
    return;
  auto index = *guard;
  if (llvm_sancov->points.size() >= index) {
    llvm_sancov->points[index - 1] = true;
    llvm_sancov->reach++;
  }
  *guard = 0;
}

extern "C" void __sanitizer_cov_pcs_init(const uintptr_t *pcs_beg, const uintptr_t *pcs_end) {
  char info_str[1024] = "(?) ";
  char *pcDescr = info_str + 4;
  auto p = (const llvm_sancov_pc_t *)pcs_beg;
  auto n = (const llvm_sancov_pc_t *)pcs_end - p;
  for (int i = 0; i < n; i++, p++) {
    info_str[1] = p->tag ? 'Y' : 'N';
    auto pc = (uintptr_t)p->pc + (p->tag ? 1 : 0);
    __sanitizer_symbolize_pc(pc, "%p %F %L", pcDescr, sizeof(info_str) - 4);
    std::string str(info_str);
    llvm_sancov->info.push_back(str);
  }
}
#endif // LLVM_COVER

UnionCoverage::UnionCoverage(Coverage *_c1, Coverage *_c2) : c1(_c1), c2(_c2) {}

void UnionCoverage::reset() {
  c1->reset();
  c2->reset();
}

uint32_t UnionCoverage::get_total_points() {
  return c1->get_total_points() + c2->get_total_points();
}

uint32_t UnionCoverage::get_covered_points() {
  return c1->get_covered_points() + c2->get_covered_points();
}

void UnionCoverage::accumulate() {
  c1->accumulate();
  c2->accumulate();
}

uint32_t UnionCoverage::get_acc_covered_points() {
  return c1->get_acc_covered_points() + c2->get_acc_covered_points();
}

void UnionCoverage::display_uncovered_points() {
  c1->display_uncovered_points();
  c2->display_uncovered_points();
}

// cover_name should be get_name():c1->get_name()+c1->get_name()
void UnionCoverage::update_is_feedback(const char *cover_name) {
  auto name_len = strlen(get_name()) + strlen(c1->get_name()) + strlen(c2->get_name()) + 2;
  char *correct_name = new char[name_len + 1];
  snprintf(correct_name, name_len + 1, "%s:%s+%s", get_name(), c1->get_name(), c2->get_name());
  is_feedback = !cover_name_cmp(cover_name, correct_name);
  delete[] correct_name;
}

void UnionCoverage::to_covered_bytes(uint8_t *bytes) {
  c1->to_covered_bytes(bytes);
  c2->to_covered_bytes(bytes + c1->get_total_points());
}
