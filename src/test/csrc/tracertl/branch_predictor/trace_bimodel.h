#ifndef TRACE_BIMODEL_H
#define TRACE_BIMODEL_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>

#include "debug_macro.h"

#define BIMODEL_USEFUL_BITS 2
#define BIMODEL_TAKEN_THRESHOLD (1 << (BIMODEL_USEFUL_BITS - 1))
#define BIMODLE_FLOOR 0
#define BIMODEL_CEIL ((1 << BIMODEL_USEFUL_BITS) - 1)

#define BIMODEL_SIZE (512)

class TraceBimodel {
private:
    // 模式历史表 (PHT)
    uint8_t pht[BIMODEL_SIZE]; // 2-bit饱和计数器
    // 00: strongly not taken
    // 01: weakly not taken
    // 10: weakly taken
    // 11: strongly taken
    const int index_bits = log2(BIMODEL_CEIL); // 2**11 = 2048
    size_t index_mask; // 用于从PC中提取索引的掩码

    void updateCounter(uint8_t &counter, bool taken, bool &bpu_changed) {
#ifdef BIM_DEBUG
      fprintf(stderr, "  [BIM  Counter] act_tak %d old counter %x", taken, counter);
#endif
      if (taken && counter < BIMODEL_CEIL) {
#ifdef BIM_DEBUG
        fprintf(stderr, "++ -> %d\n", counter + 1);
#endif
        counter++;
        bpu_changed = true;
      }
      else if (!taken && (counter > BIMODLE_FLOOR)) {
#ifdef BIM_DEBUG
        fprintf(stderr, "-- -> %d\n", counter - 1);
#endif
        counter--;
        bpu_changed = true;
      }

#ifdef BIM_DEBUG
      else {
        fprintf(stderr, "\n");
      }
#endif
    }

    size_t get_index(uint64_t pc) {
      return (pc >> 1) & index_mask;
    }

public:
    // 构造函数，初始化PHT
    TraceBimodel() {
      index_mask = BIMODEL_SIZE - 1;
      for (size_t i = 0; i < BIMODEL_SIZE; ++i) {
        pht[i] = BIMODEL_TAKEN_THRESHOLD; // 初始化为 weakly taken
      }
    }

    // 预测分支是否会被采取
    bool predict(uint64_t pc) {
      size_t index = get_index(pc); // 通常使用PC的[2:index_bits+1]作为索引
#ifdef BIM_DEBUG
      fprintf(stderr, "  [BIM  Predict] index %lx taken %d\n", index, pht[index] >= BIMODEL_TAKEN_THRESHOLD);
#endif
      return pht[index] >= BIMODEL_TAKEN_THRESHOLD;
    }

    // 更新预测器状态
    void update(uint64_t pc, bool taken, bool &bpu_changed) {
      size_t index = get_index(pc);

#ifdef BIM_DEBUG
      fprintf(stderr, "  [BIM  Update] index %lx taken %d\n", index, taken);
#endif
      updateCounter(pht[index], taken, bpu_changed);
    }
};

#endif