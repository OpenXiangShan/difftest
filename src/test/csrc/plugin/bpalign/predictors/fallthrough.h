#ifndef __PLUGIN_BPALIGN_PREDICTORS_FALLTHROUGH_H__
#define __PLUGIN_BPALIGN_PREDICTORS_FALLTHROUGH_H__

#include "../bpu.h"
#include <cstdint>

struct FallthroughPredictor : public BasePredictor {
    uint64_t s0_startVAddr;
    RegEnable<uint64_t> s1_startVAddr;
    int tick() override;
    int predict(Prediction *pred) override;
};

inline uint64_t getAlignedAddr(uint64_t addr) {
    return addr & ~((1ULL << FetchBlockAlignWidth) - 1);
}

inline uint64_t getNextAlignedAddr(uint64_t addr) {
    return getAlignedAddr(addr) + (1ULL << FetchBlockAlignWidth);
}

inline int getAlignedInstOffset(uint64_t addr) {
    return BITS(addr, FetchBlockAlignWidth - 1, instOffsetBits);
}

inline int getAlignedPosition(uint64_t addr, int ftqOffset) {
    int fullPosition = ftqOffset + getAlignedInstOffset(addr);
    return BITS(fullPosition, CfiPositionWidth - 1, 0);
}

inline int getFtqOffset(uint64_t addr, int position) {
    return position - getAlignedInstOffset(addr);
}

#endif