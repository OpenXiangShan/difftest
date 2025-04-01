#ifndef TRACE_BRANCH_PREDICTOR_H
#define TRACE_BRANCH_PREDICTOR_H

#include <cstdio>
#include <cstdint>

#include "trace_taken_predictor.h"
#include "trace_target_predictor.h"
#include "trace_ras.h"
#include "trace_ittage.h"

#include "trace_branch_type_fixer.h"
#include "debug_macro.h"

class TraceBranchPredictor {
  TraceTakenPredictor takenPredictor;

  TraceBTB btb;
  TraceRAS ras;
  TraceITTAGE ittage;

  GlobalHistory ghr;

public:

  uint64_t predictTarget(uint64_t pc, uint8_t branch_type, bool is_nonret_jr) {
#ifdef BPU_TOP_DEBUG
    fprintf(stderr, "  [BPU PredTarget] pc %lx type %d is_nonret_jr %d\n", pc, branch_type, is_nonret_jr);
#endif
    uint64_t target;
    if (branch_type == BRANCH_Return) {
      bool hit = ras.pop(target);
      // return target;
      if (hit) {
        return target;
      }
    // } else if (is_nonret_jr) {
// #ifdef BPU_TOP_DEBUG
    // fprintf(stderr, "  [BPU PredTarget] call ittage\n", pc, branch_type);
// #endif
      // if (ittage.predict(pc, ghr, target)) {
        // return target;
      // }
    }
    btb.predict(pc, target);
    return target;
  };

  bool predictTaken(uint64_t pc, uint8_t branch_type) {
#ifdef BPU_TOP_DEBUG
    fprintf(stderr, "  [BPU PredTaken] pc %lx type %d\n", pc, branch_type);
#endif

    if (branch_type == BRANCH_Cond) {
#ifdef BPU_TOP_DEBUG
      fprintf(stderr, "  [BPU PredTaken] call predictor\n");
#endif

      return takenPredictor.predict(pc, ghr);
    } else {
#ifdef BPU_TOP_DEBUG
      fprintf(stderr, "  [BPU PredTaken] direct ret true\n");
#endif
      return true;
    }
  };

  void update(uint64_t pc, uint64_t target, uint8_t taken, uint8_t branch_type, uint64_t seq_pc, bool is_nonret_jr, bool &bpu_changed) {
    if (branch_type == BRANCH_Call) {
      ras.push(seq_pc);
    } else if (branch_type == BRANCH_Cond) {
      takenPredictor.update(pc, taken, ghr, bpu_changed);
    }

    // btb as t0 of ittage
    // uint64_t btb_target;
    // bool is_btb_hit = btb.predict(pc, btb_target);
    bool btb_as_provider = true;

    // if (is_nonret_jr) {
    //   btb_as_provider = ittage.update(pc, ghr, target, is_btb_hit, btb_target);
    // }
    if (btb_as_provider) {
      btb.update(pc, target, bpu_changed);
    }

    ghr.update(taken);
  };
};

#endif // TRACE_BRANCH_PREDICTOR_H