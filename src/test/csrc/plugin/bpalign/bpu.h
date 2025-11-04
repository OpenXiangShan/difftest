#ifndef __PLUGIN_BPALIGN_BPU_H__
#define __PLUGIN_BPALIGN_BPU_H__

#include "BpAlignIO.h"
#include "common.h"
#include <vector>

#define FetchBlockSizeWidth (log2Ceil(FetchBlockSize))
#define FetchBlockAlignSize (FetchBlockSize / 2)
#define FetchBlockAlignWidth (log2Ceil(FetchBlockAlignSize))
#define FetchBlockAlignInstNum (FetchBlockAlignSize / instBytes)
#define instOffsetBits (log2Ceil(instBytes))
#define CfiPositionWidth (log2Ceil(FetchBlockInstNum))

struct Prediction {
    BpuPrediction_t pred[BPSTAGES + 1];
};

struct StageCtrl {
    bool s0_fire;
    bool s1_fire;
    bool s2_fire;
    bool s3_fire;
    bool s4_fire;
};

struct BasePredictor {
    bool enable;
    bool resetDone;
    StageCtrl stage;
    virtual int tick() = 0;
    virtual int predict(Prediction *pred) = 0;
    virtual ~BasePredictor() = default;
};


#endif