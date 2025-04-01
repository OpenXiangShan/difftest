#ifndef __TRACE_ITTAGE_H__
#define __TRACE_ITTAGE_H__

#include <iostream>
#include <vector>
#include <bitset>
#include <cstdint>
#include <cmath>
#include <random>
#include <cstdlib>
#include <bitset>

#include "debug_macro.h"

#include "trace_global_history.h"

#define ITTAGE_TAG_BITS (9)           // 标记的位数
#define ITTAGE_COUNTER_BITS  (3)        // 预测计数器的位数
#define ITTAGE_USE_BITS (1)              // 有用性计数器位数

#define ITTAGE_TAKEN_THRESHOLD (1 << (ITTAGE_COUNTER_BITS - 1))
#define ITTAGE_TAKEN_CEIL ((1 << ITTAGE_COUNTER_BITS) - 1)

#define ITTAGE_TABLES_NUM (5)

typedef uint16_t ittage_tag_t;
typedef uint8_t ittage_ctr_t;
typedef uint8_t ittage_use_t;

#define ITTAGE_TABLES_NUM (5)         //
#define ITTAGE_TAG_BITS (9)           // 标记的位数
// #define TAGE_TABLE_SIZE (4096) // XS's 4096
const int ittage_table_size_vec[ITTAGE_TABLES_NUM] = {256, 256, 512, 512, 512};
const int ittage_history_length_vec[ITTAGE_TABLES_NUM] = {4, 8, 13, 16, 32};
const int ittage_fh_index_bits_vec[ITTAGE_TABLES_NUM] = {4, 8, 9, 9, 9};
const int ittage_fh1_tag_bits_vec[ITTAGE_TABLES_NUM] = {4, 8, 9, 9, 9};
const int ittage_fh2_tag_bits_vec[ITTAGE_TABLES_NUM] = {4, 8, 8, 8, 8};

struct ITTAGEEntry {
  uint64_t target;
  ittage_tag_t tag;
  ittage_ctr_t counter : ITTAGE_COUNTER_BITS;
  ittage_use_t useful : ITTAGE_USE_BITS;
  bool valid;
};

class ITTAGETable {
protected:
  std::vector<ITTAGEEntry> entries; // 表项数组
  size_t history_len;                // 该表的历史长度
  size_t index_bits;                 // 索引位数
  size_t tag_bits;

  // for folded history
  size_t fh_index_bits;
  size_t fh1_tag_bits;
  size_t fh2_tag_bits;

  size_t get_pc_visibled(uint64_t pc, size_t unit_len) {
    return (pc >> 1) & ((1ULL << unit_len) - 1);
  }

  inline bool isTaken(ittage_ctr_t ctr) {
    return ctr >= ITTAGE_TAKEN_THRESHOLD;
  }

public:
  ITTAGETable(int history_len, int num_entries, int fh_index_bits, int fh1_tag_bits, int fh2_tag_bits)
      : history_len(history_len), entries(num_entries),
      fh_index_bits(fh_index_bits),
      fh1_tag_bits(fh1_tag_bits),
      fh2_tag_bits(fh2_tag_bits) {
    tag_bits = ITTAGE_TAG_BITS;
    index_bits = log2(num_entries);

    if (fh_index_bits == 0 || fh1_tag_bits == 0 || fh2_tag_bits == 0) {
      printf("Error fh bits: %d %d %d\n", fh_index_bits, fh1_tag_bits, fh2_tag_bits);
      exit(1);
    }
  }

    size_t gen_index(uint64_t pc, GlobalHistory &ghr) {
    size_t folded_history = ghr.get_folded_history(history_len, fh_index_bits);
    uint64_t pc_visibled = get_pc_visibled(pc, index_bits);
    size_t index = (folded_history ^ pc_visibled) % entries.size();
    return index;
  }

  ittage_tag_t gen_tag(uint64_t pc, GlobalHistory &ghr) {
    ittage_tag_t fh1 = ghr.get_folded_history(history_len, fh1_tag_bits);
    ittage_tag_t fh2 = ghr.get_folded_history(history_len, fh2_tag_bits);
    ittage_tag_t pc_visible = get_pc_visibled(pc, tag_bits);
    ittage_tag_t tag = (fh1 ^ fh2) ^ pc_visible;
    return tag;
  }

  bool match_tag(uint64_t index, ittage_tag_t tag) {
    return entries[index].valid && entries[index].tag == tag;
  }

  ittage_ctr_t get_prediction_ctr(uint64_t index) {
    return entries[index].counter;
  }

  uint64_t get_prediction_target(uint64_t index) {
    return entries[index].target;
  }

  void update_counter(uint64_t index, bool isRight) {
    ittage_ctr_t ctr = get_prediction_ctr(index);
    if (isRight && (ctr < ITTAGE_TAKEN_CEIL)) {
      entries[index].counter ++;
    } else if (!isRight && ctr > 0) {
      entries[index].counter --;
    }
  }

  void update_target(uint64_t index, uint64_t target) {
    entries[index].target = target;
  }

  void allocate_new_entry(uint64_t index, uint64_t tag, uint64_t target) {
    entries[index].valid = true;
    entries[index].tag = tag;
    entries[index].useful = 0;
    entries[index].counter = 0;
    entries[index].target = target;
  }

    // 增加有用性计数器
  inline void increment_u(uint64_t index) {
    entries[index].useful = 1;
  }

  // 减少有用性计数器
  inline void decrement_u(uint64_t index) {
    entries[index].useful = 0;
  }

  uint8_t get_u(uint64_t index) {
    return entries[index].useful;
  }

  void clear_all_useful() {
    for (int i = 0; i < entries.size(); i++) {
      entries[i].useful = 0;
    }
  }
};

class TraceITTAGE {
protected:
  std::vector<ITTAGETable> tables;
  BankTickCounter btc = BankTickCounter(8);

  inline bool isUnConf(ittage_ctr_t ctr) {
    return ctr < (1 << (ITTAGE_COUNTER_BITS - 1));
  }

  inline bool isNotNull(ittage_ctr_t ctr) {
    return ctr != 0;
  }

public:
  TraceITTAGE() {
    for (int i = 0; i < ITTAGE_TABLES_NUM; i++) {
      tables.emplace_back(ITTAGETable(ittage_history_length_vec[i], ittage_table_size_vec[i],
        ittage_fh_index_bits_vec[i], ittage_fh1_tag_bits_vec[i], ittage_fh2_tag_bits_vec[i]));
    }
  }

  bool predict(uint64_t pc, GlobalHistory &ghr, uint64_t &target) {
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Predict] pc %lx\n", pc);
#endif

    bool longest_null = false;
    for (int i = ITTAGE_TABLES_NUM - 1; i >= 0; i--) {
      size_t index = tables[i].gen_index(pc, ghr);
      ittage_tag_t tag = tables[i].gen_tag(pc, ghr);

      if (tables[i].match_tag(index, tag)) {
        ittage_ctr_t ctr = tables[i].get_prediction_ctr(index);
        if (isNotNull(ctr) || longest_null) {
          target = tables[i].get_prediction_target(index);
#ifdef ITTAGE_DEBUG
        fprintf(stderr, "  [ITTAGE Predict] match level %d target %lx index 0x%lx tag 0x%x ctr 0x%x isNotNull %d longNull %d\n", i, target, index, tag, ctr, isNotNull(ctr), longest_null);
#endif
          return true;
        } else {
#ifdef ITTAGE_DEBUG
        fprintf(stderr, "  [ITTAGE Predict] match but null level %d index 0x%lx tag 0x%x ctr 0x%x isNotNull %d set longest_null\n", i, index, tag, ctr, isNotNull(ctr));
#endif
          longest_null = true;
        }
      }
    }
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Predict] miss\n");
#endif
    return false;
  }

  // return true if update base
  bool update(uint64_t pc, GlobalHistory &ghr, uint64_t actual_target, bool is_base_hit, uint64_t base_target) {
#ifdef ITTAGE_DEBUG
      fprintf(stderr, "  [ITTAGE Update] pc %lx act_target %lx\n", pc, actual_target);
      std::cerr << "  [ITTAGE Update] ghr " << ghr.history << std::endl;
#endif
    bool should_update_base = false;

    bool should_allocate = false;
    int allocate_level = -1;

    // 查找 Provider 和 AltProvider
    int longest_level = -1;
    uint64_t longest_result = 0;
    size_t longest_index = -1;
    ittage_ctr_t longest_result_ctr;
    bool longest_null = false;

    int sec_longest_level = -1;
    uint64_t sec_longest_result = 0;
    size_t sec_longest_index = -1;
    ittage_ctr_t sec_longest_result_ctr;

    for (int i = ITTAGE_TABLES_NUM - 1; i >= 0; i--) {
      size_t index = tables[i].gen_index(pc, ghr);
      ittage_tag_t tag = tables[i].gen_tag(pc, ghr);

      if (tables[i].match_tag(index, tag)) {
        ittage_ctr_t ctr = tables[i].get_prediction_ctr(index);
        auto result = tables[i].get_prediction_target(index);
        if (longest_null && longest_level != -1) {
          sec_longest_level = i;
          sec_longest_result = result;
          sec_longest_index = index;
          sec_longest_result_ctr = ctr;
          break;
        } else {
          longest_level = i;
          longest_result = result;
          longest_index = index;
          longest_result_ctr = ctr;

          longest_null = !isNotNull(ctr);
          if (!longest_null) break;
        }
      }
    }

    if (longest_level == -1 && is_base_hit && base_target == actual_target) {
#ifdef ITTAGE_DEBUG
      fprintf(stderr, "  [ITTAGE Update] base hit and right, return true\n");
#endif
      should_update_base = true;
    } else if (longest_level == -1 && is_base_hit && base_target != actual_target) {
#ifdef ITTAGE_DEBUG
      fprintf(stderr, "  [ITTAGE Update] base hit and wrong, return true. Allocate new entry\n");
#endif
      // allocate(pc, ghr, actual_target, 0);
      should_allocate = 0;
      allocate_level = 0;
      should_update_base = true;
    }

    int provider_level = sec_longest_level != -1 ? sec_longest_level : longest_level;
    uint64_t provider_target = sec_longest_level != -1 ? sec_longest_result : longest_result;
    size_t provider_index = sec_longest_level != -1 ? sec_longest_index : longest_index;
    ittage_ctr_t provider_ctr = sec_longest_level != -1 ? sec_longest_result_ctr : longest_result_ctr;

#ifdef ITTAGE_DEBUG
    if (longest_level != -1) {
      fprintf(stderr, "  [ITTAGE Update] longest_level %d longest_result %lx longest_index 0x%lx longest_result_ctr 0x%x\n", longest_level, longest_result, longest_index, longest_result_ctr);
    }
    if (sec_longest_level != -1) {
      fprintf(stderr, "  [ITTAGE Update] sec_longest_level %d sec_longest_result %lx sec_longest_index 0x%lx sec_longest_result_ctr 0x%x\n", sec_longest_level, sec_longest_result, sec_longest_index, sec_longest_result_ctr);
    }
    if (provider_level != -1) {
      fprintf(stderr, "  [ITTAGE Update] provider_level %d provider_target %lx provider_index 0x%lx provider_ctr 0x%x\n", provider_level, provider_target, provider_index, provider_ctr);
    }
#endif

    // longest_level always update
    if (provider_level != longest_level) {
#ifdef ITTAGE_DEBUG
      fprintf(stderr, "  [ITTAGE Update] update longest, index %lx isRight %d ctr %x\n", longest_index, longest_result == actual_target, longest_result_ctr);
#endif
      if (!isNotNull(longest_result_ctr)) {
#ifdef ITTAGE_DEBUG
        fprintf(stderr, "  [ITTAGE Update] update longest target -> %lx\n", actual_target);
#endif
        tables[longest_level].update_target(longest_index, actual_target);
      }
      tables[longest_level].update_counter(longest_index, longest_result == actual_target);
    }

    if (provider_level == longest_level) {
      if (longest_result == actual_target) {
      } else {
        // allocate(pc, ghr, actual_target, provider_level + 1);
#ifdef ITTAGE_DEBUG
        fprintf(stderr, "  [ITTAGE Update] allocate for longest wrong, level %d\n", provider_level + 1);
#endif
        should_allocate = true;
        allocate_level = provider_level + 1;
      }
    }


    // if T0 and Tn both hit
    if (is_base_hit && (provider_level != -1)) {
#ifdef ITTAGE_DEBUG
      fprintf(stderr, "  [ITTAGE Update] update provider, isRight %d ctr %x\n", provider_target == actual_target, provider_ctr);
#endif
      if (!isNotNull(provider_ctr)) {
#ifdef ITTAGE_DEBUG
        fprintf(stderr, "  [ITTAGE Update] update provider target -> %lx\n", actual_target);
#endif
        tables[provider_level].update_target(provider_index, actual_target);
      }
#ifdef ITTAGE_DEBUG
      fprintf(stderr, "  [ITTAGE Update] update provider counter\n");
#endif
      tables[provider_level].update_counter(provider_index, provider_target == actual_target);

      if (base_target == provider_target) {
        if (base_target == actual_target) {
          // nop
        } else {
          // allocate(pc, ghr, actual_target, provider_level + 1);
#ifdef ITTAGE_DEBUG
          fprintf(stderr, "  [ITTAGE Update] allocate for provider wrong, level %d\n", provider_level + 1);
#endif
          should_allocate = true;
          allocate_level = provider_level + 1;
        }
      } else {
#ifdef ITTAGE_DEBUG
        fprintf(stderr, "  [ITTAGE Update] base and provider not equal\n");
#endif
        if (provider_target == actual_target) {
#ifdef ITTAGE_DEBUG
          fprintf(stderr, "  [ITTAGE Update] provider right, inc u\n");
#endif
          tables[provider_level].increment_u(provider_index);
          if (isUnConf(provider_ctr)) {
            should_update_base = true;
          }
        } else {
#ifdef ITTAGE_DEBUG
          fprintf(stderr, "  [ITTAGE Update] provider wrong, dec u\n");
#endif
          tables[provider_level].decrement_u(provider_index);
          // allocate(pc, ghr, actual_target, provider_level + 1);
#ifdef ITTAGE_DEBUG
          fprintf(stderr, "  [ITTAGE Update] allocate for provider wrong, level %d\n", provider_level + 1);
#endif
          should_allocate = true;
          allocate_level = provider_level + 1;
          if (isUnConf(provider_ctr)) {
            should_update_base = true;
          }
        }
      }
    }

    if (should_allocate) {
      allocate(pc, ghr, actual_target, allocate_level);
    }
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Update] update btb %d\n", should_update_base);
#endif

    return should_update_base; // update btb
  }

  void allocate(uint64_t pc, GlobalHistory &ghr, uint64_t actual_target, int from_level) {
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Allocate] pc %lx act_target %lx from_level %d\n", pc, actual_target, from_level);
#endif
    if (from_level >= ITTAGE_TABLES_NUM) {
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Allocate] level to large over\n");
#endif
      return;
    }

    int level = (rand() % (ITTAGE_TABLES_NUM - from_level)) + from_level;
    size_t index = tables[level].gen_index(pc, ghr);
    ittage_use_t u = tables[level].get_u(index);
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Allocate] level %d index %lx old_use %d\n", level, index, u);
#endif
    if (u == 0) {
      ittage_tag_t tag = tables[level].gen_tag(pc, ghr);
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Allocate] success index %lx tag %x act_target %lx\n", index, tag, actual_target);
#endif
      tables[level].allocate_new_entry(index, tag, actual_target);

      btc.decrease(1);
    } else {
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Allocate] conflict index %lx\n", index);
#endif
      if(btc.updateAndIsFull(1)) {
#ifdef ITTAGE_DEBUG
    fprintf(stderr, "  [ITTAGE Allocate] clear all useful\n");
#endif
        for (int i = 0; i < ITTAGE_TABLES_NUM; i++) {
          tables[i].clear_all_useful();
        }
      }
    }
  }
};

#endif