#ifndef __UPARAM_H
#define __UPARAM_H

#include "common.h"
#include <vector>
#include "o3cpu_design_space.h"

struct uparam_t {
    int robsize;
    int lqsize;
    int sqsize;
    int ftqsize;
    int ibufsize;
    int intdqsize;
    int fpdqsize;
    int lsdqsize;
    int l2mshrs;
    int l3mshrs;
    int l2sets;
    int l3sets;
    int intphyregs;
    int fpphyregs;
    int rasssize;
    int dcacheways;
    int dcachemshrs;
};

extern uint64_t max_epoch;

extern uparam_t uparam;

#define ROBSIZE_ADDR        0x0000
#define LQSIZE_ADDR         0x0008
#define SQSIZE_ADDR         0x0010
#define FTQSIZE_ADDR        0x0018
#define IBUFSIZE_ADDR       0x0020
#define INTDQSIZE_ADDR      0x0028
#define FPDQSIZE_ADDR       0x0030
#define LSDQSIZE_ADDR       0x0038
#define L2MSHRS_ADDR        0x0040
#define L3MSHRS_ADDR        0x0048
#define L2SETS_ADDR         0x0050
#define L3SETS_ADDR         0x0058
#define INTPHYREGS_ADDR     0x0060
#define FPPHYREGS_ADDR      0x0068
#define RASSIZE_ADDR        0x0070
#define DCACHEWAYS_ADDR     0x0078
#define DCACHEMSHRS_ADDR    0x0080

#define MAX_EPOCH_ADDR      0x1000

void init_uparam(std::vector<int> embedding, int epoch);
void embedding_to_uparam(std::vector<int> embedding);
void set_uparam(uint64_t addr, uint64_t data);
std::vector<int> uparam_to_embedding();


#endif // __UPARAM_H