#ifndef __PLUGIN_BPALIGN_PREDICTORS_FALLTHROUGH_H__
#define __PLUGIN_BPALIGN_PREDICTORS_FALLTHROUGH_H__

#include "../bpu.h"
#include "../utils/reg.h"

struct FallthroughPredictor : public BasePredictor {
    RegEnable<uint64_t> s1_startVAddr;
    int tick(const FtqToBpAlign_t& in, BpAlignToFtq_t *out) override;
};

#endif