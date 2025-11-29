
#include "export.h"

Difftest *GetDifftest(int index) {
  if (index < NUM_CORES)
    return difftest[index];
  return NULL;
}

void InitRam(std::string image, uint64_t n_bytes) {
  init_ram(image.c_str(), n_bytes);
}

void InitFlash(std::string flash_bin) {
  if (flash_bin.empty()) {
    init_flash(0);
    return;
  }
  init_flash(flash_bin.c_str());
}

uint64_t FlashRead(uint32_t addr) {
  uint64_t data;
  flash_read(addr, &data);
  return data;
}

int FlashWrite(uint32_t addr, uint64_t data) {
  if (!flash_dev.base) {
    return -1;
  }
  uint32_t aligned_addr = addr & FLASH_ALIGH_MASK;
  uint64_t rIdx = aligned_addr / sizeof(uint64_t);
  if (rIdx >= flash_dev.size / sizeof(uint64_t)) {
    printf("[warning] read addr %x is out of bound\n", addr);
    return -1;
  } else {
    flash_dev.base[rIdx] = data;
  }
  return 0;
}

flash_device_t *GetFlash() {
  return &flash_dev;
}

int _diff_stat = -1;
bool DifftestStepAndCheck(uint64_t pin, uint64_t val, uint64_t arg) {
  _diff_stat = difftest_nstep(1, true);
  return _diff_stat != STATE_RUNNING;
}

uint64_t GetFuncAddressOfDifftestStepAndCheck() {
  return (uint64_t)DifftestStepAndCheck;
}

int GetDifftestStat() {
  return _diff_stat;
}

void GoldenMemInit() {
  init_goldenmem();
}

void GoldenMemFinish() {
  goldenmem_finish();
}

void SetProxyRefSo(uint64_t addr) {
  difftest_ref_so = (const char *)addr;
}

uint64_t GetProxyRefSo() {
  return (uint64_t)difftest_ref_so;
}

uint64_t Get_PMEM_BASE() {
  return PMEM_BASE;
}

uint64_t Get_FIRST_INST_ADDRESS() {
  return FIRST_INST_ADDRESS;
}

void Set_PMEM_BASE(uint64_t v) {
  PMEM_BASE = v;
}

void Set_FIRST_INST_ADDRESS(uint64_t v) {
  FIRST_INST_ADDRESS = v;
}
