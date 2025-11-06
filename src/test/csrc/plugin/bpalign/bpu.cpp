#include "BpAlignIO.h"
#include "bpu.h"
#include "common.h"
#include "predictors/fallthrough.h"
#include <cstdint>

struct BpAlign{
    RegEnable<uint64_t> s0_pcReg;
    RegEnable<uint64_t> s1_pc;
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
    ctrl.s[1].flush = in.redirect_valid;
    ctrl.s[1].fire = ctrl.s[1].valid.read() && in.prediction_ready;
    ctrl.s[1].ready = ctrl.s[1].fire || !ctrl.s[1].valid.read() || ctrl.s[1].flush;

    fallthrough.predict(&s1_prediction);
    fallthrough.ctrl.s[1].fire = ctrl.s[1].fire;
    // stage 0 
    ctrl.s[0].fire = ctrl.s[1].ready;
    ctrl.s[0].stall = !(ctrl.s[1].valid.read() || in.redirect_valid);
    s0_pc = (in.redirect_valid) ? getFullAddr(in.redirect.target) :
                     (ctrl.s[1].valid.read()) ? s1_prediction.target : s0_pcReg.read();
                     
    fallthrough.s0_startVAddr = s0_pc;
    fallthrough.ctrl.s[0].fire = ctrl.s[0].fire;


    /* output logic */
    out.prediction_valid = ctrl.s[1].valid.read();
    out.prediction = s1_prediction.toBpuPrediction(s1_pc.read());
    return 0;
}

int BpAlign::tick() {
    s0_pcReg.resetVector = getFullAddr(in.resetVector);
    s0_pcReg.Din = s0_pc;
    s0_pcReg.en = !ctrl.s[0].stall;

    ctrl.s[1].valid.Din = ctrl.s[0].fire;
    ctrl.s[1].valid.en = ctrl.s[0].fire || ctrl.s[1].flush || ctrl.s[1].fire;

    s1_pc.Din = s0_pc;
    s1_pc.en = ctrl.s[0].fire;
    
    s1_pc.tick(reset);
    s0_pcReg.tick(reset);
    fallthrough.tick(reset);
    ctrl.tick(reset);
    return 0;
}

extern "C" void ftqtobpalign(FtqToBpAlign_t in) {
    top.in = in;
    // static uint64_t cur = 0;
    // printf("[%ld]", cur);
    // printf("redirect_valid: %d", top.in.redirect_valid);
    // printf("redirect_target: %lx", getFullAddr(top.in.redirect.target));
    // printf(" s0_pcReg: %lx", top.s0_pcReg.read());
    // printf("\n");
    // cur ++;
    top.predict();
}

extern "C" void bpaligntoftq(BpAlignToFtq_t *out) {
    *out = top.out;
    // static uint64_t cur = 0;
    // printf("[%ld]predict.valid: %d", cur, out->prediction_valid);
    // printf(" predict.ready: %d", top.in.prediction_ready);
    // printf(" predict.target: %lx", getFullAddr(out->prediction.target));
    // printf("\n");
    // cur ++;
    top.tick();
}

extern "C" void do_reset(char isReset) {
    top.reset = isReset;
    // if(isReset) printf("reset\n");
}