#ifndef __PLUGIN_BPALIGN_PREDICTORS_FALLTHROUGH_H__
#define __PLUGIN_BPALIGN_PREDICTORS_FALLTHROUGH_H__

#include "../bpu.h"
#include <cstdint>

struct FallthroughPredictor : public BasePredictor {
    Reg<uint64_t> s1_startVAddr;
    int tick(bool reset) override;
    int predict(Prediction *pred) override;
};

#endif