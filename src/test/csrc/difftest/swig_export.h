#ifndef __SWIG_DIFFTEST_EXPORT__
#define __SWIG_DIFFTEST_EXPORT__

#include "difftest.h"
#include "ram.h"
#include "flash.h"
#include <string>

extern Difftest **difftest;

Difftest * GetDifftest(int index);

void InitRam(std::string image, uint64_t n_bytes);
void InitFlash(std::string flash_bin);

void     InitPikerUart(uint64_t p1, uint64_t p2);
uint64_t GetPickerUartArgPtr();
uint64_t GetPickerUartFucPtr();

#endif
