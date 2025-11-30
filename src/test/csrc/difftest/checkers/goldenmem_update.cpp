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
#include "goldenmem.h"

void dumpGoldenMem(const char *banner, uint64_t addr, uint64_t time) {
#ifdef DEBUG_REFILL
  char buf[512];
  if (addr == 0) {
    return;
  }
  Info("============== %s =============== time = %ld\ndata: ", banner, time);
  for (int i = 0; i < 8; i++) {
    read_goldenmem(addr + i * 8, &buf, 8);
    Info("%016lx", *((uint64_t *)buf));
  }
  Info("\n");
#endif
}

bool GoldenMemoryInit::get_valid(const DifftestTrapEvent &probe) {
  return initDump && probe.cycleCnt >= 100;
}
void GoldenMemoryInit::clear_valid(DifftestTrapEvent &probe) {
  initDump = false;
}

int GoldenMemoryInit::check(const DifftestTrapEvent &probe) {
  dumpGoldenMem("Init", state->track_instr, probe.cycleCnt);
  return 0;
}

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
bool SbufferChecker::get_valid(const DifftestSbufferEvent &probe) {
  return probe.valid;
}

void SbufferChecker::clear_valid(DifftestSbufferEvent &probe) {
  probe.valid = 0;
}

int SbufferChecker::check(const DifftestSbufferEvent &probe) {
  update_goldenmem(probe.addr, probe.data, probe.mask, 64);
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Store", state->track_instr, cycleCnt);
  }
  return 0;
}

#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
bool UncacheMmStoreChecker::get_valid(const DifftestUncacheMmStoreEvent &probe) {
  return probe.valid;
}

void UncacheMmStoreChecker::clear_valid(DifftestUncacheMmStoreEvent &probe) {
  probe.valid = 0;
}

int UncacheMmStoreChecker::check(const DifftestUncacheMmStoreEvent &probe) {
  probe.valid = 0;
  // the flag is set only in the case of multi-cores and uncache mm store
  uint8_t flag = NUM_CORES > 1 ? 1 : 0;
  update_goldenmem(probe.addr, probe.data, probe.mask, 8, flag);
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Uncache MM Store", state->track_instr, cycleCnt);
  }
  return 0;
}
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
bool AtomicChecker::get_valid(const DifftestAtomicEvent &probe) {
  return probe.valid;
}

void AtomicChecker::clear_valid(DifftestAtomicEvent &probe) {
  probe.valid = 0;
}

int AtomicChecker::check(const DifftestAtomicEvent &probe) {
  int ret = handle_atomic(state->coreid, probe.addr, (uint64_t *)probe.data, probe.mask, (uint64_t *)probe.cmp,
                          probe.fuop, (uint64_t *)probe.out);
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Atmoic", state->track_instr, cycleCnt);
  }
  if (ret)
    return ret;
}

inline int handle_atomic(int coreid, uint64_t atomicAddr, uint64_t atomicData[], uint64_t atomicMask,
                         uint64_t atomicCmp[], uint8_t atomicFuop, uint64_t atomicOut[]) {
  // We need to do atmoic operations here so as to update goldenMem
  if (!(atomicMask == 0xf || atomicMask == 0xf0 || atomicMask == 0xff || atomicMask == 0xffff)) {
    Info("Unrecognized mask: %lx\n", atomicMask);
    return 1;
  }

  if (atomicMask == 0xff) {
    uint64_t rs = atomicData[0]; // rs2
    uint64_t t = atomicOut[0];   // original value
    uint64_t cmp = atomicCmp[0]; // rd, data to be compared
    uint64_t ret;
    uint64_t mem;
    read_goldenmem(atomicAddr, &mem, 8);
    if (mem != t && atomicFuop != 007 && atomicFuop != 003) { // ignore sc_d & lr_d
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx, t: 0x%lx, op: 0x%x, addr: 0x%lx\n", coreid, mem, t,
           atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 002:
      case 003: ret = t; break;
      // if sc fails(aka atomicOut == 1), no update to goldenmem
      case 006:
      case 007:
        if (t == 1)
          return 0;
        ret = rs;
        break;
      case 012:
      case 013: ret = rs; break;
      case 016:
      case 017: ret = t + rs; break;
      case 022:
      case 023: ret = (t ^ rs); break;
      case 026:
      case 027: ret = t & rs; break;
      case 032:
      case 033: ret = t | rs; break;
      case 036:
      case 037: ret = ((int64_t)t < (int64_t)rs) ? t : rs; break;
      case 042:
      case 043: ret = ((int64_t)t > (int64_t)rs) ? t : rs; break;
      case 046:
      case 047: ret = (t < rs) ? t : rs; break;
      case 052:
      case 053: ret = (t > rs) ? t : rs; break;
      case 054:
      case 056:
      case 057: ret = (t == cmp) ? rs : t; break;
      default: Info("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    update_goldenmem(atomicAddr, &ret, atomicMask, 8);
  }

  if (atomicMask == 0xf || atomicMask == 0xf0) {
    uint32_t rs = (uint32_t)atomicData[0]; // rs2
    uint32_t t = (uint32_t)atomicOut[0];   // original value
    uint32_t cmp = (uint32_t)atomicCmp[0]; // rd, data to be compared
    uint32_t ret;
    uint32_t mem;
    uint64_t mem_raw;
    uint64_t ret_sel;
    atomicAddr = (atomicAddr & 0xfffffffffffffff8);
    read_goldenmem(atomicAddr, &mem_raw, 8);

    if (atomicMask == 0xf)
      mem = (uint32_t)mem_raw;
    else
      mem = (uint32_t)(mem_raw >> 32);

    if (mem != t && atomicFuop != 006 && atomicFuop != 002) { // ignore sc_w & lr_w
      Info("Core %d atomic instr mismatch goldenMem, rawmem: 0x%lx mem: 0x%x, t: 0x%x, op: 0x%x, addr: 0x%lx\n", coreid,
           mem_raw, mem, t, atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 002:
      case 003: ret = t; break;
      // if sc fails(aka atomicOut == 1), no update to goldenmem
      case 006:
      case 007:
        if (t == 1)
          return 0;
        ret = rs;
        break;
      case 012:
      case 013: ret = rs; break;
      case 016:
      case 017: ret = t + rs; break;
      case 022:
      case 023: ret = (t ^ rs); break;
      case 026:
      case 027: ret = t & rs; break;
      case 032:
      case 033: ret = t | rs; break;
      case 036:
      case 037: ret = ((int32_t)t < (int32_t)rs) ? t : rs; break;
      case 042:
      case 043: ret = ((int32_t)t > (int32_t)rs) ? t : rs; break;
      case 046:
      case 047: ret = (t < rs) ? t : rs; break;
      case 052:
      case 053: ret = (t > rs) ? t : rs; break;
      case 054:
      case 056:
      case 057: ret = (t == cmp) ? rs : t; break;
      default: Info("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    ret_sel = ret;
    if (atomicMask == 0xf0)
      ret_sel = (ret_sel << 32);
    update_goldenmem(atomicAddr, &ret_sel, atomicMask, 8);
  }

  if (atomicMask == 0xffff) {
    uint64_t meml, memh;
    uint64_t retl, reth;
    read_goldenmem(atomicAddr, &meml, 8);
    read_goldenmem(atomicAddr + 8, &memh, 8);
    if (meml != atomicOut[0] || memh != atomicOut[1]) {
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx 0x%lx, t: 0x%lx 0x%lx, op: 0x%x, addr: 0x%lx\n", coreid,
           memh, meml, atomicOut[1], atomicOut[0], atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 054: {
        bool success = atomicOut[0] == atomicCmp[0] && atomicOut[1] == atomicCmp[1];
        retl = success ? atomicData[0] : atomicOut[0];
        reth = success ? atomicData[1] : atomicOut[1];
        break;
      }
      default: Info("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    update_goldenmem(atomicAddr, &retl, 0xff, 8);
    update_goldenmem(atomicAddr + 8, &reth, 0xff, 8);
  }
  return 0;
}
#endif // CONFIG_DIFFTEST_ATOMICEVENT
