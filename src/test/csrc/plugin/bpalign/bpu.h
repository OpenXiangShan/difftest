#ifndef __PLUGIN_BPALIGN_BPU__
#define __PLUGIN_BPALIGN_BPU__

#include "BpAlignIO.h"

#define BPSTAGES 3

struct BpuAlign {
    BpAlignToFtq_t out;
    FtqToBpAlign_t in;
    int tick();
};

struct BpuBase {
    bool enable;
    BpuPrediction_t s[BPSTAGES + 1];
    BpAlignToFtq_t tick(const FtqToBpAlign_t& in);
};

#endif