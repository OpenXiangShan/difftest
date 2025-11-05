#include "BpAlignIO.h"
#include "bpu.h"
#include "predictors/fallthrough.h"
#include <cstdint>

struct BpAlign{
    RegEnable<uint64_t> s0_pcReg;
    uint64_t s0_pc;
    FtqToBpAlign_t in;
    BpAlignToFtq_t out;
    FallthroughPredictor fallthrough;
    int tick();
};

static BpAlign top;

int BpAlign::tick() {
    Prediction pred{};
    fallthrough.predict(&pred);

    fallthrough.tick();
    return 0;
}

extern "C" void ftqtobpalign_read(FtqToBpAlign_t in) {
    top.in = in;
}

extern "C" void tick() {
    top.tick();
}

extern "C" void bpaligntoftq_write(BpAlignToFtq_t *out) {
    *out = top.out;
}