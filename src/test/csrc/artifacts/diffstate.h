#ifndef __DIFFSTATE_H__
#define __DIFFSTATE_H__

#include <cstdint>

#define CPU_XIANGSHAN
#define CONFIG_DIFFTEST_SQUASH

#define CONFIG_DIFFTEST_REFILLEVENT
#define CONFIG_DIFF_REFILL_WIDTH 3
typedef struct {
  uint8_t  valid;
  uint64_t addr;
  uint64_t data[8];
  uint8_t  idtfr;
} DifftestRefillEvent;

#define CONFIG_DIFFTEST_VECCSRSTATE
typedef struct {
  uint64_t vstart;
  uint64_t vxsat;
  uint64_t vxrm;
  uint64_t vcsr;
  uint64_t vl;
  uint64_t vtype;
  uint64_t vlenb;
} DifftestVecCSRState;

#define CONFIG_DIFFTEST_LRSCEVENT
typedef struct {
  uint8_t  valid;
  uint8_t  success;
} DifftestLrScEvent;

//#define CONFIG_DIFFTEST_STOREEVENT
//#define CONFIG_DIFF_STORE_WIDTH 2
//typedef struct {
//  uint8_t  valid;
//  uint64_t addr;
//  uint64_t data;
//  uint8_t  mask;
//} DifftestStoreEvent;

#define CONFIG_DIFFTEST_ATOMICEVENT
typedef struct {
  uint8_t  valid;
  uint64_t addr;
  uint64_t data;
  uint8_t  mask;
  uint8_t  fuop;
  uint64_t out;
} DifftestAtomicEvent;

#define CONFIG_DIFFTEST_FPWRITEBACK
#define CONFIG_DIFF_WB_FP_WIDTH 128
typedef struct {
  uint64_t data;
} DifftestFpWriteback;

#define CONFIG_DIFFTEST_ARCHVECREGSTATE
typedef struct {
  uint64_t value[64];
} DifftestArchVecRegState;

#define CONFIG_DIFFTEST_DEBUGMODE
typedef struct {
  uint8_t  debugMode;
  uint64_t dcsr;
  uint64_t dpc;
  uint64_t dscratch0;
  uint64_t dscratch1;
} DifftestDebugMode;

#define CONFIG_DIFFTEST_TRAPEVENT
typedef struct {
  uint8_t  hasTrap;
  uint64_t cycleCnt;
  uint64_t instrCnt;
  uint8_t  hasWFI;
  uint8_t  code;
  uint64_t pc;
} DifftestTrapEvent;

#define CONFIG_DIFFTEST_SBUFFEREVENT
#define CONFIG_DIFF_SBUFFER_WIDTH 2
typedef struct {
  uint8_t  valid;
  uint64_t addr;
  uint8_t  data[64];
  uint64_t mask;
} DifftestSbufferEvent;

#define CONFIG_DIFFTEST_INTWRITEBACK
#define CONFIG_DIFF_WB_INT_WIDTH 128
typedef struct {
  uint64_t data;
} DifftestIntWriteback;

#define CONFIG_DIFFTEST_INSTRCOMMIT
#define CONFIG_DIFF_COMMIT_WIDTH 6
typedef struct {
  uint8_t  valid;
  uint8_t  skip;
  uint8_t  isRVC;
  uint8_t  rfwen;
  uint8_t  fpwen;
  uint8_t  vecwen;
  uint8_t  wpdest;
  uint8_t  wdest;
  uint64_t pc;
  uint32_t instr;
  uint16_t robIdx;
  uint8_t  lqIdx;
  uint8_t  sqIdx;
  uint8_t  isLoad;
  uint8_t  isStore;
  uint8_t  nFused;
  uint8_t  special;
} DifftestInstrCommit;

#define CONFIG_DIFFTEST_RUNAHEADCOMMITEVENT
#define CONFIG_DIFF_RUNAHEAD_COMMIT_WIDTH 6
typedef struct {
  uint8_t  valid;
  uint64_t pc;
} DifftestRunaheadCommitEvent;

#define CONFIG_DIFFTEST_ARCHINTREGSTATE
typedef struct {
  uint64_t value[32];
} DifftestArchIntRegState;

#define CONFIG_DIFFTEST_ARCHFPREGSTATE
typedef struct {
  uint64_t value[32];
} DifftestArchFpRegState;

#define CONFIG_DIFFTEST_CSRSTATE
typedef struct {
  uint64_t priviledgeMode;
  uint64_t mstatus;
  uint64_t sstatus;
  uint64_t mepc;
  uint64_t sepc;
  uint64_t mtval;
  uint64_t stval;
  uint64_t mtvec;
  uint64_t stvec;
  uint64_t mcause;
  uint64_t scause;
  uint64_t satp;
  uint64_t mip;
  uint64_t mie;
  uint64_t mscratch;
  uint64_t sscratch;
  uint64_t mideleg;
  uint64_t medeleg;
} DifftestCSRState;

#define CONFIG_DIFFTEST_ARCHEVENT
typedef struct {
  uint8_t  valid;
  uint32_t interrupt;
  uint32_t exception;
  uint64_t exceptionPC;
  uint32_t exceptionInst;
} DifftestArchEvent;

#define CONFIG_DIFFTEST_LOADEVENT
#define CONFIG_DIFF_LOAD_WIDTH 6
typedef struct {
  uint8_t  valid;
  uint64_t paddr;
  uint8_t  opType;
  uint8_t  fuType;
} DifftestLoadEvent;

typedef struct {
  DifftestArchIntRegState        regs_int;
  DifftestCSRState               csr;
  DifftestArchFpRegState         regs_fp;
  DifftestArchVecRegState        regs_vec;
  DifftestVecCSRState            vcsr;
  DifftestArchEvent              event;
  DifftestAtomicEvent            atomic;
  DifftestDebugMode              dmregs;
  DifftestFpWriteback            wb_fp[128];
  DifftestInstrCommit            commit[6];
  DifftestIntWriteback           wb_int[128];
  DifftestLoadEvent              load[6];
  DifftestLrScEvent              lrsc;
  DifftestRefillEvent            refill[3];
  DifftestRunaheadCommitEvent    runahead_commit[6];
  DifftestSbufferEvent           sbuffer[2];
//  DifftestStoreEvent             store[2];
  DifftestTrapEvent              trap;
} DiffTestState;

#endif // __DIFFSTATE_H__
