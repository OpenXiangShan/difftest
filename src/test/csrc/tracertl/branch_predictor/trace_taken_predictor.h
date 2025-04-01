#ifndef TRACE_TAKEN_PREDICTOR_H
#define TRACE_TAKEN_PREDICTOR_H

#include <cstdio>
#include <cstdint>
#include "trace_tage.h"

class TraceTakenPredictor {
private:
  TAGEPredictor tagePredictor;

public:
  void update(uint64_t pc, uint8_t taken, GlobalHistory &ghr, bool &bpu_changed) {
    tagePredictor.update(pc, taken, ghr, bpu_changed);
  }

  bool predict(uint64_t pc, GlobalHistory &ghr) {
    return tagePredictor.predict(pc, ghr);
  }

};

#endif // TRACE_TAKEN_PREDICTOR_H