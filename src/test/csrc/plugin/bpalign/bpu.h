#ifndef __PLUGIN_BPALIGN_BPU_H__
#define __PLUGIN_BPALIGN_BPU_H__

#include "BpAlignIO.h"

#include <vector>

#define BPSTAGES 3

struct BasePredictor {
    bool enable;
    bool resetDone;
    PrunedAddr_t s0_pc;
    BpuPrediction_t s[BPSTAGES + 1];
    virtual int tick(const FtqToBpAlign_t& in, BpAlignToFtq_t *out) = 0;
    virtual ~BasePredictor() = default;
};


#endif