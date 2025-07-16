#include <stdint.h>

void SimJTAG(int __0, uint8_t _0, uint8_t &_1, uint8_t &_2, uint8_t &_3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7,
             uint32_t &_8) {
  _1 = 1; // TRSTn
  _2 = 0; // TMS
  _3 = 0; // TDI
  _8 = 0;
}

void imsic_csr_top(uint8_t _0, uint8_t _1, uint16_t _2, uint8_t _3, uint8_t _4, uint16_t _5, uint8_t _6, uint8_t _7,
                   uint8_t _8, uint8_t _9, uint8_t _10, uint8_t _11, uint64_t _12, uint8_t &_13, uint64_t &_14,
                   uint8_t &_15, uint8_t &_16, uint32_t &_17, uint32_t &_18, uint32_t &_19) {
  _13 = 0;
  _15 = 0;
  _16 = 0; // o_irq
  _17 = 0;
  _18 = 0;
  _19 = 0;
}

void PrintCommitIDModule(uint8_t _0, uint64_t _1, uint8_t _2) {}
