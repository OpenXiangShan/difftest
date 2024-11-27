#ifndef __DSE_H
#define __DSE_H

#include "ram.h"

#define PINGPONG_REG   0x39020000
#define CTRLSEL_REG    0x39020004
#define MAX_INSTR_REG  0x39020008
#define EPOCH_REG      0x39020010
#define MAX_EPOCH_REG  0x39020018
#define ROBSIZE0_REG   0x39020100
#define ROBSIZE1_REG   0x39020108
#define LQSIZE0_REG    0x39020110
#define LQSIZE1_REG    0x39020118
#define SQSIZE0_REG    0x39020120
#define SQSIZE1_REG    0x39020128
#define FTQ0_REG       0x39020130
#define FTQ1_REG       0x39020138
#define IBUFSIZE0_REG  0x39020140
#define IBUFSIZE1_REG  0x39020148

#define MAX_INSTR_CNT  1000000000

enum Register {
    PINGPONG,
    CTRLSEL,
    MAX_INSTR,
    EPOCH,
    MAX_EPOCH,
    ROBSIZE,
    LQSIZE,
    SQSIZE,
    FTQSIZE,
    IBUFSIZE
};

class DSE {
    private:
        char pingpong;
        char ctrlsel;
        uint64_t max_instr;
        uint64_t epoch;
        uint64_t max_epoch;
        uint64_t robsize0;
        uint64_t lqsize0;
        uint64_t sqsize0;
        uint64_t ftqsize0;
        uint64_t ibufsize0;
        uint64_t robsize1;
        uint64_t lqsize1;
        uint64_t sqsize1;
        uint64_t ftqsize1;
        uint64_t ibufsize1;

    public:
        DSE() {
            pingpong = 0;
            ctrlsel = 0;
            max_instr = 0;
            epoch = 0;
            max_epoch = 0;
            robsize0 = 0;
            lqsize0 = 0;
            sqsize0 = 0;
            ftqsize0 = 0;
            ibufsize0 = 0;
            robsize1 = 0;
            lqsize1 = 0;
            sqsize1 = 0;
            ftqsize1 = 0;
            ibufsize1 = 0;
        }

        uint64_t get_register(Register reg);
};

#endif