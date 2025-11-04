#include "fallthrough.h"
#include "BpAlignIO.h"
#include <cstdint>


int FallthroughPredictor::predict(Prediction *pred) {
    uint64_t s1_alignedStartAddr = 
        getAlignedAddr(s1_startVAddr.read());
    uint64_t s1_nextBlockAlignedAddr = 
        getAlignedAddr(s1_startVAddr.read() + FetchBlockSize);
    uint64_t s1_nextPageAlignedAddr = 
        getPageAlignedAddr(s1_nextBlockAlignedAddr);
    bool s1_crossPage = 
        isCrossPage(s1_startVAddr.read(), s1_nextBlockAlignedAddr);
    int s1_cfiPosition = (s1_crossPage) ? 
        ((s1_nextPageAlignedAddr - s1_alignedStartAddr) >> instOffsetBits) - 1 :
        FetchBlockInstNum - 1;
    uint64_t s1_target = (s1_crossPage) ?
        s1_nextPageAlignedAddr :
        s1_nextBlockAlignedAddr;
    
    return 0;
}

int FallthroughPredictor::tick() {
    enable = true;
    resetDone = true;
    s1_startVAddr.Din = s0_startVAddr;
    s1_startVAddr.en = stage.s0_fire;
    s1_startVAddr.tick();
    return 0;
}