#ifndef __PLUGIN_BPALIGN_BPU_H__
#define __PLUGIN_BPALIGN_BPU_H__

#include "common.h"
#include <cstdint>
#include <vector>


struct __attribute__((packed)) Prediction {
    bool taken;
    int CfiPosition;
    uint64_t target;
    BranchAttribute_t attribute;
    BpuPrediction_t toBpuPrediction(uint64_t pc) {
        BpuPrediction_t ret;
        ret.startVAddr = getPrunedAddr(pc);
        ret.takenCfiOffset.valid = taken;
        ret.takenCfiOffset.bits = getFtqOffset(pc, CfiPosition);
        ret.target = getPrunedAddr(target);
        ret.attribute = attribute;
        return ret;
    }
};

struct StageHandshake {
    bool ready;
    Reg<bool> valid;
    bool fire;
    bool flush;
    bool stall;
    StageHandshake() {valid.resetVector = false;}
    int tick(bool reset) {valid.tick(reset); return 0;}
};

struct StageCtrl {
    StageHandshake s[BPSTAGES + 1];

    int tick(bool reset) {
        for (int i = 0; i <= BPSTAGES; i++) {
            s[i].tick(reset);
        }
        return 0;
    }
};

struct BasePredictor {
    bool enable;
    bool resetDone;
    StageCtrl ctrl;
    uint64_t startVAddr;
    virtual int tick(bool reset) = 0;
    virtual int predict(Prediction *pred) = 0;
    virtual ~BasePredictor() = default;
};


#endif