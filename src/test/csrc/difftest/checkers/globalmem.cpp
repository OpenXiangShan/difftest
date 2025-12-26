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

#include "checkers.h"
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
  return STATE_OK;
}

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
bool SbufferChecker::get_valid(const DifftestSbufferEvent &probe) {
  return probe.valid;
}

void SbufferChecker::clear_valid(DifftestSbufferEvent &probe) {
  probe.valid = 0;
}

int SbufferChecker::check(const DifftestSbufferEvent &probe) {
  update_goldenmem(probe.addr, (void *)probe.data, probe.mask, 64);
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Store", state->track_instr, state->cycle_count);
  }
  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
bool UncacheMmStoreChecker::get_valid(const DifftestUncacheMMStoreEvent &probe) {
  return probe.valid;
}

void UncacheMmStoreChecker::clear_valid(DifftestUncacheMMStoreEvent &probe) {
  probe.valid = 0;
}

int UncacheMmStoreChecker::check(const DifftestUncacheMMStoreEvent &probe) {
  // the flag is set only in the case of multi-cores and uncache mm store
  uint8_t flag = NUM_CORES > 1 ? 1 : 0;
  update_goldenmem(probe.addr, (void *)probe.data, probe.mask, 8, flag);
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Uncache MM Store", state->track_instr, state->cycle_count);
  }
  return STATE_OK;
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
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Atmoic", state->track_instr, state->cycle_count);
  }

  // We need to do atmoic operations here so as to update goldenMem
  if (!(probe.mask == 0xf || probe.mask == 0xf0 || probe.mask == 0xff || probe.mask == 0xffff)) {
    Info("Unrecognized probe.mask: %lx\n", probe.mask);
    return STATE_ERROR;
  }

  if (probe.mask == 0xff) {
    uint64_t rs = probe.data[0]; // rs2
    uint64_t t = probe.out[0];   // original value
    uint64_t cmp = probe.cmp[0]; // rd, probe.data to be compared
    uint64_t ret;
    uint64_t mem;
    read_goldenmem(probe.addr, &mem, 8);

    if (mem != t && probe.fuop != 007 && probe.fuop != 003) { // ignore sc_d & lr_d
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx, t: 0x%lx, op: 0x%x, addr: 0x%lx\n", state->coreid, mem,
           t, probe.fuop, probe.addr);
      return STATE_ERROR;
    }

    switch (probe.fuop) {
      case 002:
      case 003: ret = t; break;
      // if sc fails(aka probe.out == 1), no update to goldenmem
      case 006:
      case 007:
        if (t == 1)
          return STATE_OK;
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
      default: Info("Unknown atomic fuOpType: 0x%x\n", probe.fuop);
    }
    update_goldenmem(probe.addr, &ret, probe.mask, 8);
  } else if (probe.mask == 0xf || probe.mask == 0xf0) {
    uint32_t rs = (uint32_t)probe.data[0]; // rs2
    uint32_t t = (uint32_t)probe.out[0];   // original value
    uint32_t cmp = (uint32_t)probe.cmp[0]; // rd, probe.data to be compared
    uint32_t ret;
    uint32_t mem;
    uint64_t mem_raw;
    uint64_t ret_sel;
    uint64_t aligned_addr = probe.addr & 0xfffffffffffffff8;
    read_goldenmem(aligned_addr, &mem_raw, 8);

    if (probe.mask == 0xf)
      mem = (uint32_t)mem_raw;
    else
      mem = (uint32_t)(mem_raw >> 32);

    if (mem != t && probe.fuop != 006 && probe.fuop != 002) { // ignore sc_w & lr_w
      Info("Core %d atomic instr mismatch goldenMem, rawmem: 0x%lx mem: 0x%x, t: 0x%x, op: 0x%x, addr: 0x%lx\n",
           state->coreid, mem_raw, mem, t, probe.fuop, probe.addr);
      return STATE_ERROR;
    }
    switch (probe.fuop) {
      case 002:
      case 003: ret = t; break;
      // if sc fails(aka probe.out == 1), no update to goldenmem
      case 006:
      case 007:
        if (t == 1)
          return STATE_OK;
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
      default: Info("Unknown atomic fuOpType: 0x%x\n", probe.fuop);
    }
    ret_sel = ret;
    if (probe.mask == 0xf0)
      ret_sel = (ret_sel << 32);
    update_goldenmem(aligned_addr, &ret_sel, probe.mask, 8);
  } else if (probe.mask == 0xffff) {
    uint64_t meml, memh;
    uint64_t retl, reth;
    read_goldenmem(probe.addr, &meml, 8);
    read_goldenmem(probe.addr + 8, &memh, 8);
    if (meml != probe.out[0] || memh != probe.out[1]) {
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx 0x%lx, t: 0x%lx 0x%lx, op: 0x%x, addr: 0x%lx\n",
           state->coreid, memh, meml, probe.out[1], probe.out[0], probe.fuop, probe.addr);
      return STATE_ERROR;
    }
    switch (probe.fuop) {
      case 054: {
        bool success = probe.out[0] == probe.cmp[0] && probe.out[1] == probe.cmp[1];
        retl = success ? probe.data[0] : probe.out[0];
        reth = success ? probe.data[1] : probe.out[1];
        break;
      }
      default: Info("Unknown atomic fuOpType: 0x%x\n", probe.fuop);
    }
    update_goldenmem(probe.addr, &retl, 0xff, 8);
    update_goldenmem(probe.addr + 8, &reth, 0xff, 8);
  } else {
    return STATE_ERROR;
  }

  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_ATOMICEVENT
