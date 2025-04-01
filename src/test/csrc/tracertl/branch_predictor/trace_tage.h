#ifndef __TRACE_TAGE_H__
#define __TRACE_TAGE_H__

#include <iostream>
#include <vector>
#include <bitset>
#include <cstdint>
#include <cmath>
#include <random>
#include <cstdlib>
#include <bitset>

#include "trace_global_history.h"
#include "trace_bimodel.h"
#include "debug_macro.h"

// TAGE only consider Conditional branch ?

// 配置参数
#define TAGE_COUNTER_BITS  (3)        // 预测计数器的位数
#define TAGE_USE_BITS (1)              // 有用性计数器位数

#define TAGE_TAKEN_THRESHOLD (1 << (TAGE_COUNTER_BITS - 1))
#define TAGE_TAKEN_CEIL ((1 << TAGE_COUNTER_BITS) - 1)

#define TAGE_TAG_BITS (8)           // 标记的位数
#define TAGE_TABLES_NUM (4)         //
#define TAGE_TABLE_SIZE (4096) // XS's 4096
const int tage_history_length_vec[TAGE_TABLES_NUM] = {9, 13, 32, 119}; // xs's {9, 13, 32, 119};
const int tage_fh_index_bits_vec[TAGE_TABLES_NUM] = {8, 11, 11, 11}; // xs's {8, 11, 11, 11};
const int tage_fh1_tag_bits_vec[TAGE_TABLES_NUM] = {8, 8, 8, 8}; // xs's {8, 8, 8, 8};
const int tage_fh2_tag_bits_vec[TAGE_TABLES_NUM] = {7, 7, 7, 7}; // xs's {7, 7, 7, 7};

typedef uint16_t tage_tag_t;
typedef uint8_t tage_ctr_t;
typedef uint8_t tage_use_t;

// 单个 TAGE 表项
struct TAGEEntry {
    tage_tag_t tag;
    tage_ctr_t counter : TAGE_COUNTER_BITS;
    tage_use_t useful : TAGE_USE_BITS;
    bool valid;
};

// 单个 TAGE 表
class TAGETable {
protected:
  std::vector<TAGEEntry> entries; // 表项数组
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

  inline bool isTaken(tage_ctr_t ctr) {
    return ctr >= TAGE_TAKEN_THRESHOLD;
  }

public:
  TAGETable(int history_len, int num_entries, int fh_index_bits, int fh1_tag_bits, int fh2_tag_bits)
      : history_len(history_len), entries(num_entries),
      fh_index_bits(fh_index_bits),
      fh1_tag_bits(fh1_tag_bits),
      fh2_tag_bits(fh2_tag_bits) {
    tag_bits = TAGE_TAG_BITS;
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

  tage_tag_t gen_tag(uint64_t pc, GlobalHistory &ghr) {
    tage_tag_t fh1 = ghr.get_folded_history(history_len, fh1_tag_bits);
    tage_tag_t fh2 = ghr.get_folded_history(history_len, fh2_tag_bits);
    tage_tag_t pc_visible = get_pc_visibled(pc, tag_bits);
    tage_tag_t tag = (fh1 ^ fh2) ^ pc_visible;
    return tag;
  }

  // 检查标记是否匹配
  bool match_tag(uint64_t index, tage_tag_t tag) {
    return entries[index].valid && entries[index].tag == tag;
  }

  tage_ctr_t get_prediction(uint64_t index) {
    return entries[index].counter; // 最高位决定方向
  }

  void update_counter(uint64_t index, bool actual_taken, bool &bpu_changed) {
    tage_ctr_t ctr = get_prediction(index);
    if (actual_taken && (ctr < TAGE_TAKEN_CEIL)) {
      entries[index].counter ++;
      bpu_changed = true;
    } else if (!actual_taken && ctr > 0) {
      entries[index].counter --;
      bpu_changed = true;
    }
  }

  void allocate_new_entry(uint64_t index, uint64_t tag, bool taken) {
    entries[index].valid = true;
    entries[index].tag = tag;
    entries[index].useful = 0;
    entries[index].counter = taken ? TAGE_TAKEN_THRESHOLD : (TAGE_TAKEN_THRESHOLD-1);
  }

  // 增加有用性计数器
  inline void increment_u(uint64_t index, bool &bpu_changed) {
    if (entries[index].useful != 1)
      bpu_changed = true;
    entries[index].useful = 1;
  }

  // 减少有用性计数器
  inline void decrement_u(uint64_t index, bool &bpu_changed) {
    if (entries[index].useful != 0)
      bpu_changed = true;
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

typedef uint8_t alt_count_t;
#define AltCounterBits (4)
#define AltCounter_Size (128)
#define AltCounter_InitValue (1 << (AltCounterBits - 1))
#define AltCounter_Ceil ((1 << AltCounterBits) - 1)
class AltCounter {
private:
  alt_count_t alt_counter[AltCounter_Size];

  inline size_t get_index(uint64_t pc) {
    return (pc >> 1) & (AltCounter_Size - 1);
  }

public:
  AltCounter() {
    for (int i = 0; i < AltCounter_Size; i++) {
      alt_counter[i] = AltCounter_InitValue;
    }
  }

  void increase(uint64_t pc, bool &bpu_changed) {
    auto index = get_index(pc);
    if (alt_counter[index] < AltCounter_Ceil) {
      alt_counter[index]++;
      bpu_changed = true;
    }
  }

  void decrease(uint64_t pc, bool &bpu_changed) {
    auto index = get_index(pc);
    if (alt_counter[index] > 0) {
      alt_counter[index]--;
      bpu_changed = true;
    }
  }

  inline bool useAltCounter(uint64_t pc) {
    return alt_counter[get_index(pc)] == AltCounter_Ceil;
  }

};




class TAGEPredictor {

protected:
  std::vector<TAGETable> tables;  // 多级 TAGE 表
  TraceBimodel bim; // 2**11 = 2048

  AltCounter alt_counter;
  BankTickCounter btc = BankTickCounter(7);

  inline bool isWeakPred(tage_ctr_t ctr) {
    return (ctr == TAGE_TAKEN_THRESHOLD) ||
      (ctr == (TAGE_TAKEN_THRESHOLD - 1));
  }

  inline bool isTaken(tage_ctr_t ctr) {
    return ctr >= TAGE_TAKEN_THRESHOLD;
  }

public:
    TAGEPredictor() {
      for (int i = 0; i < TAGE_TABLES_NUM; i++) {
        tables.emplace_back(TAGETable(tage_history_length_vec[i], TAGE_TABLE_SIZE,
          tage_fh_index_bits_vec[i], tage_fh1_tag_bits_vec[i], tage_fh2_tag_bits_vec[i]));
      }
    }

    // 预测分支方向
    bool predict(uint64_t pc, GlobalHistory &ghr) {
#ifdef TAGE_DEBUG
      fprintf(stderr, "  [TAGE Predict] pc %lx\n", pc);
#endif
      for (int i = TAGE_TABLES_NUM - 1; i >= 0; i--) {
        size_t index = tables[i].gen_index(pc, ghr);
        tage_tag_t tag = tables[i].gen_tag(pc, ghr);

        if (tables[i].match_tag(index, tag)) {
          tage_ctr_t ctr = tables[i].get_prediction(index);
#ifdef TAGE_DEBUG
          fprintf(stderr, "  [TAGE Predict] match level %d index 0x%lx tag 0x%x ctr 0x%x taken %d\n", i, index, tag, ctr, isTaken(ctr));
#endif

          if (isWeakPred(ctr) && alt_counter.useAltCounter(pc)) break;
          else return isTaken(ctr);
        }
      }

      // base_preditor
      bool base_result = bim.predict(pc);

#ifdef TAGE_DEBUG
      fprintf(stderr, "  [TAGE Predict] but base taken 0x%d\n", base_result);
#endif
      return base_result;
    }

    // 更新预测器状态
    void update(uint64_t pc, bool actual_taken, GlobalHistory &ghr, bool &bpu_changed) {
#ifdef TAGE_DEBUG
      fprintf(stderr, "  [TAGE Update] pc %lx act_taken %d\n", pc, actual_taken);
      std::cerr << "  [TAGE Update] ghr " << ghr.history << std::endl;
#endif

      // 查找 Provider 和 AltProvider
      int provider_level = -1;
      int hit_level = -1;
      bool table_hit = false;

      bool base_result = bim.predict(pc);
      bool table_result = false;
      tage_ctr_t tabel_result_ctr;

      for (int i = TAGE_TABLES_NUM - 1; i >= 0; i--) {
        size_t index = tables[i].gen_index(pc, ghr);
        tage_tag_t tag = tables[i].gen_tag(pc, ghr);

        if (tables[i].match_tag(index, tag)) {
            tabel_result_ctr = tables[i].get_prediction(index);
            table_hit = true;
            hit_level = i;
            table_result = isTaken(tabel_result_ctr);
#ifdef TAGE_DEBUG
              fprintf(stderr, "  [TAGE Update] table_hit taken %d\n", table_result);
#endif

            if (isWeakPred(tabel_result_ctr) && alt_counter.useAltCounter(pc)) {
#ifdef TAGE_DEBUG
              fprintf(stderr, "  [TAGE Update] provider is base\n");
#endif
              break;
            }
            else {
#ifdef TAGE_DEBUG
              fprintf(stderr, "  [TAGE Update] provider_level %d\n", i);
#endif
              provider_level = i;
              break;
            }
        }
      }

      // T0 as provider
      if (provider_level == -1) {
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Update] update bim\n");
#endif
        bim.update(pc, actual_taken, bpu_changed);
      }

      // if only T0 hit
      if (!table_hit) {
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Update] table not hit\n");
#endif
        if (base_result != actual_taken) {
#ifdef TAGE_DEBUG
          fprintf(stderr, "  [TAGE Update] base not correct, allocate\n");
#endif
          // allocate new entry, table not hit
          bpu_changed = true;
          allocate(pc, ghr, actual_taken, 0);
        } else {} // nop
      } else {
// #ifdef TAGE_DEBUG
//         fprintf(stderr, "  [TAGE Update] table_hit path\n");
// #endif
        // if T0 and Tn both hit
        size_t index = tables[hit_level].gen_index(pc, ghr);
        tables[hit_level].update_counter(index, actual_taken, bpu_changed);

        // if T0 result is equal to Tn result
        if (base_result == table_result) {
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Update] base equal table\n");
#endif
          if (table_result == actual_taken) {}
          else {
#ifdef TAGE_DEBUG
            fprintf(stderr, "  [TAGE Update] table result wrong, call allocate\n");
#endif
            // allocate new entry for table not correct
            bpu_changed = true;
            allocate(pc, ghr, actual_taken, hit_level+1);
          }
        } else {
#ifdef TAGE_DEBUG
          fprintf(stderr, "  [TAGE Update] base not equal table\n");
#endif
          if (table_result == actual_taken) {
#ifdef TAGE_DEBUG
            fprintf(stderr, "  [TAGE Update] table result correct, update u\n");
#endif
            tables[hit_level].increment_u(index, bpu_changed);
            if (isWeakPred(tabel_result_ctr)) {
#ifdef TAGE_DEBUG
              fprintf(stderr, "  [TAGE Update] weak pred, decrease alt_counter\n");
#endif
              alt_counter.decrease(pc, bpu_changed);
            }
          } else {
#ifdef TAGE_DEBUG
            fprintf(stderr, "  [TAGE Update] table result wrong, decrease u and call allocate\n");
#endif
            tables[hit_level].decrement_u(index, bpu_changed);
            // allocate new entry, for table not correct
            bpu_changed = true;
            allocate(pc, ghr, actual_taken, hit_level+1);

            if (isWeakPred(tabel_result_ctr)) {
#ifdef TAGE_DEBUG
              fprintf(stderr, "  [TAGE Update] weak pred, increase alt_counter\n");
#endif
              alt_counter.increase(pc, bpu_changed);
            }
          }
        }
      }
#ifdef TAGE_DEBUG
      fprintf(stderr, "  [TAGE Update] update ghr\n");
#endif
    }

    void allocate(uint64_t pc, GlobalHistory &ghr, bool actual_taken, int from_level) {
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Allocate] pc %lx act_taken %x from_level %d\n",
          pc, actual_taken, from_level);
#endif
      if (from_level >= TAGE_TABLES_NUM) {
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Allocate] from_level largest, return\n");
#endif
        return;
      }

      uint8_t canAllocate = 0, cannotAllocate = 0;
      bool allocateable[TAGE_TABLES_NUM];

      for (int level = from_level; level < TAGE_TABLES_NUM; level++) {
        size_t index = tables[level].gen_index(pc, ghr);
        tage_use_t useful = tables[level].get_u(index);
        canAllocate += useful == 0;
        cannotAllocate += useful != 0;

        allocateable[level] = useful == 0;
      }

#ifdef TAGE_DEBUG
      fprintf(stderr, "  [TAGE Allocate] allocatable ");
      for(int i = from_level; i < TAGE_TABLES_NUM; i++) {
        fprintf(stderr, "%d", allocateable[i]);
      }
      fprintf(stderr, "\n");
#endif

      uint8_t delta = (cannotAllocate > canAllocate) ? (cannotAllocate - canAllocate) : 0;
      if (btc.updateAndIsFull(delta)) {
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Allocate] clear all use\n");
#endif
        for (int i = 0; i < TAGE_TABLES_NUM; i++) {
          tables[i].clear_all_useful();
        }
      }

      // do allocate
      if (canAllocate > 0) {
        int level = (rand() % (TAGE_TABLES_NUM - from_level)) + from_level;
#ifdef TAGE_DEBUG
        fprintf(stderr, "  [TAGE Allocate] level %d\n", level);
#endif
        if (allocateable[level]) {
          size_t index = tables[level].gen_index(pc, ghr);
          tage_tag_t tag = tables[level].gen_tag(pc, ghr);
#ifdef TAGE_DEBUG
          fprintf(stderr, "  [TAGE Allocate] success level %d index %lx tag %x act_taken %d\n", level, index, tag, actual_taken);
#endif
          tables[level].allocate_new_entry(index, tag, actual_taken);
        }
      }
    }
};

#endif // __TRACE_TAGE_H__