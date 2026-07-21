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

#ifdef CONFIG_DIFFTEST_MATRIXSTOREEVENT

bool MatrixStoreChecker::get_valid(const DifftestMatrixStoreEvent &probe) {
  return probe.valid;
}

void MatrixStoreChecker::clear_valid(DifftestMatrixStoreEvent &probe) {
  probe.valid = 0;
}

int MatrixStoreChecker::check(const DifftestMatrixStoreEvent &probe) {
  update_goldenmem(probe.addr, (void *)probe.data, probe.mask, 64);
  if (probe.addr == state->track_instr) {
    dumpGoldenMem("Matrix Store", state->track_instr, state->cycle_count);
  }
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_MATRIXSTOREEVENT

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

  // AMOCAS.Q is the only operation wider than a doubleword.
  if (probe.mask == 0xffff) {
    uint64_t meml, memh;
    uint64_t retl, reth;
    read_goldenmem(probe.addr, &meml, 8);
    read_goldenmem(probe.addr + 8, &memh, 8);
    if (meml != probe.out[0] || memh != probe.out[1]) {
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx 0x%lx, t: 0x%lx 0x%lx, op: 0x%x, addr: 0x%lx\n",
           state->coreid, memh, meml, probe.out[1], probe.out[0], probe.fuop, probe.addr);
      return STATE_ERROR;
    }
    const unsigned op = (probe.fuop >> 3) & 0xf;
    if (op != 11) {
      Info("Unknown 128-bit atomic fuOpType: 0x%x\n", probe.fuop);
      return STATE_ERROR;
    }
    bool success = probe.out[0] == probe.cmp[0] && probe.out[1] == probe.cmp[1];
    retl = success ? probe.data[0] : probe.out[0];
    reth = success ? probe.data[1] : probe.out[1];
    update_goldenmem(probe.addr, &retl, 0xff, 8);
    update_goldenmem(probe.addr + 8, &reth, 0xff, 8);
  } else {
    const uint64_t byte_mask = probe.mask;
    if (byte_mask == 0 || (byte_mask & ~0xffULL) != 0) {
      Info("Unrecognized atomic mask: 0x%lx\n", probe.mask);
      return STATE_ERROR;
    }

    const unsigned byte_offset = __builtin_ctzll(byte_mask);
    const unsigned bytes = __builtin_popcountll(byte_mask);
    const uint64_t contiguous_mask = ((1ULL << bytes) - 1) << byte_offset;
    if ((bytes != 1 && bytes != 2 && bytes != 4 && bytes != 8) || byte_mask != contiguous_mask) {
      Info("Non-contiguous atomic mask: 0x%lx\n", probe.mask);
      return STATE_ERROR;
    }

    const unsigned bits = bytes * 8;
    const unsigned bit_shift = byte_offset * 8;
    const uint64_t value_mask = bits == 64 ? ~0ULL : (1ULL << bits) - 1;
    const uint64_t aligned_addr = probe.addr & ~0x7ULL;
    uint64_t raw_mem;
    read_goldenmem(aligned_addr, &raw_mem, 8);

    const uint64_t mem = (raw_mem >> bit_shift) & value_mask;
    const uint64_t rs = probe.data[0] & value_mask;
    const uint64_t t = probe.out[0] & value_mask;
    const uint64_t cmp = probe.cmp[0] & value_mask;
    const unsigned op = (probe.fuop >> 3) & 0xf;

    // LR/SC return reservation status rather than an old-memory value.
    if (mem != t && op != 0 && op != 1) {
      Info("Core %d atomic instr mismatch goldenMem, rawmem: 0x%lx mem: 0x%lx, t: 0x%lx, op: 0x%x, addr: 0x%lx\n",
           state->coreid, raw_mem, mem, t, probe.fuop, probe.addr);
      return STATE_ERROR;
    }

    const auto as_signed = [bits](uint64_t value) -> int64_t {
      if (bits == 64)
        return static_cast<int64_t>(value);
      const uint64_t sign = 1ULL << (bits - 1);
      return static_cast<int64_t>((value ^ sign) - sign);
    };

    uint64_t ret;
    switch (op) {
      case 0: ret = t; break; // LR
      case 1:                 // SC
        if (t == 1)
          return STATE_OK;
        ret = rs;
        break;
      case 2: ret = rs; break;                                    // SWAP
      case 3: ret = t + rs; break;                                // ADD
      case 4: ret = t ^ rs; break;                                // XOR
      case 5: ret = t & rs; break;                                // AND
      case 6: ret = t | rs; break;                                // OR
      case 7: ret = as_signed(t) < as_signed(rs) ? t : rs; break; // MIN
      case 8: ret = as_signed(t) > as_signed(rs) ? t : rs; break; // MAX
      case 9: ret = t < rs ? t : rs; break;                       // MINU
      case 10: ret = t > rs ? t : rs; break;                      // MAXU
      case 11: ret = t == cmp ? rs : t; break;                    // CAS
      default: Info("Unknown atomic fuOpType: 0x%x\n", probe.fuop); return STATE_ERROR;
    }

    uint64_t write_data = (ret & value_mask) << bit_shift;
    update_goldenmem(aligned_addr, &write_data, byte_mask, 8);
  }

  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_ATOMICEVENT
