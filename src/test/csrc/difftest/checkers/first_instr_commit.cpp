/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "checker.h"
#include "flash.h"
#include "ram.h"

bool FirstInstrCommitChecker::get_valid(const DifftestInstrCommit &probe) {
  return !state->has_commit && probe.valid;
}

void FirstInstrCommitChecker::clear_valid(DifftestInstrCommit &probe) {
  state->has_commit = true;
}

int FirstInstrCommitChecker::check(const DifftestInstrCommit &probe) {
  Info("The first instruction of core %d has commited. Difftest enabled. \n", state->coreid);
  proxy->flash_init((const uint8_t *)flash_dev.base, flash_dev.img_size, flash_dev.img_path);
  simMemory->clone_on_demand(
      [this](uint64_t offset, void *src, size_t n) {
        uint64_t dest_addr = PMEM_BASE + offset;
        proxy->mem_init(dest_addr, src, n, DUT_TO_REF);
      },
      true);
  proxy->regcpy(&get_regs(), FIRST_INST_ADDRESS);
  // Do not reconfig simulator 'proxy->update_config(&nemu_config)' here:
  // If this is main sim thread, simulator has its own initial config
  // If this process is checkpoint wakeuped, simulator's config has already been updated,
  // do not override it.
  return 0;
}
