#ifndef __SWIG_DIFFTEST_EXPORT__
#define __SWIG_DIFFTEST_EXPORT__

#include "difftest.h"
#include "flash.h"
#include "goldenmem.h"
#include "ram.h"
#include <string>

extern Difftest **difftest;

Difftest *GetDifftest(int index);

void InitRam(std::string image, uint64_t n_bytes);
void InitFlash(std::string flash_bin);

flash_device_t *GetFlash();
uint64_t FlashRead(uint32_t addr);
int FlashWrite(uint32_t addr, uint64_t data);

bool DifftestStepAndCheck(uint64_t pin, uint64_t val, uint64_t arg);
uint64_t GetFuncAddressOfDifftestStepAndCheck();
int GetDifftestStat();

void GoldenMemInit();
void GoldenMemFinish();
void SetProxyRefSo(uint64_t addr);
uint64_t GetProxyRefSo();

uint64_t Get_PMEM_BASE();
uint64_t Get_FIRST_INST_ADDRESS();
void Set_PMEM_BASE(uint64_t v);
void Set_FIRST_INST_ADDRESS(uint64_t v);

#endif
