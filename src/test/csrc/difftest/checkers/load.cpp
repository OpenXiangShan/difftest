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

#ifdef CONFIG_DIFFTEST_LOADEVENT
bool LoadChecker::get_valid(const DifftestLoadEvent &probe) {
  return probe.valid;
}
void LoadChecker::clear_valid(DifftestLoadEvent &probe) {
  probe.valid = 0;
}

int LoadChecker::check(const DifftestLoadEvent &probe) {
#ifdef CONFIG_DIFFTEST_SQUASH
  load_event_queue.push(probe);
#else
  bool regWen = ((dut->commit[i].rfwen && dut->commit[i].wdest != 0) || dut->commit[i].fpwen) && !dut->commit[i].vecwen;
  auto refRegPtr = proxy->arch_reg(dut->commit[i].wdest, dut->commit[i].fpwen);
  auto commitData = get_commit_data(i);
  do_load_check(load_event, regWen, refRegPtr, commitData);
#endif // CONFIG_DIFFTEST_SQUASH
}

bool LoadSquashChecker::get_valid() {
  return !load_event_queue.empty() && load_event_queue.front().stamp == state->commit_stamp;
}

void LoadSquashChecker::clear_valid() {
  load_event_queue.pop();
}

int LoadSquashChecker::check() {
  bool regWen = load_event.regWen;
  auto refRegPtr = proxy->arch_reg(load_event.wdest, load_event.fpwen);
  auto commitData = load_event.commitData;
  do_load_check(load_event, regWen, refRegPtr, commitData);
}

static int do_load_check(const DifftestLoadEvent &probe, bool regWen, uint64_t *refRegPtr, uint64_t commitData) {
  if (probe.isVLoad) {
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
    do_vec_load_check(i, probe);
#else
    Info("isVLoad should never be set if vector is not enabled\n");
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
    return;
  }

  if (probe.isLoad || probe.isAtomic) {
    proxy->sync();
    if (regWen && *refRegPtr != commitData) {
      uint64_t golden;
      uint64_t golden_flag;
      uint64_t mask = 0xFFFFFFFFFFFFFFFF;
      int len = 0;
      if (probe.isLoad) {
        switch (probe.opType) {
          case 0:  // lb
          case 4:  // lbu
          case 16: // hlvb
          case 20: // hlvbu
            len = 1;
            break;

          case 1:  // lh
          case 5:  // lhu
          case 17: // hlvh
          case 21: // hlvhu
          case 29: // hlvxhu
            len = 2;
            break;

          case 2:  // lw
          case 6:  // lwu
          case 18: // hlvw
          case 22: // hlvwu
          case 30: // hlvxwu
            len = 4;
            break;

          case 3:  // ld
          case 19: // hlvd
            len = 8;
            break;

          default: Info("Unknown fuOpType: 0x%x\n", probe.opType);
        }
      } else if (probe.isAtomic) {
        if (probe.opType % 2 == 0) {
          len = 4;
        } else { // probe.opType % 2 == 1
          len = 8;
        }
      }
      read_goldenmem(probe.paddr, &golden, len, &golden_flag);
      if (probe.isLoad) {
        switch (len) {
          case 1:
            golden = (int64_t)(int8_t)golden;
            golden_flag = (int64_t)(int8_t)golden_flag;
            mask = (uint64_t)(0xFF);
            break;
          case 2:
            golden = (int64_t)(int16_t)golden;
            golden_flag = (int64_t)(int16_t)golden_flag;
            mask = (uint64_t)(0xFFFF);
            break;
          case 4:
            golden = (int64_t)(int32_t)golden;
            golden_flag = (int64_t)(int32_t)golden_flag;
            mask = (uint64_t)(0xFFFFFFFF);
            break;
        }
      }
      if (golden == commitData || probe.isAtomic) { //  atomic instr carefully handled
        proxy->ref_memcpy(probe.paddr, &golden, len, DUT_TO_REF);
        if (regWen) {
          *refRegPtr = commitData;
          proxy->sync(true);
        }
      } else if (probe.isLoad && golden_flag != 0) {
        // goldenmem check failed, but the flag is set, so use DUT data to reset
        Info("load check of uncache mm store flag\n");
        Info("  DUT data: 0x%lx, regWen: %d, refRegPtr: 0x%lx\n", commitData, regWen, refRegPtr);
        proxy->ref_memcpy(probe.paddr, &commitData, len, DUT_TO_REF);
        update_goldenmem(probe.paddr, &commitData, mask, len);
        if (regWen) {
          *refRegPtr = commitData;
          proxy->sync(true);
        }
      } else {
#ifdef DEBUG_SMP
        // goldenmem check failed as well, raise error
        Info("---  SMP difftest mismatch!\n");
        Info("---  Trying to probe local data of another core\n");
        uint64_t buf;
        difftest[(NUM_CORES - 1) - this->id]->proxy->memcpy(probe.paddr, &buf, len, DIFFTEST_TO_DUT);
        Info("---    content: %lx\n", buf);
#else
        proxy->ref_memcpy(probe.paddr, &golden, len, DUT_TO_REF);
        if (regWen) {
          *refRegPtr = commitData;
          proxy->sync(true);
        }
#endif
      }
    }
  }
}

#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
static int do_vec_load_check(const DifftestLoadEvent &probe, int index) {
  if (!enable_vec_load_goldenmem_check) {
    return 0;
  }

  // ===============================================================
  //                      Comparison data
  // ===============================================================
  uint32_t vdNum = proxy->get_ref_vdNum();

  proxy->sync();

#ifdef CONFIG_DIFFTEST_SQUASH
  auto vecFirstLdest = load_event.wdest;
#else
  auto vecFirstLdest = dut->commit[index].wdest;
#endif // CONFIG_DIFFTEST_SQUASH

  bool reg_mismatch = false;

  for (int vdidx = 0; vdidx < vdNum; vdidx++) {
    auto vecNextLdest = vecFirstLdest + vdidx;
    for (int i = 0; i < VLENE_64; i++) {
      int dataIndex = VLENE_64 * vdidx + i;
#ifdef CONFIG_DIFFTEST_VECCOMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
      uint64_t dutRegData = load_event.vecCommitData[dataIndex];
#else
      uint64_t dutRegData = dut->vec_commit_data[index].data[dataIndex];
#endif // CONFIG_DIFFTEST_SQUASH
#else
      // TODO: support Squash without vec_commit_data (i.e. update ArchReg in software)
      auto vecNextPdest = dut->commit[index].otherwpdest[dataIndex];
      uint64_t dutRegData = dut->pregs_vrf.value[vecPdest];
#endif // CONFIG_DIFFTEST_VECCOMMITDATA

      uint64_t *refRegPtr = proxy->arch_vecreg(VLENE_64 * vecNextLdest + i);
      reg_mismatch |= dutRegData != *refRegPtr;
    }
  }

  // ===============================================================
  //                      Regs Mismatch handle
  // ===============================================================
  bool goldenmem_mismatch = false;

  if (reg_mismatch) {
    // ===============================================================
    //                      Check golden memory
    // ===============================================================
    uint64_t *vec_goldenmem_regPtr = (uint64_t *)proxy->get_vec_goldenmem_reg();

    if (vec_goldenmem_regPtr == nullptr) {
      Info("Vector Load comparison failed and no consistency check with golden mem was performed.\n");
      return;
    }

    for (int vdidx = 0; vdidx < vdNum; vdidx++) {
#ifndef CONFIG_DIFFTEST_COMMITDATA
      bool v0Wen = dut->commit[index].v0wen && vdidx == 0;
      auto vecNextPdest = dut->commit[index].otherwpdest[vdidx];
      uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vrf[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

      for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
        uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
        uint64_t dutRegData = dut->vec_commit_data[index].data[VLENE_64 * vdidx + i];
#endif // CONFIG_DIFFTEST_SQUASH
#else
        uint64_t dutRegData = dutRegPtr[i];
#endif // CONFIG_DIFFTEST_COMMITDATA

        goldenmem_mismatch |= dutRegData != vec_goldenmem_regPtr[VLENE_64 * vdidx + i];
      }
    }

    if (!goldenmem_mismatch) {
      // ===============================================================
      //                      sync memory and regs
      // ===============================================================
      proxy->vec_update_goldenmem();

      for (int vdidx = 0; vdidx < vdNum; vdidx++) {
#ifndef CONFIG_DIFFTEST_COMMITDATA
        bool v0Wen = dut->commit[index].v0wen && vdidx == 0;
        auto vecNextPdest = dut->commit[index].otherwpdest[vdidx];
        uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vrf[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

        auto vecNextLdest = vecFirstLdest + vdidx;

        for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
          uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
          uint64_t dutRegData = dut->vec_commit_data[index].data[VLENE_64 * vdidx + i];
#endif // CONFIG_DIFFTEST_SQUASH
#else
          uint64_t dutRegData = dutRegPtr[i];
#endif // CONFIG_DIFFTEST_COMMITDATA

          uint64_t *refRegPtr = proxy->arch_vecreg(VLENE_64 * vecNextLdest + i);
          *refRegPtr = dutRegData;
        }
      }

      proxy->sync(true);
    } else {
      Info("Vector Load register and golden memory mismatch\n");
    }
  }
  return 0;
}
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
}
;
#endif // CONFIG_DIFFTEST_LOADEVENT
