#include "bpu.h"
#include "predictors/fallthrough.h"

struct BpAlign : BasePredictor {
    FallthroughPredictor fallthrough;
    int tick(const FtqToBpAlign_t &in, BpAlignToFtq_t *out) override;
};

int BpAlign::tick(const FtqToBpAlign_t &in, BpAlignToFtq_t *out) {
    return 0;
}