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
    pred->taken = false;
    pred->CfiPosition = s1_cfiPosition;
    pred->target = s1_target;
    pred->attribute = BranchAttribute_t();
    return 0;
}

int FallthroughPredictor::tick(bool reset) {
    resetDone = true;
    s1_startVAddr.Din = s0_startVAddr;
    s1_startVAddr.en = ctrl.s[0].fire;
    s1_startVAddr.tick(reset);
    return 0;
}

