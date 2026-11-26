/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#include "difftest.h"
#include "matrix_hash.h"

#if defined(CONFIG_DIFFTEST_AMUCTRLEVENT) || defined(CONFIG_DIFFTEST_MSYNCEVENT)
static void set_error_pc(int coreid, uint64_t pc) {
  difftest[coreid]->get_trap_event()->pc = pc;
}
#endif

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

#include <algorithm>
#include <cassert>

// Temporary legacy defaults for designs that do not generate AMU geometry.
#ifndef CONFIG_DIFF_AMU_ARLEN
#define CONFIG_DIFF_AMU_ARLEN 4096
#endif

#ifndef CONFIG_DIFF_AMU_TRLEN
#define CONFIG_DIFF_AMU_TRLEN 512
#endif

#ifndef CONFIG_DIFF_AMU_BANK_WIDTH
#define CONFIG_DIFF_AMU_BANK_WIDTH 256
#endif

#ifndef CONFIG_DIFF_AMU_WRITEBACK_BANKS
#define CONFIG_DIFF_AMU_WRITEBACK_BANKS 8
#endif

#ifndef CONFIG_DIFF_AMU_WRITEBACK_WORDS_PER_BANK
#define CONFIG_DIFF_AMU_WRITEBACK_WORDS_PER_BANK 4
#endif

#ifndef CONFIG_DIFF_AMU_AB_WORDS_PER_BANK
#define CONFIG_DIFF_AMU_AB_WORDS_PER_BANK 4
#endif

#ifndef CONFIG_DIFF_AMU_C_WORDS_PER_BANK
#define CONFIG_DIFF_AMU_C_WORDS_PER_BANK 4
#endif

#ifndef CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES
#define CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES 8192
#endif

#ifndef CONFIG_DIFF_AMU_C_REG_SIZE_BYTES
#define CONFIG_DIFF_AMU_C_REG_SIZE_BYTES 65536
#endif

static inline const char *amu_ctrl_op_name(uint8_t op) {
  switch (op) {
    case 0: return "MMA";
    case 1: return "MLS";
    case 2: return "MRELEASE";
    case 3: return "MARITH";
    default: return "UNKNOWN";
  }
}

static void print_amu_ctrl_event(const char *source, const DifftestAmuCtrlEvent &event) {
  printf("  %s AMU ctrl: pc 0x%016lx, op %s\n", source, event.pc, amu_ctrl_op_name(event.op));
  switch (event.op) {
    case 0: // MMA
      printf(
          "                md %d, rm %d, sat %d, ms1 %d, ms2 %d, fp %d,\n"
          "                mtilem %d, mtilen %d, mtilek %d, types1 %d, types2 %d, typed %d\n",
          event.md, event.rm, event.sat, event.ms1, event.ms2, event.isfp, event.mtilem, event.mtilen, event.mtilek,
          event.types1, event.types2, event.typed);
      break;
    case 1: // MLS
      printf(
          "                md %d, ls %d, transpose %d, isacc %d,\n"
          "                base %016lx, stride %016lx, row %d, column %d, widths %d\n",
          event.md, event.sat, event.isfp, event.types1, event.base, event.stride, event.mtilem, event.mtilen,
          event.typed);
      break;
    case 2: // MRelease
      printf("                msyncRd %d\n", event.mtilem);
      break;
    case 3: // Arith
      printf("                md %d, opType %#lx\n", event.md, event.base);
      break;
    default: printf("                Unknown amu event op: %d\n", event.op); break;
  }
}

// Difftest events can be packed by the batch gateway. Convert to the natural
// layout expected by NEMU before crossing the reference-model ABI boundary.
static struct AmuCtrlEvent to_nemu_amu_ctrl(const DifftestAmuCtrlEvent &source) {
  struct AmuCtrlEvent target = {};
  target.valid = source.valid;
  target.op = source.op;
  target.rm = source.rm;
  target.md = source.md;
  target.sat = source.sat;
  target.ms1 = source.ms1;
  target.ms2 = source.ms2;
  target.mtilem = source.mtilem;
  target.mtilen = source.mtilen;
  target.mtilek = source.mtilek;
  target.types1 = source.types1;
  target.types2 = source.types2;
  target.typed = source.typed;
  target.isfp = source.isfp;
  target.base = source.base;
  target.stride = source.stride;
  target.pc = source.pc;
  target.computeType = source.computeType;
  target.isScale = source.isScale;
  target.isConvertF32 = source.isConvertF32;
  target.isDynamicMxfpQuant = source.isDynamicMxfpQuant;
  return target;
}

static void from_nemu_amu_ctrl(DifftestAmuCtrlEvent &target, const struct AmuCtrlEvent &source) {
  target.valid = source.valid;
  target.op = source.op;
  target.rm = source.rm;
  target.md = source.md;
  target.sat = source.sat;
  target.ms1 = source.ms1;
  target.ms2 = source.ms2;
  target.mtilem = source.mtilem;
  target.mtilen = source.mtilen;
  target.mtilek = source.mtilek;
  target.types1 = source.types1;
  target.types2 = source.types2;
  target.typed = source.typed;
  target.isfp = source.isfp;
  target.base = source.base;
  target.stride = source.stride;
  target.pc = source.pc;
  target.computeType = source.computeType;
  target.isScale = source.isScale;
  target.isConvertF32 = source.isConvertF32;
  target.isDynamicMxfpQuant = source.isDynamicMxfpQuant;
}

// AME instruction lifecycle in software ROB
// - An AME instruction is first committed in the DUT's hardware ROB, then
// sent to CUTE. When it's sent to CUTE, it is captured by AmuCtrlRecorder
// and from then on it is no longer managed by the DUT hardware ROB, but
// instead by the DiffTest framework's software ROB. In this phase, the
// instruction waits for the REF side to commit it as well; this corresponds
// to the WAIT_REF_COMMIT state.
// - Once REF has also committed this instruction, AmuCtrlChecker compares
// the amu_ctrl signals from DUT and REF. If the signals match, the state
// transitions to WAIT_DUT_EXEC, meaning we are now waiting for the DUT's
// execution result.
// - Next, the DUT's matrix unit executes the instruction and gradually
// writes the result into the matrix registers. AmuExecRecorder captures this
// writeback process and updates the mirrored registers in DiffTest. When the
// instruction has fully finished and all matrix register writes are
// complete, the state becomes WAIT_SWROB_COMMIT, waiting for the software
// ROB to commit the instruction.
// - An instruction in the software ROB can normally be committed if and only
// if it has completed and all preceding instructions have also completed.
// Mrelease is different from other asynchronous matrix operations: its
// completion directly advances DUT msync, which may unblock a following
// core-side macquire. CUTE orders mrelease only after preceding mstores, so it
// may complete while a preceding non-store is still unfinished. The recorder
// must therefore apply its REF-side effect as soon as the DUT finish event
// arrives; otherwise macquire could be checked against stale REF msync state.
// The checker then verifies that no preceding mstore is unfinished and retires
// the mrelease entry without waiting for preceding non-stores.

// 1. Capture AME commit from DUT ROB → push to software ROB (WAIT_REF_COMMIT)
static bool is_scaled_mma(const DifftestAmuCtrlEvent &event) {
  return event.op == 0 && event.isfp &&
         (event.computeType == 7 || event.computeType == 8 || event.computeType == 10);
}

static bool needs_mx_snapshot(const DifftestAmuCtrlEvent &event) {
  return event.isScale || event.isDynamicMxfpQuant || is_scaled_mma(event);
}

#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
static constexpr size_t DynamicRows = 8;
static constexpr size_t DynamicTrPitch = 32;
static constexpr size_t DynamicScaleBlocks = 2;

static unsigned scale_side(const DifftestAmuCtrlEvent &event) {
  return event.types2 ? 0 : 1;
}

static bool valid_dynamic_ctrl(const DifftestAmuCtrlEvent &event) {
  return event.op == 1 && event.isConvertF32 == 1 && event.isDynamicMxfpQuant == 1 &&
         !event.isScale && !event.sat && !event.isfp && !event.types1 && event.types2 <= 1 &&
         event.md < 4 && event.typed == 2 && event.computeType == 10 &&
         (event.types2 ? event.mtilem : event.mtilen) == DynamicRows &&
         (event.types2 ? event.mtilen : event.mtilem) == 64 && !(event.base & 3) &&
         event.stride >= 256 && !(event.stride & 3);
}

static int capture_ref_snapshot(DiffState::AmeInstRobEntry &entry, AmuCtrlEvent &ctrl, RefProxy *proxy) {
#ifdef CONFIG_DIFF_AMU_SCALE_BANK_BYTES
  const auto &event = entry.amu_event;
  const bool dynamic = event.isDynamicMxfpQuant;
  auto &mx = *entry.mx;
  RefAmuCtrlSnapshot snapshot{};
  for (unsigned side = 0; side < 2; ++side) {
    auto &bytes = mx.ref_scales[side].bytes;
    bytes.resize(CONFIG_DIFF_AMU_SCALE_BANK_BYTES);
    snapshot.scales[side].bytes = bytes.data();
    snapshot.scales[side].capacity = bytes.size();
  }
  if (dynamic) {
    mx.expected_tr.resize(DynamicRows * DynamicTrPitch);
    snapshot.expected_tr = {mx.expected_tr.data(), mx.expected_tr.size(), DynamicTrPitch, 0};
  }
  if (int ret = proxy->get_amu_ctrl_snapshot(&ctrl, &snapshot)) return ret;
  if (snapshot.version != 2 || snapshot.struct_size != sizeof(snapshot) || snapshot.reserved ||
      snapshot.epoch != event.epoch ||
      (dynamic && (snapshot.expected_tr.bytes != mx.expected_tr.data() || snapshot.expected_tr.reserved ||
                   snapshot.expected_tr.capacity != mx.expected_tr.size() ||
                   snapshot.expected_tr.row_pitch != DynamicTrPitch ||
                   (snapshot.raised_flags != 0 && snapshot.raised_flags != 0x10)))) return -2;
  for (unsigned side = 0; side < 2; ++side) {
    const auto &scale = snapshot.scales[side];
    const auto &meta = scale.metadata;
    auto &owned = mx.ref_scales[side];
    if (scale.bytes != owned.bytes.data() || scale.capacity != owned.bytes.size()) return -2;
    if (!is_scaled_mma(event) && side != scale_side(event)) {
      if (meta.generation || meta.rows || meta.blocks || meta.row_pitch || meta.reg_id || meta.side ||
          meta.valid || meta.reserved || meta.source_stride) return -2;
      owned.bytes.clear();
      continue;
    }
    const unsigned reg = is_scaled_mma(event) ? (side ? event.ms2 : event.ms1) : event.md;
    const unsigned rows = dynamic ? DynamicRows : ((side && is_scaled_mma(event)) ? event.mtilen : event.mtilem);
    const unsigned blocks = dynamic ? DynamicScaleBlocks :
                            (event.isScale ? event.mtilen : (unsigned(event.mtilek) + 31) / 32);
    const size_t live = size_t(meta.rows) * meta.blocks;
    if (!meta.generation || !rows || !blocks || meta.reserved || reg >= 4 ||
        meta.reg_id != reg || meta.side != side || meta.rows != rows || meta.blocks != blocks ||
        meta.row_pitch != blocks || meta.source_stride < blocks || live > owned.bytes.size() ||
        meta.valid != (!dynamic || snapshot.raised_flags == 0) ||
        (!is_scaled_mma(event) && meta.source_stride != event.stride)) return -2;
    owned.valid = meta.valid;
    owned.side = side;
    owned.reg_id = reg;
    owned.compute_type = event.computeType;
    owned.generation = meta.generation;
    owned.rows = rows;
    owned.blocks = blocks;
    owned.pitch = meta.row_pitch;
    owned.source_stride = meta.source_stride;
    owned.epoch = snapshot.epoch;
    owned.bytes.resize(live);
  }
  mx.ref_flags = snapshot.raised_flags;
  return 0;
#else
  return -2;
#endif
}

static void advance_mx(DiffState::AmeInstRobEntry &entry) {
  const auto &mx = *entry.mx;
  const bool complete = entry.amu_event.isDynamicMxfpQuant ?
                        mx.data_done && mx.scale_done && mx.retired && mx.returned :
                        entry.amu_event.isScale ? mx.scale_done :
                        mx.bound && mx.data_done && mx.compute_done && mx.retired && mx.returned;
  if (complete) entry.state = DiffState::WAIT_SWROB_COMMIT;
}
#endif

bool AmuCtrlRecorder::get_valid(const DifftestAmuCtrlEvent &probe) {
  return probe.valid;
}

void AmuCtrlRecorder::clear_valid(DifftestAmuCtrlEvent &probe) {
  probe.valid = 0;
}

int AmuCtrlRecorder::check(const DifftestAmuCtrlEvent &probe) {
  if (probe.op > 3) {
    Info("Invalid AMU ctrl op: core=%d, pc=0x%016lx, op=%d\n", state->coreid, probe.pc, probe.op);
    set_error_pc(state->coreid, probe.pc);
    return STATE_ERROR;
  }

  const bool needs_mx = needs_mx_snapshot(probe);
#if defined(CONFIG_DIFFTEST_AMUMXFINISHEVENT) && !defined(CONFIG_DIFFTEST_SQUASH) && \
    !defined(CONFIG_DIFFTEST_REPLAY) && !defined(BASIC_DIFFTEST_ONLY)
  if (state->cpu_amu_commits.empty()) {
    Info("AMU control without CPU retirement: core=%d pc=%#lx\n", state->coreid, uint64_t(probe.pc));
    set_error_pc(state->coreid, probe.pc);
    return STATE_ERROR;
  }
  const auto &cpu = state->cpu_amu_commits.front();
  if (cpu.id != probe.retireId || cpu.pc != probe.pc || cpu.epoch != probe.epoch || cpu.op != probe.op ||
      (probe.isDynamicMxfpQuant && !valid_dynamic_ctrl(probe))) {
    Info("CPU/AMU identity or semantic mismatch: core=%d pc=%#lx epoch=%lu id=%lu\n",
         state->coreid, uint64_t(probe.pc), uint64_t(probe.epoch), uint64_t(probe.retireId));
    set_error_pc(state->coreid, probe.pc);
    return STATE_ERROR;
  }
  state->cpu_amu_commits.pop_front();
#else
  if (needs_mx) {
    Info("MX control requires a generated MX observation schema and raw CPU retirement\n");
    set_error_pc(state->coreid, probe.pc);
    return STATE_ERROR;
  }
#endif

  if (probe.op == 0 && !is_mma_32bit_result_type(probe.typed)) {
    Info("Unsupported MMA result type: typed=%u is not 32-bit\n", static_cast<unsigned>(probe.typed));
    set_error_pc(state->coreid, probe.pc);
    return STATE_ERROR;
  }

#ifdef CONFIG_DIFFTEST_REPLAY
  state->matrix_sw_rob.emplace_back(probe, ++state->matrix_commit_sequence);
#else
  state->matrix_sw_rob.emplace_back(probe);
#endif // CONFIG_DIFFTEST_REPLAY
#if defined(CONFIG_DIFFTEST_AMUMXFINISHEVENT) && !defined(CONFIG_DIFFTEST_SQUASH) && \
    !defined(CONFIG_DIFFTEST_REPLAY) && !defined(BASIC_DIFFTEST_ONLY)
  if (needs_mx) state->matrix_sw_rob.back().mx = std::make_unique<DiffState::MxVerificationState>();
#endif
  ++state->swrob_wait_refcommit_cnt;
  return STATE_OK;
}

// 2. When REF commits same inst, compare amu_ctrl → on match set WAIT_DUT_EXEC
int AmuCtrlChecker::do_step() {
  // Every WAIT_REF entry originates in AmuCtrlRecorder. A successful control
  // comparison removes exactly one from this count; result-only WB cannot add one.
  if (state->swrob_wait_refcommit_cnt == 0)
    return STATE_OK;
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end(); ++iter) {
    if (iter->state == DiffState::WAIT_REF_COMMIT) {
      const DifftestAmuCtrlEvent dut_event = iter->amu_event;
      DifftestAmuCtrlEvent ref_event = dut_event;
      struct AmuCtrlEvent ref_event_nemu = to_nemu_amu_ctrl(ref_event);
      int check_res;
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
      if (iter->mx) {
        check_res = capture_ref_snapshot(*iter, ref_event_nemu, proxy);
      } else
#endif
      {
        check_res = proxy->get_amu_ctrl_event(&ref_event_nemu);
      }
      from_nemu_amu_ctrl(ref_event, ref_event_nemu);

      if (check_res == 1) {
        printf("\n==============  Amu Mma Ctrl Event (Core %d)  ==============\n", state->coreid);
        printf("Mismatch for amu ctrl event \n");
        print_amu_ctrl_event("REF", ref_event);
        print_amu_ctrl_event("DUT", dut_event);
        set_error_pc(state->coreid, dut_event.pc);
        return STATE_ERROR;
      } else if (check_res == -1) {
        printf("\n==============  Amu Mma Ctrl Event (Core %d)  ==============\n", state->coreid);
        printf("  No available REF AMU ctrl\n");
        print_amu_ctrl_event("DUT", dut_event);
        set_error_pc(state->coreid, dut_event.pc);
        return STATE_ERROR;
      } else if (check_res != 0) {
        Info("Unsupported/invalid REF AMU control snapshot: pc=%#lx status=%d\n", uint64_t(dut_event.pc), check_res);
        set_error_pc(state->coreid, dut_event.pc);
        return STATE_ERROR;
      } else {
        iter->state = DiffState::WAIT_DUT_EXEC;
        --state->swrob_wait_refcommit_cnt;
      }
    }
  }
  return STATE_OK;
}

// 3. Record DUT matrix writeback and completion events before software ROB retirement
static int mark_writeback_complete(DiffState::AmeInstRobEntry &entry) {
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
  if (entry.mx) {
    if (!is_scaled_mma(entry.amu_event) || !entry.res || entry.mx->data_done) {
      return DiffTestChecker::STATE_ERROR;
    }
    entry.mx->data_done = true;
    advance_mx(entry);
    return DiffTestChecker::STATE_OK;
  }
#endif
  entry.state = DiffState::WAIT_SWROB_COMMIT;
  return DiffTestChecker::STATE_OK;
}

#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
static void note_mx_write(DiffState::AmeInstRobEntry &entry, size_t offset, size_t bytes, size_t capacity) {
  if (!entry.mx) return;
  auto &written = entry.mx->result_coverage;
  if (written.empty()) written.resize(capacity);
  assert(offset <= written.size() && bytes <= written.size() - offset);
  std::fill_n(written.begin() + offset, bytes, uint8_t{1});
}

static bool mx_writeback_done(const DiffState::AmeInstRobEntry &entry) {
  return entry.mx && entry.mx->data_done;
}
#endif

bool AmuExecRecorder::get_valid(const DifftestAmuWritebackEvent &probe) {
  return probe.valid;
}

void AmuExecRecorder::clear_valid(DifftestAmuWritebackEvent &probe) {
  probe.valid = 0;
}

void AmuExecRecorder::record_writeback(DiffState::AmeInstRobEntry &entry, const DifftestAmuWritebackEvent &probe) {
  const static size_t ARLen = CONFIG_DIFF_AMU_ARLEN;
  const static size_t TRLen = CONFIG_DIFF_AMU_TRLEN;
  const size_t matrix_size =
      entry.amu_event.md < 4 ? CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES : CONFIG_DIFF_AMU_C_REG_SIZE_BYTES;
  const size_t matrix_u64_size = (matrix_size + sizeof(uint64_t) - 1) / sizeof(uint64_t);
  if (entry.res == NULL) {
    // first `valid` for this inst: alloc space for matrix inst
    if (entry.amu_event.op == 0) {
      entry.res = difftest[state->coreid]->get_mma_verifier()->allocate_result();
    } else {
      entry.res.reset(new uint64_t[matrix_u64_size]());
    }
  }
  uint8_t md = entry.amu_event.md;
  const size_t matrix_words_per_bank = (md < 4) ? CONFIG_DIFF_AMU_AB_WORDS_PER_BANK : CONFIG_DIFF_AMU_C_WORDS_PER_BANK;
  // bankAddr is an entry index in the selected matrix register. The
  // writeback event may carry a wider, shared payload than that entry
  // (e.g. C is 128 bits in MinimalMatrixConfig, while the event is
  // still 256 bits for the A/B path). Derive the interleave stride from
  // the selected register's physical entry width, not the event width.
  const size_t row_bits = (md < 4) ? TRLen : ARLen;
  const size_t entry_bits = matrix_words_per_bank * sizeof(uint64_t) * 8;
  assert(entry_bits > 0);
  assert(row_bits % entry_bits == 0);
  const size_t stride = row_bits / entry_bits;
  assert(stride > 0);
  assert(matrix_words_per_bank > 0);

  for (int j = 0; j < CONFIG_DIFF_AMU_WRITEBACK_BANKS; ++j) { // for each bank
    if ((probe.bankValid >> j) & 0x1U) {
      const size_t addr = probe.bankAddr[j];
      const size_t matrix_entry = addr / stride * stride * CONFIG_DIFF_AMU_WRITEBACK_BANKS + j * stride + addr % stride;
      const size_t idx = matrix_entry * matrix_words_per_bank;
      assert(idx + matrix_words_per_bank <= matrix_u64_size);

      uint8_t *dst = reinterpret_cast<uint8_t *>(&entry.res[idx]);
      const uint8_t *src = reinterpret_cast<const uint8_t *>(&probe.data[j * CONFIG_DIFF_AMU_WRITEBACK_WORDS_PER_BANK]);
      const size_t bank_bytes = matrix_words_per_bank * sizeof(uint64_t);
#ifdef CONFIG_DIFF_AMU_WRITEBACK_NO_MASK
      memcpy(dst, src, bank_bytes);
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
      note_mx_write(entry, idx * sizeof(uint64_t), bank_bytes, matrix_size);
#endif
#else
      const uint64_t mask = probe.bankMask[j];
      for (size_t k = 0; k < bank_bytes; ++k) {
        if ((mask >> k) & 0x1U) {
          dst[k] = src[k];
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
          note_mx_write(entry, idx * sizeof(uint64_t) + k, 1, matrix_size);
#endif
        }
      }
#endif
    }
  }
}

int AmuExecRecorder::check(const DifftestAmuWritebackEvent &probe) {
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end(); ++iter) {
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
    if (mx_writeback_done(*iter)) continue;
#endif
    if (iter->amu_event.pc == probe.pc && iter->state == DiffState::WAIT_DUT_EXEC) {
      const auto &event = iter->amu_event;
#ifdef CONFIG_DIFF_AMU_WRITEBACK_NO_MASK
      // MMA
      bool expects_writeback = event.op == 0;
#else
      // Raw loads/zero may coexist with hashed loads on other probe channels.
      bool expects_writeback = event.op == 0 || (event.op == 1 && event.sat == 0) || event.op == 3;
#endif // CONFIG_DIFF_AMU_WRITEBACK_NO_MASK
      if (!expects_writeback) {
        printf("Unexpected writeback event: core %d, pc 0x%016lx, op %d\n", state->coreid, probe.pc, event.op);
        set_error_pc(state->coreid, probe.pc);
        return STATE_ERROR;
      }
      record_writeback(*iter, probe);
      if (probe.finish) {
        if (int ret = mark_writeback_complete(*iter)) {
          set_error_pc(state->coreid, probe.pc);
          return ret;
        }
      }
      return STATE_OK;
    }
  }
  printf("No matching AMU instruction for writeback event: core %d, pc 0x%016lx\n", state->coreid, probe.pc);
  set_error_pc(state->coreid, probe.pc);
  return STATE_ERROR;
}

#ifdef CONFIG_DIFFTEST_AMUWRITEBACKBLOCK
int AmuWritebackBlockRecorder::check(const DifftestAmuWritebackBlock &probe) {
  constexpr size_t banks = CONFIG_DIFF_AMU_WRITEBACK_BANKS;
  constexpr size_t bank_bytes = CONFIG_DIFF_AMU_C_WORDS_PER_BANK * sizeof(uint64_t);
  constexpr size_t stride = CONFIG_DIFF_AMU_ARLEN / (8 * bank_bytes);
  constexpr size_t entries_per_bank = CONFIG_DIFF_AMU_C_REG_SIZE_BYTES / (banks * bank_bytes);
  constexpr size_t block_capacity = sizeof(probe.bankAddr) / sizeof(probe.bankAddr[0]);
  if (probe.count == 0 || probe.count > block_capacity) {
    printf("Invalid MMA writeback block count: %u\n", unsigned(probe.count));
    set_error_pc(state->coreid, probe.pc);
    return STATE_ERROR;
  }

  for (auto &entry: state->matrix_sw_rob) {
    if (entry.amu_event.pc != probe.pc || entry.state != DiffState::WAIT_DUT_EXEC) {
      continue;
    }
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
    if (mx_writeback_done(entry)) continue;
#endif
    if (entry.amu_event.op != 0) {
      break;
    }
    if (!entry.res) {
      entry.res = difftest[state->coreid]->get_mma_verifier()->allocate_result();
    }
    auto *dst = reinterpret_cast<uint8_t *>(entry.res.get());
    const auto *src = reinterpret_cast<const uint8_t *>(&probe) + offsetof(DifftestAmuWritebackBlock, data);
    for (unsigned i = 0; i < probe.count; ++i) {
      const size_t addr = probe.bankAddr[i];
      if (addr >= entries_per_bank) {
        printf("Invalid MMA writeback block address: %zu\n", addr);
        set_error_pc(state->coreid, probe.pc);
        return STATE_ERROR;
      }
      const size_t base = (addr / stride * stride * banks + addr % stride) * bank_bytes;
      for (size_t bank = 0; bank < banks; ++bank) {
        const size_t offset = base + bank * stride * bank_bytes;
        memcpy(dst + offset, src, bank_bytes);
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
        note_mx_write(entry, offset, bank_bytes, CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
#endif
        src += bank_bytes;
      }
    }
    if (probe.finish) {
      if (int ret = mark_writeback_complete(entry)) {
        set_error_pc(state->coreid, probe.pc);
        return ret;
      }
    }
    return STATE_OK;
  }
  printf("No matching MMA instruction for writeback block: core %d, pc 0x%016lx\n", state->coreid, probe.pc);
  set_error_pc(state->coreid, probe.pc);
  return STATE_ERROR;
}
#endif

#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
using MxFinish = DifftestAmuMxFinishEvent;
using AmuEntry = DiffState::AmeInstRobEntry;
using ScaleImage = DiffState::OwnedScaleSnapshot;
static constexpr size_t MxBanks = CONFIG_DIFF_AMU_MX_FINISH_BANKS;
static constexpr size_t MxChunkBytes = CONFIG_DIFF_AMU_MX_FINISH_WORDS_PER_BANK * sizeof(uint64_t);

static ScaleImage scale_descriptor(const MxFinish &probe, unsigned side) {
  ScaleImage scale;
  scale.valid = side ? probe.scaleValidB : probe.scaleValidA;
  scale.side = side;
  scale.reg_id = side ? probe.scaleRegB : probe.scaleRegA;
  scale.bank = side ? probe.scaleBankB : probe.scaleBankA;
  scale.compute_type = probe.computeType;
  scale.generation = side ? probe.scaleGenB : probe.scaleGenA;
  scale.rows = side ? probe.scaleRowsB : probe.scaleRowsA;
  scale.vectors = side ? probe.scaleKVectorsB : probe.scaleKVectorsA;
  scale.blocks = side ? probe.scaleBlocksB : probe.scaleBlocksA;
  scale.pitch = side ? probe.scaleRowPitchB : probe.scaleRowPitchA;
  scale.epoch = probe.epoch;
  return scale;
}

static bool same_scale_shape(const ScaleImage &actual, const ScaleImage &expected) {
  return actual.side == expected.side && actual.reg_id == expected.reg_id &&
         actual.generation == expected.generation && actual.compute_type == expected.compute_type &&
         actual.rows == expected.rows && actual.blocks == expected.blocks &&
         actual.pitch == expected.pitch && actual.epoch == expected.epoch;
}

static bool same_scale_image(const ScaleImage &actual, const ScaleImage &expected) {
  return same_scale_shape(actual, expected) && actual.valid == expected.valid &&
         actual.source_stride == expected.source_stride && actual.bytes == expected.bytes;
}

static bool scale_fields_empty(const MxFinish &probe, unsigned side) {
  const auto scale = scale_descriptor(probe, side);
  return !scale.valid && !scale.reg_id && !scale.bank && !scale.generation &&
         !scale.rows && !scale.vectors && !scale.blocks && !scale.pitch;
}

static bool mx_chunks_empty(const MxFinish &probe) {
  for (size_t bank = 0; bank < MxBanks; ++bank) {
    if (probe.bankValid[bank] || probe.bankMask[bank]) return false;
  }
  return true;
}

static const uint8_t *mx_chunk(const MxFinish &probe, size_t bank) {
  return reinterpret_cast<const uint8_t *>(&probe) + offsetof(MxFinish, data) + bank * MxChunkBytes;
}

static bool record_scale_destination(DiffState &state, AmuEntry &entry, const MxFinish &probe) {
#if defined(CONFIG_DIFF_AMU_SCALE_BANK_BYTES) && defined(CONFIG_DIFF_AMU_SCALE_BEAT_BYTES) && \
    defined(CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES)
  const auto &event = entry.amu_event;
  auto &mx = *entry.mx;
  const bool dynamic = event.isDynamicMxfpQuant;
  const unsigned side = probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_SCALE_A ? 0 : 1;
  const auto &expected = mx.ref_scales[side];
  auto observed = scale_descriptor(probe, side);
  const bool mx4 = observed.compute_type == 10;
  const unsigned per_vector = CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES / (mx4 ? 16 : 32);
  const size_t live = expected.bytes.size();
  if ((!event.isScale && !dynamic) || side != scale_side(event) || mx.scale_done ||
      observed.bank >= 4 || !per_vector || !live || live > CONFIG_DIFF_AMU_SCALE_BANK_BYTES ||
      !same_scale_shape(observed, expected) || observed.vectors != (observed.blocks + per_vector - 1) / per_vector ||
      observed.valid != (probe.finish && expected.valid) || !scale_fields_empty(probe, 1 - side) ||
      probe.flagsValid || probe.flagsRaised) return false;
  observed.source_stride = event.stride;
  auto &pending = mx.pending_scale;
  if (pending.generation == 0) {
    pending = std::move(observed);
    pending.bytes.resize(live);
    pending.coverage.resize(live);
    state.dut_scale_banks[side][pending.bank].valid = false;
  } else if (observed.bank != pending.bank || observed.vectors != pending.vectors) {
    return false;
  }

  for (size_t bank = 0; bank < MxBanks; ++bank) {
    const uint64_t mask = probe.bankMask[bank];
    if (!probe.bankValid[bank] && mask) return false;
    for (size_t byte = 0; byte < MxChunkBytes; ++byte) {
      if (!(mask >> byte & 1)) continue;
      const size_t chunk_offset = bank * MxChunkBytes + byte;
      const size_t offset = size_t(probe.bankAddr[bank]) * CONFIG_DIFF_AMU_SCALE_BEAT_BYTES + chunk_offset;
      if (chunk_offset >= CONFIG_DIFF_AMU_SCALE_BEAT_BYTES || offset >= live || pending.coverage[offset]) return false;
      pending.bytes[offset] = mx_chunk(probe, bank)[byte];
      pending.coverage[offset] = 1;
    }
  }
  if (probe.finish) {
    if (std::find(pending.coverage.begin(), pending.coverage.end(), 0) != pending.coverage.end()) return false;
    pending.valid = bool(side ? probe.scaleValidB : probe.scaleValidA);
    if (!same_scale_image(pending, expected)) return false;
    pending.coverage.clear();
    state.dut_scale_banks[side][pending.bank] = pending;
    mx.scale_done = true;
    advance_mx(entry);
  }
  return true;
#else
  return false;
#endif
}

static bool record_mma_phase(DiffState &state, AmuEntry &entry, const MxFinish &probe) {
#ifdef CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES
  const auto &ctrl = entry.amu_event;
  auto &mx = *entry.mx;
  const bool bind = probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_MMA_BIND;
  const size_t elements_per_vector = CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES * (ctrl.computeType == 10 ? 2 : 1);
  if (!is_scaled_mma(ctrl) || !elements_per_vector || bool(probe.finish) == bind ||
      bool(probe.flagsValid) == bind || probe.flagsRaised > 31 || !mx_chunks_empty(probe) ||
      (bind ? (mx.bound || probe.flagsRaised) : (!mx.bound || mx.compute_done))) return false;
  const size_t vectors = (ctrl.mtilek + elements_per_vector - 1) / elements_per_vector;
  for (unsigned side = 0; side < 2; ++side) {
    const auto observed = scale_descriptor(probe, side);
    const auto &expected = mx.ref_scales[side];
    if (!observed.valid || observed.bank >= 4 || observed.vectors != vectors ||
        !same_scale_shape(observed, expected)) return false;
    const auto &resident = bind ? state.dut_scale_banks[side][observed.bank] : mx.dut_scales[side];
    if (resident.bank != observed.bank || resident.vectors != vectors || !same_scale_image(resident, expected)) {
      return false;
    }
  }
  if (bind) {
    mx.dut_scales[0] = state.dut_scale_banks[0][probe.scaleBankA];
    mx.dut_scales[1] = state.dut_scale_banks[1][probe.scaleBankB];
    mx.bound = true;
  } else {
    mx.dut_flags = probe.flagsRaised;
    mx.compute_done = true;
    advance_mx(entry);
  }
  return true;
#else
  return false;
#endif
}

static bool record_mx_retirement(DiffState &state, AmuEntry &entry, const MxFinish &probe) {
  auto &mx = *entry.mx;
  const bool dynamic = entry.amu_event.isDynamicMxfpQuant;
  const bool retiring = probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_ORDERED_RETIRE;
  auto &last_id = retiring ? state.mx_retired_id : state.mx_returned_id;
  if ((!dynamic && (!is_scaled_mma(entry.amu_event) || !mx.compute_done)) ||
      !probe.finish || !probe.flagsValid || probe.flagsRaised != (dynamic ? mx.ref_flags : mx.dut_flags) ||
      !scale_fields_empty(probe, 0) || !scale_fields_empty(probe, 1) || !mx_chunks_empty(probe) ||
      probe.retireId <= last_id || (retiring ? mx.retired : (!mx.retired || mx.returned))) return false;
  for (const auto &older: state.matrix_sw_rob) {
    if (older.amu_event.retireId >= probe.retireId) break;
    if (older.mx && !older.amu_event.isScale && !(retiring ? older.mx->retired : older.mx->returned)) return false;
  }
  last_id = probe.retireId;
  if (retiring) mx.retired = true;
  else mx.returned = true;
  if (dynamic) mx.dut_flags = probe.flagsRaised;
  advance_mx(entry);
  return true;
}

static bool record_dynamic_tr(AmuEntry &entry, const MxFinish &probe) {
  constexpr size_t bank_bytes = CONFIG_DIFF_AMU_AB_WORDS_PER_BANK * sizeof(uint64_t);
  constexpr size_t pitch = CONFIG_DIFF_AMU_TRLEN / 8;
  constexpr size_t capacity = CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES;
  static_assert(bank_bytes && bank_bytes <= 64 && pitch && pitch % bank_bytes == 0);
  constexpr size_t stride = pitch / bank_bytes;
  auto &mx = *entry.mx;
  if (!entry.amu_event.isDynamicMxfpQuant || mx.data_done || bank_bytes > MxChunkBytes ||
      probe.flagsValid || probe.flagsRaised || !scale_fields_empty(probe, 0) || !scale_fields_empty(probe, 1)) {
    return false;
  }
  if (!entry.res) entry.res = MatrixResult(new uint64_t[(capacity + sizeof(uint64_t) - 1) / sizeof(uint64_t)]());
  auto *dst = reinterpret_cast<uint8_t *>(entry.res.get());
  for (size_t bank = 0; bank < MxBanks; ++bank) {
    const uint64_t mask = probe.bankMask[bank];
    if (!probe.bankValid[bank]) {
      if (mask) return false;
      continue;
    }
    const size_t addr = probe.bankAddr[bank];
    const size_t offset = (addr / stride * stride * MxBanks + bank * stride + addr % stride) * bank_bytes;
    if (offset > capacity || bank_bytes > capacity - offset) return false;
    for (size_t byte = 0; byte < MxChunkBytes; ++byte) {
      if (!(mask >> byte & 1)) continue;
      const size_t location = offset + byte;
      if (byte >= bank_bytes || location / pitch >= DynamicRows || location % pitch >= DynamicTrPitch ||
          (!mx.result_coverage.empty() && mx.result_coverage[location])) return false;
      dst[location] = mx_chunk(probe, bank)[byte];
      note_mx_write(entry, location, 1, capacity);
    }
  }
  if (probe.finish) {
    mx.data_done = true;
    advance_mx(entry);
  }
  return true;
}

int AmuMxFinishRecorder::check(const MxFinish &probe) {
  for (auto &entry: state->matrix_sw_rob) {
    const auto &ctrl = entry.amu_event;
    if (!entry.mx || entry.state != DiffState::WAIT_DUT_EXEC || ctrl.pc != probe.pc ||
        ctrl.retireId != probe.retireId || ctrl.epoch != probe.epoch || ctrl.computeType != probe.computeType) continue;
    bool ok = false;
    switch (probe.kind) {
      case CONFIG_DIFF_AMU_FINISH_KIND_SCALE_A:
      case CONFIG_DIFF_AMU_FINISH_KIND_SCALE_B:
        ok = record_scale_destination(*state, entry, probe);
        break;
      case CONFIG_DIFF_AMU_FINISH_KIND_MMA_BIND:
      case CONFIG_DIFF_AMU_FINISH_KIND_MMA_DONE:
        ok = record_mma_phase(*state, entry, probe);
        break;
      case CONFIG_DIFF_AMU_FINISH_KIND_ORDERED_RETIRE:
      case CONFIG_DIFF_AMU_FINISH_KIND_RETURN_ACCEPTED:
        ok = record_mx_retirement(*state, entry, probe);
        break;
      case CONFIG_DIFF_AMU_FINISH_KIND_DYNAMIC_TR:
        ok = record_dynamic_tr(entry, probe);
        break;
    }
    if (ok) return STATE_OK;
    break;
  }
  Info("Invalid MX observation: core=%d pc=%#lx id=%lu epoch=%lu kind=%u\n",
       state->coreid, uint64_t(probe.pc), uint64_t(probe.retireId), uint64_t(probe.epoch), unsigned(probe.kind));
  set_error_pc(state->coreid, probe.pc);
  return STATE_ERROR;
}
#endif

#ifdef CONFIG_DIFFTEST_AMUFINISHEVENT
bool AmuFinishRecorder::get_valid(const DifftestAmuFinishEvent &probe) {
  return probe.valid;
}

void AmuFinishRecorder::clear_valid(DifftestAmuFinishEvent &probe) {
  probe.valid = 0;
}

int AmuFinishRecorder::check(const DifftestAmuFinishEvent &probe) {
  for (auto &entry: state->matrix_sw_rob) {
    if (entry.amu_event.pc != probe.pc || entry.state != DiffState::WAIT_DUT_EXEC) {
      continue;
    }
    const auto &event = entry.amu_event;
    if (event.op == 2) {
      // Apply msync before a following core-side macquire can be checked.
      struct AmuCtrlEvent amu_event_nemu = to_nemu_amu_ctrl(event);
      if (proxy->exec_amu(&amu_event_nemu) != 0) {
        printf("Failed to execute REF mrelease: core %d, pc 0x%016lx\n", state->coreid, event.pc);
        set_error_pc(state->coreid, event.pc);
        return STATE_ERROR;
      }
      state->mrelease_check_pending = true;
    } else if (event.op != 1 || event.sat != 1) {
      printf("Expected mstore/mrelease for finish event: core %d, pc 0x%016lx\n", state->coreid, probe.pc);
      set_error_pc(state->coreid, probe.pc);
      return STATE_ERROR;
    }
    entry.state = DiffState::WAIT_SWROB_COMMIT;
    return STATE_OK;
  }
  printf("No matching AMU instruction for finish event: core %d, pc 0x%016lx\n", state->coreid, probe.pc);
  set_error_pc(state->coreid, probe.pc);
  return STATE_ERROR;
}
#endif // CONFIG_DIFFTEST_AMUFINISHEVENT

#ifdef CONFIG_DIFFTEST_AMUHASHEVENT
bool AmuHashExecRecorder::get_valid(const DifftestAmuHashEvent &probe) {
  return probe.valid;
}

void AmuHashExecRecorder::clear_valid(DifftestAmuHashEvent &probe) {
  probe.valid = 0;
}

int AmuHashExecRecorder::check(const DifftestAmuHashEvent &probe) {
  for (auto &entry: state->matrix_sw_rob) {
    if (entry.amu_event.pc == probe.pc && entry.state == DiffState::WAIT_DUT_EXEC) {
      const auto &event = entry.amu_event;
      if (needs_mx_snapshot(event) || !(event.op == 3 || (event.op == 1 && event.sat == 0)))
        break;
      entry.hash = {probe.hashLo, 0, probe.byteCount};
      entry.state = DiffState::WAIT_SWROB_COMMIT;
      return STATE_OK;
    }
  }
  printf("No matching mload/mzero for hash completion: pc 0x%016lx\n", probe.pc);
  set_error_pc(state->coreid, probe.pc);
  return STATE_ERROR;
}
#endif

#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
static RefAmuScaleSnapshot ref_scale_view(ScaleImage &scale) {
  return {scale.bytes.data(), scale.bytes.size(),
          {scale.generation, scale.rows, scale.blocks, scale.pitch, scale.reg_id,
           scale.side, uint8_t(scale.valid), 0, scale.source_stride}};
}

static bool matches_result_rows(const AmuEntry &entry, const std::vector<uint8_t> &expected,
                                size_t rows, size_t active_bytes, size_t physical_pitch) {
  const auto &coverage = entry.mx->result_coverage;
  if (!entry.res || !rows || !active_bytes || !physical_pitch || active_bytes > physical_pitch ||
      rows > coverage.size() / physical_pitch || rows > expected.size() / active_bytes ||
      expected.size() != rows * active_bytes) return false;
  const auto *data = reinterpret_cast<const uint8_t *>(entry.res.get());
  for (size_t row = 0; row < rows; ++row) {
    const size_t offset = row * physical_pitch;
    if (std::find(coverage.begin() + offset, coverage.begin() + offset + active_bytes, 0) !=
            coverage.begin() + offset + active_bytes ||
        memcmp(data + offset, expected.data() + row * active_bytes, active_bytes) != 0) return false;
  }
  return true;
}

static bool verify_mx_result(AmuEntry &entry, RefProxy *proxy) {
  auto &mx = *entry.mx;
  const auto &event = entry.amu_event;
  auto ctrl = to_nemu_amu_ctrl(event);
  RefAmuCtrlSnapshot scales{};
  scales.epoch = event.epoch;
  RefAmuMmaSnapshot snapshot{};
  snapshot.scales = &scales;
  if (event.isScale) return mx.scale_done;
  if (event.isDynamicMxfpQuant) {
    const unsigned side = scale_side(event);
    if (!mx.scale_done || mx.dut_flags != mx.ref_flags ||
        !matches_result_rows(entry, mx.expected_tr, DynamicRows, DynamicTrPitch, CONFIG_DIFF_AMU_TRLEN / 8)) {
      return false;
    }
    scales.scales[side] = ref_scale_view(mx.ref_scales[side]);
    scales.expected_tr = {mx.expected_tr.data(), mx.expected_tr.size(), DynamicTrPitch, 0};
    scales.raised_flags = mx.ref_flags;
    return proxy->get_amu_lazy_snapshot(&ctrl, &snapshot) == 0;
  }

  if (!mx.bound || !mx.compute_done || !is_scaled_mma(event)) return false;
  const size_t rows = event.mtilem;
  const size_t cols = event.mtilen;
  const size_t depth = event.mtilek;
  const size_t ab_pitch = event.computeType == 10 ? (depth + 1) / 2 : depth;
  const size_t c_pitch = cols * sizeof(uint32_t);
  if (!rows || !cols || !ab_pitch || ab_pitch > CONFIG_DIFF_AMU_TRLEN / 8 || c_pitch > CONFIG_DIFF_AMU_ARLEN / 8 ||
      std::max(rows, cols) > CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES / (CONFIG_DIFF_AMU_TRLEN / 8) ||
      rows > CONFIG_DIFF_AMU_C_REG_SIZE_BYTES / (CONFIG_DIFF_AMU_ARLEN / 8)) return false;
  for (unsigned side = 0; side < 2; ++side) {
    if (!same_scale_image(mx.dut_scales[side], mx.ref_scales[side])) return false;
    scales.scales[side] = ref_scale_view(mx.ref_scales[side]);
  }
  std::vector<uint8_t> matrices[4];
  const size_t view_rows[] = {rows, cols, rows, rows};
  const size_t view_pitch[] = {ab_pitch, ab_pitch, c_pitch, c_pitch};
  for (unsigned idx = 0; idx < 4; ++idx) {
    matrices[idx].resize(view_rows[idx] * view_pitch[idx]);
    snapshot.matrices[idx] = {matrices[idx].data(), matrices[idx].size(), uint32_t(view_pitch[idx]), 0};
  }
  if (proxy->get_amu_lazy_snapshot(&ctrl, &snapshot) != 0 || snapshot.raised_flags != mx.dut_flags) return false;
  return matches_result_rows(entry, matrices[3], rows, c_pitch, CONFIG_DIFF_AMU_ARLEN / 8);
}
#endif

int AmuExecChecker::commit_ready_prefix() {
  Difftest *dt = difftest[state->coreid];
  MmaVerifier *mma_verifier = dt->get_mma_verifier();

  // Retire the ready prefix in software-ROB order. The first non-ready entry
  // blocks ordinary operations, but not a completed mrelease; check_mreleases()
  // handles those entries separately.
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end();) {
    if (iter->state == DiffState::WAIT_SWROB_COMMIT) {
      DifftestAmuCtrlEvent amu_event = iter->amu_event;
#ifdef CONFIG_DIFFTEST_AMUMXFINISHEVENT
      if (iter->mx) {
        if (!verify_mx_result(*iter, proxy)) {
          Info("MX result/snapshot mismatch: core=%d pc=%#lx id=%lu epoch=%lu\n", state->coreid,
               uint64_t(amu_event.pc), uint64_t(amu_event.retireId), uint64_t(amu_event.epoch));
          set_error_pc(state->coreid, amu_event.pc);
          return STATE_ERROR;
        }
        iter = state->matrix_sw_rob.erase(iter);
        continue;
      }
#endif
      uint8_t op = amu_event.op;
      MmaVerificationBuffer *buffer = nullptr;
      struct AmuCtrlEvent amu_event_nemu = {};
      switch (op) {
        case 0: // MMA
          // Allocate buffer for MMA verification
          buffer = mma_verifier->allocate_buffer(&amu_event);
          // Transfer the completed software-ROB result without allocating a copy.
          buffer->dut_result = std::move(iter->res);
#ifdef CONFIG_DIFFTEST_REPLAY
          buffer->sequence = iter->sequence;
#endif // CONFIG_DIFFTEST_REPLAY \
    // Capture REF operands and install the DUT result for asynchronous MMA verification.
          amu_event_nemu = to_nemu_amu_ctrl(amu_event);
          if (proxy->exec_amu_lazy(&amu_event_nemu, buffer->dut_result.get(), buffer->src1, buffer->src2,
                                   buffer->src3) != 0) {
            printf("Failed to get REF operands for MMA verification: core %d, pc 0x%016lx\n", state->coreid,
                   amu_event.pc);
            mma_verifier->free_buffer(buffer);
            set_error_pc(state->coreid, amu_event.pc);
            return STATE_ERROR;
          }
          // Submit using the build-selected verification schedule.
          mma_verifier->add_to_verification_queue(buffer);
#ifdef CONFIG_DIFFTEST_MMA_SYNC
          // Do not consume the next ready event after a synchronous failure.
          if (mma_verifier->has_mma_verification_error()) {
            set_error_pc(state->coreid, amu_event.pc);
            return STATE_ERROR;
          }
#endif
          break;
        case 1:   // MLS
        case 3: { // Arith
          amu_event_nemu = to_nemu_amu_ctrl(amu_event);
#ifdef CONFIG_DIFFTEST_AMUHASHEVENT
          const int result = iter->res ? proxy->exec_amu(&amu_event_nemu, iter->res.get())
                                       : proxy->exec_amu_hash(&amu_event_nemu, &iter->hash);
#else
          const int result = proxy->exec_amu(&amu_event_nemu, iter->res.get());
#endif
          if (result != 0) {
            printf("Mismatch for amu exec event: pc 0x%016lx, op %s\n", amu_event.pc, amu_ctrl_op_name(amu_event.op));
            set_error_pc(state->coreid, amu_event.pc);
            return STATE_ERROR;
          }
          break;
        }
        case 2: // MRelease
          // The REF-side effect was applied when the DUT finish event was recorded.
          break;
        default:
          printf("Unknown amu event op: %d\n", op);
          iter->res.reset();
          set_error_pc(state->coreid, amu_event.pc);
          return STATE_ERROR;
      }
      // Remove the processed event from matrix_sw_rob
      iter = state->matrix_sw_rob.erase(iter);
    } else {
      // When there's an unfinished inst before the current inst, the current inst will not be committed.
      // So, when the current inst is unfinished, break the loop.
      break;
    }
  }
  return STATE_OK;
}

int AmuExecChecker::check_mreleases() {
  // Retire completed mrelease entries without waiting for older non-stores.
  // CUTE makes mrelease depend only on older mstores, so a completed mrelease
  // with an unfinished older mstore indicates a DUT ordering violation.
  bool unfinished_store_seen = false;
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end();) {
    if (iter->state == DiffState::WAIT_SWROB_COMMIT && iter->amu_event.op == 2) {
      if (unfinished_store_seen) {
        printf("Mrelease completed before an older mstore: core %d, mrelease pc 0x%016lx\n", state->coreid,
               iter->amu_event.pc);
        set_error_pc(state->coreid, iter->amu_event.pc);
        return STATE_ERROR;
      }
      iter = state->matrix_sw_rob.erase(iter);
    } else {
      if (iter->state != DiffState::WAIT_SWROB_COMMIT && iter->amu_event.op == 1 && iter->amu_event.sat == 1) {
        unfinished_store_seen = true;
      }
      ++iter;
    }
  }
  return STATE_OK;
}

// 4. Commit ready inst from software ROB: REF exec and compare
int AmuExecChecker::do_step() {
  if (int ret = commit_ready_prefix()) {
    return ret;
  }
  // A successful scan leaves no completed mrelease in the ROB.
  if (!state->mrelease_check_pending)
    return STATE_OK;
  const int ret = check_mreleases();
  if (ret == STATE_OK)
    state->mrelease_check_pending = false;
  return ret;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#ifdef CONFIG_DIFFTEST_MSYNCEVENT

static void print_msync_event(const char *source, const DifftestMsyncEvent &event) {
  const char *op_name = "UNKNOWN";
  switch (event.op) {
    case 0: op_name = "msyncregreset"; break;
    case 1: op_name = "macquire"; break;
    case 2: op_name = "mfence"; break;
  }

  printf("  %s Msync: pc 0x%016lx, op %s, MsyncRd %d\n", source, event.pc, op_name, event.msyncRd);
  if (event.op > 2) {
    printf("               Unknown Msync event op: %d\n", event.op);
  }
}

static struct MsyncEvent to_nemu_msync(const DifftestMsyncEvent &source) {
  struct MsyncEvent target = {};
  target.valid = source.valid;
  target.op = source.op;
  target.msyncRd = source.msyncRd;
  target.pc = source.pc;
  return target;
}

static void from_nemu_msync(DifftestMsyncEvent &target, const struct MsyncEvent &source) {
  target.valid = source.valid;
  target.op = source.op;
  target.msyncRd = source.msyncRd;
  target.pc = source.pc;
}

int MsyncChecker::do_step() {
  while (!state->msync_event_queue.empty()) {
    const DifftestMsyncEvent dut_event = state->msync_event_queue.front();
    DifftestMsyncEvent ref_event = dut_event;
    struct MsyncEvent ref_event_nemu = to_nemu_msync(ref_event);
    int check_res = proxy->get_msync_event(&ref_event_nemu);
    from_nemu_msync(ref_event, ref_event_nemu);

    if (check_res == 1) {
      // Compare the msync event info.
      // For mismatch, proxy returns 1 and sets the msync event info.
      // Use global difftest pointer to access the owning Difftest instance for this core
      printf("\n==============  Msync Event (Core %d)  ==============\n", state->coreid);
      printf("Mismatch for Msync event\n");
      print_msync_event("REF", ref_event);
      print_msync_event("DUT", dut_event);
      state->msync_event_queue.pop();
      set_error_pc(state->coreid, dut_event.pc);
      return STATE_ERROR;
    } else if (check_res == -1) {
      printf("\n==============  Msync Event (Core %d)  ==============\n", state->coreid);
      printf("  No available REF Msync events\n");
      print_msync_event("DUT", dut_event);
      state->msync_event_queue.pop();
      set_error_pc(state->coreid, dut_event.pc);
      return STATE_ERROR;
    }
    state->msync_event_queue.pop();
  }
  return STATE_OK;
}

bool MsyncRecorder::get_valid(const DifftestMsyncEvent &probe) {
  return probe.valid;
}

void MsyncRecorder::clear_valid(DifftestMsyncEvent &probe) {
  probe.valid = 0;
}

int MsyncRecorder::check(const DifftestMsyncEvent &probe) {
  state->msync_event_queue.push(probe);
  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_MSYNCEVENT
