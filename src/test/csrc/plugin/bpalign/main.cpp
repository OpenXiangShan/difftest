#include "BpAlignIO.h"
#include "bpu.h"
#include "predictors/fallthrough.h"

struct BpAlign : BasePredictor {
    FtqToBpAlign_t in;
    BpAlignToFtq_t out;
    FallthroughPredictor fallthrough;
    int tick() override;
};

int BpAlign::tick() {
    return 0;
}