#include "BpAlignIO.h"
#include "bpu.h"
#include "common.h"
#include "predictors/fallthrough.h"
#include <cstdint>

struct BpAlign{
    Reg<uint64_t> s0_pcReg;
    Reg<uint64_t> s1_pc;
    bool reset;
    uint64_t s0_pc;
    StageCtrl ctrl;
    FtqToBpAlign_t in;
    BpAlignToFtq_t out;
    FallthroughPredictor fallthrough;
    BpAlign();
    int predict();
    int tick();
};

static BpAlign top;

BpAlign::BpAlign() {
    s0_pcReg.resetVector = 0;
}

int BpAlign::predict() {
    Prediction s1_prediction;
    // stage 3 

    // stage 2

    // stage 1 
    ctrl.s[1].flush = in.redirect.valid;
    ctrl.s[1].fire = ctrl.s[1].valid.read() && in.prediction_ready;
    ctrl.s[1].ready = ctrl.s[1].fire || !ctrl.s[1].valid.read() || ctrl.s[1].flush;

    fallthrough.predict(&s1_prediction);
    fallthrough.ctrl.s[1].fire = ctrl.s[1].fire;
    // stage 0 
    ctrl.s[0].fire = ctrl.s[1].ready;
    ctrl.s[0].stall = !(ctrl.s[1].valid.read() || in.redirect.valid);
    s0_pc = (in.redirect.valid) ? getFullAddr(in.redirect.bits.target) :
                     (ctrl.s[1].valid.read()) ? s1_prediction.target : s0_pcReg.read();
                     


    /* output logic */
    out.prediction.valid = ctrl.s[1].valid.read();
    out.prediction.bits = s1_prediction.toBpuPrediction(s1_pc.read());
    return 0;
}

int BpAlign::tick() {
    fallthrough.startVAddr = s0_pc;
    fallthrough.ctrl.s[0].fire = ctrl.s[0].fire;
    fallthrough.tick(reset);

    s1_pc.Din = s0_pc;
    s1_pc.en = ctrl.s[0].fire;
    s1_pc.tick(reset);
    
    s0_pcReg.resetVector = getFullAddr(in.resetVector);
    s0_pcReg.Din = s0_pc;
    s0_pcReg.en = !ctrl.s[0].stall;
    s0_pcReg.tick(reset);

    ctrl.s[1].valid.Din = ctrl.s[0].fire;
    ctrl.s[1].valid.en = ctrl.s[0].fire || ctrl.s[1].flush || ctrl.s[1].fire;
    ctrl.tick(reset);
    return 0;
}

extern "C" void ftqtobpalign(FtqToBpAlign_t in) {
    top.in = in;
    top.predict();
}

extern "C" void bpaligntoftq(BpAlignToFtq_t *out) {
    *out = top.out;
    top.tick();
}

extern "C" void do_reset(char isReset) {
    top.reset = isReset;
}