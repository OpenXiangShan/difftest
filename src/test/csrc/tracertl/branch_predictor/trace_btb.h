#ifndef TRACE_BTB_H
#define TRACE_BTB_H

#include <cstdio>
#include <cstdint>
#include <vector>
#include "debug_macro.h"

#define MinInstSize (2)

#define BTB_SIZE (512) // xs 2048
#define BTB_WAYS (4) // xs 4
#define BTB_SETS (BTB_SIZE / BTB_WAYS) //

#define InstShiftBit (1) // log2(MinInstSize)
#define BTB_TAG_BITS (20) // keep same with xiangshan

// BTB takes direct branch(conditional and unconditional)
class TraceBTB {
private:
  const size_t BTB_INDEX_BITS = log2(BTB_SETS); // log2(BTB_SIZE/Ways) = log2(256)

  struct Entry {
      bool valid = false;
      uint64_t tag = 0;
      uint64_t target = 0;
  };

  struct Set {
      Entry entries[BTB_WAYS];
      uint64_t last_access_time[BTB_WAYS];
  };

  Set sets[BTB_SETS];

  // for lru, lastest access time
  // when lookup, update the lru order
  uint64_t timer = 0;

public:
  TraceBTB() {}

  void updateTimer() { timer++; }

  // return true if hit, and set target
  bool predict(uint64_t pc, uint64_t& target) {

    uint64_t set_index = getSetIndex(pc);
    uint64_t tag = getTag(pc);
    Set& set = sets[set_index];

#ifdef BTB_DEBUG
    fprintf(stderr, "  [BTB lookup] idx 0x%lx pc %lx\n", set_index, pc);
#endif

    for (int i = 0; i < BTB_WAYS; ++i) {
      Entry& entry = set.entries[i];

      if (entry.valid && entry.tag == tag) {
        target = entry.target;
#ifdef BTB_DEBUG
        fprintf(stderr, "  [BTB lookup] hit way %x target %lx\n", i, target);
#endif
        updateLRU(set, i); // 更新LRU顺序
        return true;
      }
    }
#ifdef BTB_DEBUG
    fprintf(stderr, "  [BTB lookup] miss\n");
#endif
    return false;
  }

  // 更新或插入新的分支目标
  void update(uint64_t pc, uint64_t target, bool &bpu_changed) {
    uint64_t set_index = getSetIndex(pc);
    uint64_t tag = getTag(pc);
    Set& set = sets[set_index];

#ifdef BTB_DEBUG
    fprintf(stderr, "  [BTB update] idx 0x%lx pc %lx target %lx\n", set_index, pc, target);
#endif

    // 检查是否已存在
    for (int i = 0; i < BTB_WAYS; ++i) {
      Entry& entry = set.entries[i];

      if (entry.valid && entry.tag == tag) {
#ifdef BTB_DEBUG
        fprintf(stderr, "  [BTB update][hit] way %x\n", i);
#endif
        if (entry.target != target) {
          bpu_changed = true;
        }
        entry.target = target;
        updateLRU(set, i);
        return;
      }
    }

    // 寻找可替换条目（优先无效条目）
    int replace_way = 0;
    uint64_t min_time = 0;
    for (int i = 0; i < BTB_WAYS; ++i) {
      if (!set.entries[i].valid) {
        replace_way = i;
        break;
      }
      if (set.last_access_time[i] < min_time) {
        min_time = set.last_access_time[i];
        replace_way = i;
      }
    }

#ifdef BTB_DEBUG
    fprintf(stderr, "  [BTB update][allocate] way %x\n", replace_way);
#endif

    bpu_changed = true;
    Entry& entry = set.entries[replace_way];
    entry.valid = true;
    entry.tag = tag;
    entry.target = target;
    updateLRU(set, replace_way);
  }

private:
  // low log2Up(BTB_SETS), rm the low 1 bits(C-extension)
  uint64_t getSetIndex(uint64_t pc) const {
    return (pc >> (InstShiftBit)) & ((1 << BTB_INDEX_BITS) - 1);
  }

  uint64_t getTag(uint64_t pc) const {
    return (pc >> (InstShiftBit + BTB_INDEX_BITS)) & ((1 << BTB_TAG_BITS) - 1);
  }

  inline void updateLRU(Set& set, int pos) {
    set.last_access_time[pos] = timer;
  }
};

#endif // TRACE_BTB_H