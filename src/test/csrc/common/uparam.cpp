#include "uparam.h"

uparam_t uparam;
uint64_t max_epoch;

void init_uparam(std::vector<int> embedding, int epoch) {
    printf("init_uparam\n");
    embedding_to_uparam(embedding);
    max_epoch = epoch;
}

void set_uparam(uint64_t addr, uint64_t data) {
    printf("set_uparam: addr = %lx, data = %lx\n", addr, data);
    switch (addr) {
        case ROBSIZE_ADDR:
            uparam.robsize = data;
            break;
        case LQSIZE_ADDR:
            uparam.lqsize = data;
            break;
        case SQSIZE_ADDR:
            uparam.sqsize = data;
            break;
        case FTQSIZE_ADDR:
            uparam.ftqsize = data;
            break;
        case IBUFSIZE_ADDR:
            uparam.ibufsize = data;
            break;
        case INTDQSIZE_ADDR:
            uparam.intdqsize = data;
            break;
        case FPDQSIZE_ADDR:
            uparam.fpdqsize = data;
            break;
        case LSDQSIZE_ADDR:
            uparam.lsdqsize = data;
            break;
        case L2MSHRS_ADDR:
            uparam.l2mshrs = data;
            break;
        case L3MSHRS_ADDR:
            uparam.l3mshrs = data;
            break;
        case L2SETS_ADDR:
            uparam.l2sets = data;
            break;
        case L3SETS_ADDR:
            uparam.l3sets = data;
            break;
        case MAX_EPOCH_ADDR:
            max_epoch = data;
            break;
        case INTPHYREGS_ADDR:
            uparam.intphyregs = data;
            break;
        case FPPHYREGS_ADDR:
            uparam.fpphyregs = data;
            break;
        case RASSIZE_ADDR:
            uparam.rasssize = data;
            break;
        default:
            break;
    }
}

void embedding_to_uparam(std::vector<int> embedding) {
    printf("embedding_to_uparam\n");
    uparam.ftqsize = embedding[EMDIdx::FTQ];
    uparam.ibufsize = embedding[EMDIdx::IBUF]; 
    uparam.intdqsize = embedding[EMDIdx::INTDQ];
    uparam.fpdqsize = embedding[EMDIdx::FPDQ];
    uparam.lsdqsize = embedding[EMDIdx::LSDQ];
    uparam.lqsize = embedding[EMDIdx::LQ];
    uparam.sqsize = embedding[EMDIdx::SQ];
    uparam.robsize = embedding[EMDIdx::ROB];
    uparam.l2mshrs = embedding[EMDIdx::L2MSHRS];
    uparam.l2sets = embedding[EMDIdx::L2SETS];
    uparam.l3mshrs = embedding[EMDIdx::L3MSHRS];
    uparam.l3sets = embedding[EMDIdx::L3SETS];
    uparam.intphyregs = embedding[EMDIdx::INTPHYREGS];
    uparam.fpphyregs = embedding[EMDIdx::FPPHYREGS];
    uparam.rasssize = embedding[EMDIdx::RASSIZE];
}

std::vector<int> uparam_to_embedding() {
    printf("uparam_to_embedding\n");
    std::vector<int> embedding(EMDIdx::EMD_SIZE, 0);  
    embedding[EMDIdx::FTQ] = uparam.ftqsize;
    embedding[EMDIdx::IBUF] = uparam.ibufsize;
    embedding[EMDIdx::INTDQ] = uparam.intdqsize;
    embedding[EMDIdx::FPDQ] = uparam.fpdqsize;
    embedding[EMDIdx::LSDQ] = uparam.lsdqsize;
    embedding[EMDIdx::LQ] = uparam.lqsize;
    embedding[EMDIdx::SQ] = uparam.sqsize;
    embedding[EMDIdx::ROB] = uparam.robsize;
    embedding[EMDIdx::L2MSHRS] = uparam.l2mshrs;
    embedding[EMDIdx::L2SETS] = uparam.l2sets;
    embedding[EMDIdx::L3MSHRS] = uparam.l3mshrs;
    embedding[EMDIdx::L3SETS] = uparam.l3sets;
    embedding[EMDIdx::INTPHYREGS] = uparam.intphyregs;
    embedding[EMDIdx::FPPHYREGS] = uparam.fpphyregs;
    embedding[EMDIdx::RASSIZE] = uparam.rasssize;
    return embedding;
}


extern "C" void uparam_read(uint64_t addr, uint64_t *data) {
    switch (addr) {
        case ROBSIZE_ADDR:
            *data = uparam.robsize;
            break;
        case LQSIZE_ADDR:
            *data = uparam.lqsize;
            break;
        case SQSIZE_ADDR:
            *data = uparam.sqsize;
            break;
        case FTQSIZE_ADDR:
            *data = uparam.ftqsize;
            break;
        case IBUFSIZE_ADDR:
            *data = uparam.ibufsize;
            break;
        case INTDQSIZE_ADDR:
            *data = uparam.intdqsize;
            break;
        case FPDQSIZE_ADDR:
            *data = uparam.fpdqsize;
            break;
        case LSDQSIZE_ADDR:
            *data = uparam.lsdqsize;
            break;
        case L2MSHRS_ADDR:
            *data = uparam.l2mshrs;
            break;
        case L3MSHRS_ADDR:
            *data = uparam.l3mshrs;
            break;
        case L2SETS_ADDR:
            *data = uparam.l2sets;
            break;
        case L3SETS_ADDR:
            *data = uparam.l3sets;
            break;
        case MAX_EPOCH_ADDR:
            *data = max_epoch;
            break;
        case INTPHYREGS_ADDR:
            *data = uparam.intphyregs;
            break;
        case FPPHYREGS_ADDR:
            *data = uparam.fpphyregs;
            break;
        case RASSIZE_ADDR:
            *data = uparam.rasssize;
            break;
        default:
            assert("uparam_read: invalid address");
            break;
    }

    // log
    switch (addr) {
        case ROBSIZE_ADDR:
            printf("uparam_read: ROBSIZE = %d\n", *data);
            break;
        case LQSIZE_ADDR:
            printf("uparam_read: LQSIZE = %d\n", *data);
            break;
        case SQSIZE_ADDR:
            printf("uparam_read: SQSIZE = %d\n", *data);
            break;
        case FTQSIZE_ADDR:
            printf("uparam_read: FTQSIZE = %d\n", *data);
            break;
        case IBUFSIZE_ADDR:
            printf("uparam_read: IBUFSIZE = %d\n", *data);
            break;
        case INTDQSIZE_ADDR:
            printf("uparam_read: INTDQSIZE = %d\n", *data);
            break;
        case FPDQSIZE_ADDR:
            printf("uparam_read: FPDQSIZE = %d\n", *data);
            break;
        case LSDQSIZE_ADDR:
            printf("uparam_read: LSDQSIZE = %d\n", *data);
            break;
        case L2MSHRS_ADDR:
            printf("uparam_read: L2MSHRS = %d\n", *data);
            break;
        case L3MSHRS_ADDR:
            printf("uparam_read: L3MSHRS = %d\n", *data);
            break;
        case L2SETS_ADDR:
            printf("uparam_read: L2SETS = %d\n", *data);
            break;
        case L3SETS_ADDR:
            printf("uparam_read: L3SETS = %d\n", *data);
            break;
        case MAX_EPOCH_ADDR:
            printf("uparam_read: MAX_EPOCH = %d\n", *data);
            break;
        case INTPHYREGS_ADDR:
            printf("uparam_read: INTPHYREGS = %d\n", *data);
            break;
        case FPPHYREGS_ADDR:
            printf("uparam_read: FPPHYREGS = %d\n", *data);
            break;
        case RASSIZE_ADDR:
            printf("uparam_read: RASSSIZE = %d\n", *data);
            break;
        default:
            break;
    }
}