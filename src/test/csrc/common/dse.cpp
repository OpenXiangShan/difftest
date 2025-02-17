// #include "dse.h"


// uint64_t DSE::get_register(Register reg) {
//     uint64_t data = pmem_read(PINGPONG_REG);
//     switch (reg) {
//         case PINGPONG:
//             pingpong = data & 0xff;
//             return pingpong;
//         case CTRLSEL:
//             ctrlsel = (data >> 8) & 0xff;
//             return ctrlsel;
//         case MAX_INSTR:
//             max_instr = pmem_read(MAX_INSTR_REG);
//             return max_instr;
//         case EPOCH:
//             epoch = pmem_read(EPOCH_REG);
//             return epoch;
//         case MAX_EPOCH:
//             max_epoch = pmem_read(MAX_EPOCH_REG);
//             return max_epoch;
//         case ROBSIZE:
//             if (get_register(CTRLSEL) == 0) {
//                 robsize0 = pmem_read(ROBSIZE0_REG);
//                 return robsize0;
//             } else {
//                 robsize1 = pmem_read(ROBSIZE1_REG);
//                 return robsize1;
//             }
//         case LQSIZE:
//             if (get_register(CTRLSEL) == 0) {
//                 lqsize0 = pmem_read(LQSIZE0_REG);
//                 return lqsize0;
//             } else {
//                 lqsize1 = pmem_read(LQSIZE1_REG);
//                 return lqsize1;
//             }
//         case SQSIZE:
//             if (get_register(CTRLSEL) == 0) {
//                 sqsize0 = pmem_read(SQSIZE0_REG);
//                 return sqsize0;
//             } else {
//                 sqsize1 = pmem_read(SQSIZE1_REG);
//                 return sqsize1;
//             }
//         case FTQSIZE:
//             if (get_register(CTRLSEL) == 0) {
//                 ftqsize0 = pmem_read(FTQ0_REG);
//                 return ftqsize0;
//             } else {
//                 ftqsize1 = pmem_read(FTQ1_REG);
//                 return ftqsize1;
//             }
//         case IBUFSIZE:
//             if (get_register(CTRLSEL) == 0) {
//                 ibufsize0 = pmem_read(IBUFSIZE0_REG);
//                 return ibufsize0;
//             } else {
//                 ibufsize1 = pmem_read(IBUFSIZE1_REG);
//                 return ibufsize1;
//             }
//         default:
//             return 0; // 或者抛出异常
//     }
// }