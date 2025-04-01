#ifndef TRACE_TARGET_PREDICTOR_H
#define TRACE_TARGET_PREDICTOR_H

#include <cstdio>
#include <cstdint>

#include "trace_btb.h"

// BTB inside
class TraceTargetPredictor {
private:
  TraceBTB btb;

public:
  void update(uint64_t pc, uint64_t target, bool &bpu_changed) {
    btb.update(pc, target, bpu_changed);
    btb.updateTimer();
  }
  uint64_t predict(uint64_t pc) {
    uint64_t target = 0;
    if (btb.predict(pc, target)) {
      return target;
    }
    return 0;
  }
};

#endif // TRACE_TARGET_PREDICTOR_H