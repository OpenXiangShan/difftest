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

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

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

#ifndef CONFIG_DIFF_AMU_FINISH_BANKS
#define CONFIG_DIFF_AMU_FINISH_BANKS 8
#endif

#ifndef CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK
#define CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK 4
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

static inline size_t get_amu_result_size(const DifftestAmuCtrlEvent &amu_event) {
  switch (amu_event.op) {
    case 0: // MMA
      // Finish events address physical banks, not a compact active matrix.
      return CONFIG_DIFF_AMU_C_REG_SIZE_BYTES;
    case 1: // Matrix load/store
      return (amu_event.sat == 0) ?
        (amu_event.md < 4 ? CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES : CONFIG_DIFF_AMU_C_REG_SIZE_BYTES) : 0;
    case 3: // Matrix Arith
      // MARITH writes a whole matrix register selected by md:
      // md < 4  -> tile register (AB), md >= 4 -> accumulator register (C).
      return (amu_event.md < 4) ? CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES : CONFIG_DIFF_AMU_C_REG_SIZE_BYTES;
    default: return 0;
  }
}

static bool normal_observed_tile_load(const DifftestAmuCtrlEvent &event) {
  return event.op == 1 && event.sat == 0 && !event.isScale && !event.isfp &&
    !event.isConvertF32 && !event.isDynamicMxfpQuant && !event.types1 && event.md < 4 &&
    !event.computeType && (event.typed == 0 || event.typed == 7);
}

static bool is_dynamic_mxfp_load(const DifftestAmuCtrlEvent &event) {
  return event.isDynamicMxfpQuant;
}

static bool valid_dynamic_mxfp_ctrl(const DifftestAmuCtrlEvent &event) {
  const unsigned rows = event.types2 ? event.mtilem : event.mtilen;
  const unsigned k = event.types2 ? event.mtilen : event.mtilem;
  return event.op == 1 && event.isConvertF32 == 1 && event.isDynamicMxfpQuant == 1 &&
    !event.isScale && !event.sat && !event.isfp && !event.types1 &&
    event.types2 <= 1 && event.md < 4 && event.typed == 2 && event.computeType == 10 &&
    rows == 8 && k == 64 && !(event.base & 3) &&
    event.stride >= 256 && !(event.stride & 3);
}

// Freeze the existing REF control-pop descriptor into this SWROB owner.
// In particular, valid=false is a rejected generation, not missing SF bytes.
static int capture_dynamic_mxfp_snapshot(DiffState::AmeInstRobEntry &entry,
                                        const RefAmuCtrlSnapshot &snapshot) {
  const auto &ctrl = entry.amu_event;
  const unsigned side = ctrl.types2 ? 0 : 1;
  const auto &tr = snapshot.expected_tr;
  if (!valid_dynamic_mxfp_ctrl(ctrl) || entry.ref_snapshot_seen ||
      snapshot.version != 2 || snapshot.struct_size != sizeof(snapshot) ||
      snapshot.epoch != ctrl.epoch || snapshot.reserved ||
      (snapshot.raised_flags != 0 && snapshot.raised_flags != 0x10) ||
      !tr.bytes || tr.capacity < 256 || tr.row_pitch != 32 || tr.reserved) {
    Info("Invalid dynamic REF control/result snapshot\n");
    return DiffTestChecker::STATE_ERROR;
  }
  for (unsigned s = 0; s < 2; ++s) {
    const auto &scale = snapshot.scales[s];
    const auto &meta = scale.metadata;
    if (s != side) {
      if (meta.valid || meta.generation || meta.rows || meta.blocks || meta.row_pitch ||
          meta.reg_id || meta.side || meta.reserved || meta.source_stride) {
        return DiffTestChecker::STATE_ERROR;
      }
    } else if (!scale.bytes || scale.capacity < 16 || meta.reserved ||
               !meta.generation || meta.reg_id != ctrl.md || meta.side != side ||
               meta.rows != 8 || meta.blocks != 2 || meta.row_pitch != 2 ||
               meta.source_stride != ctrl.stride ||
               meta.valid != (snapshot.raised_flags == 0)) {
      Info("Invalid dynamic REF provisional scale snapshot\n");
      return DiffTestChecker::STATE_ERROR;
    }
  }
  const auto &scale = snapshot.scales[side];
  const auto &meta = scale.metadata;
  auto &owned = entry.ref_scale_snapshots[side];
  owned.valid = meta.valid;
  owned.side = side;
  owned.regId = ctrl.md;
  owned.computeType = ctrl.computeType;
  owned.generation = meta.generation;
  owned.rows = meta.rows;
  owned.kVectors = 1;
  owned.blocks = meta.blocks;
  owned.rowPitch = meta.row_pitch;
  owned.sourceStride = meta.source_stride;
  owned.epoch = snapshot.epoch;
  owned.retireId = ctrl.retireId;
  owned.bytes.assign(scale.bytes, scale.bytes + 16);
  entry.ref_expected_tr.assign(tr.bytes, tr.bytes + 256);
  entry.ref_raised_flags = snapshot.raised_flags;
  entry.ref_snapshot_seen = true;
  return DiffTestChecker::STATE_OK;
}

static bool has_scale_metadata(const DifftestAmuFinishEvent &probe) {
  return probe.scaleRegA || probe.scaleRegB || probe.scaleBankA || probe.scaleBankB ||
    probe.scaleGenA || probe.scaleGenB || probe.scaleValidA || probe.scaleValidB ||
    probe.scaleRowsA || probe.scaleRowsB || probe.scaleKVectorsA || probe.scaleKVectorsB ||
    probe.scaleBlocksA || probe.scaleBlocksB || probe.scaleRowPitchA || probe.scaleRowPitchB;
}

static void advance_dynamic_mxfp_load(DiffState::AmeInstRobEntry &entry) {
  // Four independently delayed observations, not the first finish token.
  if (entry.dynamic_tr_done && entry.dynamic_scale_done &&
      entry.dynamic_retire_seen && entry.dynamic_return_seen) {
    entry.state = DiffState::WAIT_SWROB_COMMIT;
  }
}

static bool mx_load_coverage_complete(const DiffState::AmeInstRobEntry &entry) {
  const auto &event = entry.amu_event;
  const size_t rows = event.types2 ? event.mtilem : event.mtilen;
  const size_t k = event.types2 ? event.mtilen : event.mtilem;
  const size_t bytes = is_dynamic_mxfp_load(event) ? 32 :
    (event.typed == 7 ? (k + 1) / 2 : k);
  constexpr size_t pitch = CONFIG_DIFF_AMU_TRLEN / 8;
  if (!rows || !bytes || bytes > pitch ||
      rows > CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES / pitch ||
      entry.load_coverage.size() != CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES) return false;
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < bytes; ++col) {
      if (!entry.load_coverage[row * pitch + col]) return false;
    }
  }
  return true;
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
// - An instruction in the software ROB can be committed if and only if it
// has completed and all preceding instructions have also completed.
// AmuExecChecker commits such ready instructions: the REF side re-executes
// the instruction and compares its result against the DUT result. If they
// match, the lifecycle of this instruction ends and its resources can be
// released.

// 1. Capture AME commit from DUT ROB → push to software ROB (WAIT_REF_COMMIT)
bool AmuCtrlRecorder::get_valid(const DifftestAmuCtrlEvent &probe) {
  return probe.valid;
}

void AmuCtrlRecorder::clear_valid(DifftestAmuCtrlEvent &probe) {
  probe.valid = 0;
}

int AmuCtrlRecorder::check(const DifftestAmuCtrlEvent &probe) {
  if (probe.retireId == 0 || probe.retireId <= state->last_amu_retire_id ||
      probe.epoch < state->last_amu_epoch) {
    Info("Invalid AMU committed identity: core=%d pc=0x%016lx epoch=%lu retireId=%lu last=%lu/%lu\n",
         state->coreid, probe.pc, probe.epoch, probe.retireId,
         state->last_amu_epoch, state->last_amu_retire_id);
    return STATE_ERROR;
  }
  if (probe.op > 3) {
    Info("Invalid AMU ctrl op: core=%d, pc=0x%016lx, op=%d\n", state->coreid, probe.pc, probe.op);
    return STATE_ERROR;
  }
  if ((probe.op != 1 && (probe.isConvertF32 || probe.isDynamicMxfpQuant)) ||
      (is_dynamic_mxfp_load(probe) && !valid_dynamic_mxfp_ctrl(probe))) {
    Info("Invalid dynamic/converting AMU semantic selector\n");
    return STATE_ERROR;
  }

  if (probe.op == 0 && !is_mma_32bit_result_type(probe.typed)) {
    Info("Unsupported MMA result type: typed=%u is not 32-bit\n", static_cast<unsigned>(probe.typed));
    return STATE_ERROR;
  }

#if defined(CONFIG_DIFFTEST_SQUASH) || defined(CONFIG_DIFFTEST_REPLAY) || defined(BASIC_DIFFTEST_ONLY)
  fprintf(stderr, "AMU CPU retirement attribution requires cold, raw, unsquashed, non-replay commits: core=%d\n",
          state->coreid);
  return STATE_ERROR;
#else
  if (state->cpu_amu_commits.empty()) {
    fprintf(stderr, "AMU control without CPU retirement: core=%d pc=0x%016lx "
      "epoch=%lu retireId=%lu\n", state->coreid, probe.pc, probe.epoch, probe.retireId);
    return STATE_ERROR;
  }
  const auto &cpu = state->cpu_amu_commits.front();
  if (cpu.retireId != probe.retireId || cpu.pc != probe.pc ||
      cpu.epoch != probe.epoch || cpu.op != probe.op) {
    fprintf(stderr, "CPU/AMU retirement mismatch: core=%d "
      "CPU=(pc=0x%016lx id=%lu epoch=%lu op=%u) "
      "AMU=(pc=0x%016lx id=%lu epoch=%lu op=%u)\n", state->coreid,
      cpu.pc, cpu.retireId, cpu.epoch, cpu.op,
      probe.pc, probe.retireId, probe.epoch, probe.op);
    return STATE_ERROR;
  }
#endif // CONFIG_DIFFTEST_SQUASH

  DiffState::AmeInstRobEntry entry;
  entry.amu_event = probe;
  entry.state = DiffState::WAIT_REF_COMMIT;
  entry.res = NULL;
  state->matrix_sw_rob.push_back(entry);
  state->cpu_amu_commits.pop_front();
  ++state->cpu_amu_matched;
  state->last_amu_retire_id = probe.retireId;
  state->last_amu_epoch = probe.epoch;
  return STATE_OK;
}

// 2. When REF commits same inst, compare amu_ctrl → on match set WAIT_DUT_EXEC
int AmuCtrlChecker::do_step() {
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end(); ++iter) {
    if (iter->state == DiffState::WAIT_REF_COMMIT) {
      const DifftestAmuCtrlEvent dut_event = iter->amu_event;
      DifftestAmuCtrlEvent ref_event = dut_event;
      // NOTE:
      // AMU ctrl is treated as a non-squashable, ordering-sensitive event.
      // Under squash mode, it still requires one-to-one synchronization with REF.
      // Therefore "no available REF event" (-1) is an error instead of a retriable case.
      const bool dynamic = is_dynamic_mxfp_load(dut_event);
      const bool snapshot_required = dynamic || dut_event.isScale || is_mma_mx(dut_event);
      RefAmuCtrlSnapshot snapshot{};
      std::vector<uint8_t> bytes[2];
      std::vector<uint8_t> expected_tr;
      if (snapshot_required) {
#ifdef CONFIG_DIFF_AMU_SCALE_BANK_BYTES
        for (unsigned side = 0; side < 2; ++side) {
          bytes[side].resize(CONFIG_DIFF_AMU_SCALE_BANK_BYTES);
          snapshot.scales[side].bytes = bytes[side].data();
          snapshot.scales[side].capacity = bytes[side].size();
        }
#else
        Info("MX control requires generated scale capacity\n");
        return STATE_ERROR;
#endif
      }
      if (dynamic) {
        expected_tr.resize(256);
        snapshot.expected_tr = {expected_tr.data(), expected_tr.size(), 32, 0};
      }
      int check_res = snapshot_required ? proxy->get_amu_ctrl_snapshot(&ref_event, &snapshot)
                                        : proxy->get_amu_ctrl_event(&ref_event);

      if (check_res == 1) {
        printf("\n==============  Amu Mma Ctrl Event (Core %d)  ==============\n", state->coreid);
        printf("Mismatch for amu ctrl event \n");
        print_amu_ctrl_event("REF", ref_event);
        print_amu_ctrl_event("DUT", dut_event);
        return STATE_ERROR;
      } else if (check_res == -1) {
        printf("\n==============  Amu Mma Ctrl Event (Core %d)  ==============\n", state->coreid);
        printf("  No available REF AMU ctrl\n");
        print_amu_ctrl_event("DUT", dut_event);
        return STATE_ERROR;
      } else if (check_res != 0) {
        Info("Unsupported/invalid REF control snapshot: status=%d\n", check_res);
        return STATE_ERROR;
      }
      if (dynamic) {
        if (snapshot.expected_tr.bytes != expected_tr.data() ||
            snapshot.expected_tr.capacity != expected_tr.size() ||
            snapshot.scales[0].bytes != bytes[0].data() ||
            snapshot.scales[1].bytes != bytes[1].data() ||
            snapshot.scales[0].capacity != bytes[0].size() ||
            snapshot.scales[1].capacity != bytes[1].size() ||
            capture_dynamic_mxfp_snapshot(*iter, snapshot) != STATE_OK) {
          return STATE_ERROR;
        }
      } else if (snapshot_required) {
        if (snapshot.version != 2 || snapshot.struct_size != sizeof(snapshot) ||
            snapshot.epoch != dut_event.epoch || snapshot.reserved) {
          Info("REF/DUT control snapshot ABI or epoch mismatch\n");
          return STATE_ERROR;
        }
        for (unsigned side = 0; side < 2; ++side) {
          const bool used = is_mma_mx(dut_event) || bool(dut_event.types2) == (side == 0);
          const auto &meta = snapshot.scales[side].metadata;
          if (!used) {
            if (meta.valid || meta.generation || meta.rows || meta.blocks) return STATE_ERROR;
            continue;
          }
          const unsigned reg = dut_event.isScale ? dut_event.md :
            (side == 0 ? dut_event.ms1 : dut_event.ms2);
          const size_t live = size_t(meta.rows) * meta.blocks;
          const unsigned rows = dut_event.isScale ? dut_event.mtilem :
            (side == 0 ? dut_event.mtilem : dut_event.mtilen);
          const unsigned blocks = dut_event.isScale ? dut_event.mtilen :
            (unsigned(dut_event.mtilek) + 31) / 32;
          if (!meta.valid || !meta.generation || meta.reserved || reg >= 4 ||
              meta.reg_id != reg || meta.side != side || meta.rows < rows ||
              meta.blocks < blocks || meta.row_pitch != meta.blocks ||
              meta.source_stride < meta.blocks || !live || live > bytes[side].size() ||
              (dut_event.isScale && (meta.rows != rows || meta.blocks != blocks ||
                                    meta.source_stride != dut_event.stride))) {
            Info("Invalid owned REF scale side/reg/shape/capacity\n");
            return STATE_ERROR;
          }
          auto &owned = iter->ref_scale_snapshots[side];
          owned.valid = true;
          owned.side = side;
          owned.regId = reg;
          owned.computeType = dut_event.computeType;
          owned.generation = meta.generation;
          owned.rows = meta.rows;
          owned.blocks = meta.blocks;
          owned.rowPitch = meta.row_pitch;
          owned.sourceStride = meta.source_stride;
          owned.epoch = snapshot.epoch;
          owned.retireId = dut_event.retireId;
          owned.bytes.assign(bytes[side].begin(), bytes[side].begin() + live);
        }
        iter->ref_snapshot_seen = true;
      }
      iter->state = DiffState::WAIT_DUT_EXEC;
    }
  }
  return STATE_OK;
}

// 3. Capture DUT matrix writeback, update register copy → on finish set WAIT_SWROB_COMMIT
bool AmuExecRecorder::get_valid(const DifftestAmuFinishEvent &probe) {
  return probe.valid;
}

void AmuExecRecorder::clear_valid(DifftestAmuFinishEvent &probe) {
  probe.valid = 0;
}

static int record_scale_destination(DiffState *state, DiffState::AmeInstRobEntry &entry,
                                    const DifftestAmuFinishEvent &probe) {
#if defined(CONFIG_DIFF_AMU_SCALE_BANK_BYTES) && defined(CONFIG_DIFF_AMU_SCALE_BEAT_BYTES) && \
    defined(CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES)
  const bool a = probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_SCALE_A;
  const unsigned side = a ? 0 : 1;
  const unsigned reg = a ? probe.scaleRegA : probe.scaleRegB;
  const unsigned bank = a ? probe.scaleBankA : probe.scaleBankB;
  const uint32_t generation = a ? probe.scaleGenA : probe.scaleGenB;
  const unsigned rows = a ? probe.scaleRowsA : probe.scaleRowsB;
  const unsigned vectors = a ? probe.scaleKVectorsA : probe.scaleKVectorsB;
  const unsigned blocks = a ? probe.scaleBlocksA : probe.scaleBlocksB;
  const uint32_t pitch = a ? probe.scaleRowPitchA : probe.scaleRowPitchB;
  const bool publish = a ? probe.scaleValidA : probe.scaleValidB;
  const auto &ctrl = entry.amu_event;
  const bool dynamic = is_dynamic_mxfp_load(ctrl);
  const bool mx4 = probe.computeType == 10;
  const bool mx8 = probe.computeType == 7 || probe.computeType == 8;
  const size_t per_vector = CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES / (mx4 ? 16 : 32);
  const size_t live = size_t(rows) * blocks;
  if (ctrl.op != 1 || (!ctrl.isScale && !dynamic) || ctrl.sat != 0 || bool(ctrl.types2) != a ||
      reg >= 4 || bank >= 4 || generation == 0 || rows == 0 || blocks == 0 ||
      (!mx4 && !mx8) || per_vector == 0 || vectors != (blocks + per_vector - 1) / per_vector ||
      reg != ctrl.md || rows != (dynamic ? 8 : ctrl.mtilem) ||
      blocks != (dynamic ? 2 : ctrl.mtilen) || pitch != blocks ||
      ctrl.stride < blocks || probe.computeType != ctrl.computeType ||
      live > CONFIG_DIFF_AMU_SCALE_BANK_BYTES ||
      (dynamic ? (publish && !probe.finish) : publish != bool(probe.finish))) {
    Info("Invalid scale destination metadata: core=%d epoch=%lu retireId=%lu side=%u reg=%u bank=%u\n",
         state->coreid, probe.epoch, probe.retireId, side, reg, bank);
    return DiffTestChecker::STATE_ERROR;
  }
  if (dynamic) {
    const auto &ref = entry.ref_scale_snapshots[side];
    const bool other_metadata = a ?
      (probe.scaleRegB || probe.scaleBankB || probe.scaleGenB || probe.scaleValidB ||
       probe.scaleRowsB || probe.scaleKVectorsB || probe.scaleBlocksB || probe.scaleRowPitchB) :
      (probe.scaleRegA || probe.scaleBankA || probe.scaleGenA || probe.scaleValidA ||
       probe.scaleRowsA || probe.scaleKVectorsA || probe.scaleBlocksA || probe.scaleRowPitchA);
    if (!entry.ref_snapshot_seen || entry.dynamic_scale_done || other_metadata ||
        probe.flagsValid || probe.flagsRaised || generation != ref.generation ||
        (probe.finish && publish != (entry.ref_raised_flags == 0))) {
      Info("Dynamic SF owner/status/publication mismatch\n");
      return DiffTestChecker::STATE_ERROR;
    }
  }
  auto &pending = entry.scale_snapshot;
  if (pending.generation == 0) {
    pending.side = side;
    pending.regId = reg;
    pending.bank = bank;
    pending.computeType = probe.computeType;
    pending.generation = generation;
    pending.rows = rows;
    pending.kVectors = vectors;
    pending.blocks = blocks;
    pending.rowPitch = pitch;
    pending.sourceStride = ctrl.stride;
    pending.retireId = probe.retireId;
    pending.epoch = probe.epoch;
    pending.bytes.resize(live);
    pending.coverage.resize(live, 0);
    // The first observed replacement write invalidates the old physical bank.
    // No caller can treat a reserved/incomplete new generation as published.
    state->dut_scale_banks[side][bank].valid = false;
  }
  if (pending.valid || pending.side != side || pending.regId != reg || pending.bank != bank ||
      pending.generation != generation || pending.computeType != probe.computeType ||
      pending.rows != rows || pending.blocks != blocks || pending.rowPitch != pitch ||
      pending.kVectors != vectors || pending.sourceStride != ctrl.stride ||
      pending.retireId != probe.retireId || pending.epoch != probe.epoch) {
    Info("Scale destination owner/shape changed during load\n");
    return DiffTestChecker::STATE_ERROR;
  }
  constexpr size_t chunk_bytes = CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK * sizeof(uint64_t);
  static_assert(chunk_bytes <= 64, "existing finish masks must cover each payload chunk");
  for (size_t b = 0; b < CONFIG_DIFF_AMU_FINISH_BANKS; ++b) {
    const uint64_t mask = probe.bankMask[b];
    if (!probe.bankValid[b]) {
      if (mask != 0) return DiffTestChecker::STATE_ERROR;
      continue;
    }
    const size_t beat_offset = size_t(probe.bankAddr[b]) * CONFIG_DIFF_AMU_SCALE_BEAT_BYTES;
    const size_t chunk_offset = b * chunk_bytes;
    const auto *bytes = reinterpret_cast<const uint8_t *>(
        &probe.data[b * CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK]);
    for (size_t k = 0; k < chunk_bytes; ++k) {
      if ((mask >> k) & 1) {
        const size_t offset = beat_offset + chunk_offset + k;
        if (chunk_offset + k >= CONFIG_DIFF_AMU_SCALE_BEAT_BYTES ||
            offset >= live || pending.coverage[offset] != 0) {
          Info("Scale byte out of range or written twice: offset=%zu live=%zu\n", offset, live);
          return DiffTestChecker::STATE_ERROR;
        }
        pending.bytes[offset] = bytes[k];
        pending.coverage[offset] = 1;
      }
    }
  }
  if (probe.finish) {
    for (uint8_t covered : pending.coverage) {
      if (!covered) {
        Info("Scale publication before complete destination byte coverage\n");
        return DiffTestChecker::STATE_ERROR;
      }
    }
    pending.valid = publish;
    state->dut_scale_banks[side][bank] = pending;
    if (dynamic) {
      entry.dynamic_scale_done = true;
      advance_dynamic_mxfp_load(entry);
    } else {
      entry.state = DiffState::WAIT_SWROB_COMMIT;
    }
  }
  return DiffTestChecker::STATE_OK;
#else
  // An old/unattested geometry must not silently guess scale capacity or pitch.
  Info("Scale destination observation requires generated scale geometry\n");
  return DiffTestChecker::STATE_ERROR;
#endif
}

static bool is_scaled_mma(const DifftestAmuCtrlEvent &ctrl) {
  return ctrl.op == 0 && ctrl.isfp &&
         (ctrl.computeType == 7 || ctrl.computeType == 8 || ctrl.computeType == 10);
}

static void advance_scaled_mma(DiffState::AmeInstRobEntry &entry) {
  // CDC's legacy lane is delayed; retire/return may arrive before its data end.
  // Keep this owner alive until every independently observed stage has arrived.
  if (entry.mma_bind_seen && entry.mma_data_done && entry.mma_owned_done &&
      entry.mma_retire_seen && entry.mma_return_seen) {
    entry.state = DiffState::WAIT_SWROB_COMMIT;
  }
}

static int record_mma_retirement(DiffState *state, DiffState::AmeInstRobEntry &entry,
                                 const DifftestAmuFinishEvent &probe) {
  const bool retiring = probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_ORDERED_RETIRE;
  const bool dynamic = is_dynamic_mxfp_load(entry.amu_event);
  const auto id = entry.amu_event.retireId;
  const bool retired_seen = dynamic ? entry.dynamic_retire_seen : entry.mma_retire_seen;
  const bool returned_seen = dynamic ? entry.dynamic_return_seen : entry.mma_return_seen;
  const uint8_t flags = dynamic ? entry.ref_raised_flags : entry.dut_raised_flags;
  if ((!is_scaled_mma(entry.amu_event) && !dynamic) ||
      (dynamic ? !entry.ref_snapshot_seen : !entry.mma_owned_done) ||
      !probe.finish || !probe.flagsValid || probe.flagsRaised > 31 ||
      probe.computeType != entry.amu_event.computeType ||
      probe.flagsRaised != flags ||
      (retiring && (retired_seen || id <= state->last_mma_retire_id)) ||
      (!retiring && ((!dynamic && !retired_seen) || returned_seen ||
                     id <= state->last_mma_return_id))) {
    Info("Invalid scaled MMA ordered retirement/accepted return\n");
    return DiffTestChecker::STATE_ERROR;
  }
  for (size_t b = 0; b < CONFIG_DIFF_AMU_FINISH_BANKS; ++b) {
    if (probe.bankValid[b] || probe.bankMask[b]) return DiffTestChecker::STATE_ERROR;
  }
  if (has_scale_metadata(probe)) {
    Info("Unexpected scale metadata on a return/retirement token\n");
    return DiffTestChecker::STATE_ERROR;
  }
  for (const auto &older : state->matrix_sw_rob) {
    if (older.amu_event.retireId >= id) break;
    const bool older_dynamic = is_dynamic_mxfp_load(older.amu_event);
    if ((older_dynamic && (retiring ? !older.dynamic_retire_seen : !older.dynamic_return_seen)) ||
        (is_scaled_mma(older.amu_event) &&
         (retiring ? !older.mma_retire_seen : (older.mma_retire_seen && !older.mma_return_seen)))) {
      Info("Scaled MMA retirement/return skipped an older owner\n");
      return DiffTestChecker::STATE_ERROR;
    }
  }
  if (retiring) {
    if (dynamic) entry.dynamic_retire_seen = true;
    else entry.mma_retire_seen = true;
    state->last_mma_retire_id = id;
  } else {
    if (dynamic) entry.dynamic_return_seen = true;
    else entry.mma_return_seen = true;
    state->last_mma_return_id = id;
  }
  if (dynamic) {
    entry.dut_raised_flags = probe.flagsRaised;
    advance_dynamic_mxfp_load(entry);
  } else {
    advance_scaled_mma(entry);
  }
  return DiffTestChecker::STATE_OK;
}

static int record_mma_phase(DiffState *state, DiffState::AmeInstRobEntry &entry,
                            const DifftestAmuFinishEvent &probe) {
#ifdef CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES
  const auto &ctrl = entry.amu_event;
  const bool bind = probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_MMA_BIND;
  const unsigned blocks = (unsigned(ctrl.mtilek) + 31) / 32;
  const unsigned elements_per_vector =
      CONFIG_DIFF_AMU_SCALE_VECTOR_DATA_BYTES * (ctrl.computeType == 10 ? 2 : 1);
  const unsigned vectors = (unsigned(ctrl.mtilek) + elements_per_vector - 1) / elements_per_vector;
  if (!is_scaled_mma(ctrl) || !ctrl.mtilek || !ctrl.mtilem || !ctrl.mtilen ||
      probe.computeType != ctrl.computeType || bool(probe.finish) == bind ||
      bool(probe.flagsValid) == bind || probe.flagsRaised > 31 ||
      (bind && (entry.mma_bind_seen || probe.flagsRaised != 0)) ||
      (!bind && (!entry.mma_bind_seen || entry.mma_owned_done))) {
    Info("Invalid scaled MMA bind/completion phase\n");
    return DiffTestChecker::STATE_ERROR;
  }
  for (size_t b = 0; b < CONFIG_DIFF_AMU_FINISH_BANKS; ++b) {
    if (probe.bankValid[b] || probe.bankMask[b]) return DiffTestChecker::STATE_ERROR;
  }
  for (unsigned side = 0; side < 2; ++side) {
    const unsigned reg = side == 0 ? probe.scaleRegA : probe.scaleRegB;
    const unsigned bank = side == 0 ? probe.scaleBankA : probe.scaleBankB;
    const unsigned generation = side == 0 ? probe.scaleGenA : probe.scaleGenB;
    const unsigned rows = side == 0 ? probe.scaleRowsA : probe.scaleRowsB;
    const unsigned kv = side == 0 ? probe.scaleKVectorsA : probe.scaleKVectorsB;
    const unsigned kb = side == 0 ? probe.scaleBlocksA : probe.scaleBlocksB;
    const unsigned pitch = side == 0 ? probe.scaleRowPitchA : probe.scaleRowPitchB;
    const bool valid = side == 0 ? probe.scaleValidA : probe.scaleValidB;
    if (reg >= 4 || bank >= 4 || !valid || !generation ||
        reg != (side == 0 ? ctrl.ms1 : ctrl.ms2) ||
        rows != (side == 0 ? ctrl.mtilem : ctrl.mtilen) ||
        kv != vectors || kb != blocks || pitch != blocks) {
      Info("MMA bound scale shape/architectural ID is invalid\n");
      return DiffTestChecker::STATE_ERROR;
    }
    const auto &snapshot = bind ? state->dut_scale_banks[side][bank]
                                : entry.mma_scale_snapshots[side];
    if (!snapshot.valid || snapshot.side != side || snapshot.bank != bank ||
        snapshot.regId != reg || snapshot.generation != generation ||
        snapshot.epoch != probe.epoch || snapshot.computeType != probe.computeType ||
        snapshot.rows != rows || snapshot.kVectors != kv || snapshot.blocks != kb ||
        snapshot.rowPitch != pitch || snapshot.bytes.size() != size_t(rows) * blocks) {
      Info("MMA references an unpublished, stale or mismatched DUT scale snapshot\n");
      return DiffTestChecker::STATE_ERROR;
    }
  }
  // Copy only after BOTH sides have passed. A subsequent reload cannot mutate
  // either snapshot already owned by this in-flight MMA.
  if (bind) {
    entry.mma_scale_snapshots[0] = state->dut_scale_banks[0][probe.scaleBankA];
    entry.mma_scale_snapshots[1] = state->dut_scale_banks[1][probe.scaleBankB];
    entry.mma_bind_seen = true;
  } else {
    entry.dut_raised_flags = probe.flagsRaised;
    entry.mma_owned_done = true;
    advance_scaled_mma(entry);
  }
  return DiffTestChecker::STATE_OK;
#else
  Info("MMA scale binding requires generated geometry\n");
  return DiffTestChecker::STATE_ERROR;
#endif
}

int AmuExecRecorder::check(const DifftestAmuFinishEvent &probe) {
  const static size_t ARLen = CONFIG_DIFF_AMU_ARLEN;
  const static size_t TRLen = CONFIG_DIFF_AMU_TRLEN;
  for (auto &entry: state->matrix_sw_rob) {
    if (entry.amu_event.retireId == probe.retireId && entry.amu_event.epoch == probe.epoch &&
        entry.state == DiffState::WAIT_DUT_EXEC) {
      if (entry.amu_event.pc != probe.pc) {
        Info("AMU identity matched with different PC: core=%d epoch=%lu retireId=%lu\n",
             state->coreid, probe.epoch, probe.retireId);
        return STATE_ERROR;
      }
      if (probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_SCALE_A ||
          probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_SCALE_B) {
        return record_scale_destination(state, entry, probe);
      }
      if (probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_MMA_BIND ||
          probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_MMA_DONE) {
        return record_mma_phase(state, entry, probe);
      }
      if (probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_ORDERED_RETIRE ||
          probe.kind == CONFIG_DIFF_AMU_FINISH_KIND_RETURN_ACCEPTED) {
        return record_mma_retirement(state, entry, probe);
      }
      if (probe.kind != CONFIG_DIFF_AMU_FINISH_KIND_LEGACY || entry.amu_event.isScale) {
        Info("Unsupported/misrouted AMU observation kind=%u\n", unsigned(probe.kind));
        return STATE_ERROR;
      }
      const bool scaled = is_scaled_mma(entry.amu_event);
      const bool dynamic = is_dynamic_mxfp_load(entry.amu_event);
      if (dynamic && (entry.dynamic_tr_done || has_scale_metadata(probe) ||
                      probe.flagsValid || probe.flagsRaised || probe.computeType)) {
        Info("Dynamic TR data after End or with non-legacy metadata\n");
        return STATE_ERROR;
      }
      if (scaled && (!entry.mma_bind_seen || entry.mma_data_done)) {
        Info("Scaled MMA C data before binding or after data completion\n");
        return STATE_ERROR;
      }
      const size_t matrix_size = get_amu_result_size(entry.amu_event);
      size_t matrix_u64_size = (matrix_size + sizeof(uint64_t) - 1) / sizeof(uint64_t);
      if (matrix_u64_size == 0) {
        matrix_u64_size = 1;
      }
      if (entry.res == NULL) {
        // first `valid` for this inst: alloc space for matrix inst
        entry.res = new uint64_t[matrix_u64_size];
        memset(entry.res, 0, matrix_u64_size * sizeof(uint64_t));
        if (normal_observed_tile_load(entry.amu_event) || dynamic)
          entry.load_coverage.resize(matrix_size, 0);
      }
      uint8_t md = entry.amu_event.md;
      const size_t matrix_words_per_bank =
          (md < 4) ? CONFIG_DIFF_AMU_AB_WORDS_PER_BANK : CONFIG_DIFF_AMU_C_WORDS_PER_BANK;
      const size_t entry_bits = matrix_words_per_bank * 64;
      const size_t row_bits = md < 4 ? TRLen : ARLen;
      // Finish transport pads narrow C entries to the maximum AB/C width.
      // Decode physical addresses with the actual destination entry width.
      if (!entry_bits || row_bits % entry_bits ||
          matrix_words_per_bank > CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK) {
        Info("Invalid AMU physical entry/row geometry\n");
        return STATE_ERROR;
      }
      const size_t stride = row_bits / entry_bits;
      if (!stride) return STATE_ERROR;

      for (int j = 0; j < CONFIG_DIFF_AMU_FINISH_BANKS; ++j) { // for each bank
        if (dynamic && !probe.bankValid[j] && probe.bankMask[j]) {
          Info("Dynamic TR mask without a valid physical write\n");
          return STATE_ERROR;
        }
        if (probe.bankValid[j]) {
          const size_t addr = probe.bankAddr[j];
          const size_t matrix_entry =
              addr / stride * stride * CONFIG_DIFF_AMU_FINISH_BANKS + j * stride + addr % stride;
          const size_t idx = matrix_entry * matrix_words_per_bank;
          if (idx + matrix_words_per_bank > matrix_u64_size) {
            Info("AMU bank write exceeds physical register capacity\n");
            return STATE_ERROR;
          }

          uint8_t *dst = reinterpret_cast<uint8_t *>(&entry.res[idx]);
          const uint8_t *src =
              reinterpret_cast<const uint8_t *>(&probe.data[j * CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK]);
          const uint64_t mask = probe.bankMask[j];
          const size_t bank_bytes = matrix_words_per_bank * sizeof(uint64_t);
          if (dynamic && bank_bytes < 64 && (mask >> bank_bytes)) return STATE_ERROR;
          for (size_t k = 0; k < bank_bytes; ++k) {
            if ((mask >> k) & 0x1U) {
              if (dynamic) {
                const size_t offset = idx * sizeof(uint64_t) + k;
                if (offset / (TRLen / 8) >= 8 || offset % (TRLen / 8) >= 32 ||
                    entry.load_coverage[offset]) {
                  Info("Dynamic TR byte inactive or written twice\n");
                  return STATE_ERROR;
                }
              }
              dst[k] = src[k];
              if (!entry.load_coverage.empty()) entry.load_coverage[idx * sizeof(uint64_t) + k] = 1;
            }
          }
        }
      }
      if (probe.finish) {
        if ((normal_observed_tile_load(entry.amu_event) || dynamic) &&
            !mx_load_coverage_complete(entry)) {
          Info("Tile load completed before all semantic active bytes were observed\n");
          return STATE_ERROR;
        }
        if (dynamic) {
          entry.dynamic_tr_done = true;
          advance_dynamic_mxfp_load(entry);
        } else if (scaled) {
          entry.mma_data_done = true;
          advance_scaled_mma(entry);
        } else {
          entry.state = DiffState::WAIT_SWROB_COMMIT;
        }
      }
      return STATE_OK;
    }
  }
  printf("No matching AMU instruction for finish event: core %d pc 0x%016lx epoch=%lu retireId=%lu\n",
         state->coreid, probe.pc, probe.epoch, probe.retireId);
  return STATE_ERROR;
}

// 4. Commit ready inst from software ROB: REF re-exec and compare, then release
static bool equal_scale_snapshots(const DiffState::OwnedScaleSnapshot &dut,
                                   const DiffState::OwnedScaleSnapshot &ref,
                                   bool allow_unpublished = false) {
  return (allow_unpublished ? dut.valid == ref.valid : dut.valid && ref.valid) &&
    dut.side == ref.side && dut.regId == ref.regId &&
    dut.epoch == ref.epoch && dut.generation == ref.generation &&
    dut.computeType == ref.computeType && dut.rows == ref.rows &&
    dut.blocks == ref.blocks && dut.rowPitch == ref.rowPitch &&
    dut.sourceStride == ref.sourceStride && dut.bytes == ref.bytes;
}

static bool copy_active_rows(uint8_t *dst, size_t capacity, const void *src,
                             size_t source_capacity, size_t rows, size_t row_bytes,
                             size_t physical_pitch) {
  if (!dst || !src || !rows || !row_bytes || row_bytes > physical_pitch ||
      rows > capacity / row_bytes ||
      (rows - 1) * physical_pitch + row_bytes > source_capacity) return false;
  for (size_t row = 0; row < rows; ++row) {
    memcpy(dst + row * row_bytes, static_cast<const uint8_t *>(src) + row * physical_pitch, row_bytes);
  }
  return true;
}

static bool compare_dynamic_mxfp_result(const DiffState::AmeInstRobEntry &entry) {
  const auto &ctrl = entry.amu_event;
  const unsigned side = ctrl.types2 ? 0 : 1;
  std::vector<uint8_t> packed(256);
  return valid_dynamic_mxfp_ctrl(ctrl) && entry.ref_snapshot_seen &&
    entry.state == DiffState::WAIT_SWROB_COMMIT &&
    entry.dynamic_tr_done && entry.dynamic_scale_done &&
    entry.dynamic_retire_seen && entry.dynamic_return_seen &&
    (entry.ref_raised_flags == 0 || entry.ref_raised_flags == 0x10) &&
    entry.dut_raised_flags == entry.ref_raised_flags &&
    entry.scale_snapshot.valid == (entry.ref_raised_flags == 0) &&
    entry.scale_snapshot.kVectors == 1 &&
    entry.ref_expected_tr.size() == packed.size() && mx_load_coverage_complete(entry) &&
    equal_scale_snapshots(entry.scale_snapshot, entry.ref_scale_snapshots[side], true) &&
    copy_active_rows(packed.data(), packed.size(), entry.res,
      CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES, 8, 32, CONFIG_DIFF_AMU_TRLEN / 8) &&
    packed == entry.ref_expected_tr;
}

int AmuExecChecker::do_step() {
  Difftest *dt = difftest[state->coreid];
  MmaVerifier *mma_verifier = dt->get_mma_verifier();
  // For each amu ctrl in sw_rob, check whether the inst is able to be committed.
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end();) {
    if (iter->state == DiffState::WAIT_SWROB_COMMIT) {
      DifftestAmuCtrlEvent amu_event = iter->amu_event;
      uint8_t op = amu_event.op;
      MmaVerificationBuffer *buffer = nullptr;
      switch (op) {
        case 0: { // MMA
          // Allocate buffer for MMA verification
          buffer = mma_verifier->allocate_buffer(&amu_event);
          // Store DUT result in the buffer
          bool valid;
          valid = copy_active_rows(buffer->dut_result, buffer->capacities[3], iter->res,
            CONFIG_DIFF_AMU_C_REG_SIZE_BYTES, amu_event.mtilem,
            size_t(amu_event.mtilen) * get_element_size(amu_event.typed), CONFIG_DIFF_AMU_ARLEN / 8);
          if (is_mma_mx(amu_event)) {
            RefAmuCtrlSnapshot scales{};
            valid = valid && iter->ref_snapshot_seen && iter->mma_owned_done;
            scales.epoch = amu_event.epoch;
            for (unsigned side = 0; side < 2; ++side) {
              const auto &ref = iter->ref_scale_snapshots[side];
              valid = valid && equal_scale_snapshots(iter->mma_scale_snapshots[side], ref);
              auto &scale = scales.scales[side];
              scale.bytes = iter->ref_scale_snapshots[side].bytes.data();
              scale.capacity = ref.bytes.size();
              scale.metadata = {ref.generation, ref.rows, ref.blocks, ref.rowPitch,
                                ref.regId, ref.side, uint8_t(ref.valid), 0, ref.sourceStride};
            }
            RefAmuMmaSnapshot snapshot{};
            snapshot.scales = &scales;
            uint8_t *views[] = {buffer->src1, buffer->src2, buffer->src3, buffer->expected_result};
            for (unsigned i = 0; i < 4; ++i) {
              snapshot.matrices[i] = {views[i], buffer->capacities[i], buffer->row_pitches[i], 0};
            }
            // Only REF-owned frozen scales enter REF. DUT bytes are compared
            // independently above and are never passed as expected C.
            valid = valid && proxy->get_amu_lazy_snapshot(&amu_event, &snapshot) == 0;
            valid = valid && snapshot.raised_flags <= 31;
            buffer->dut_raised_flags = iter->dut_raised_flags;
            buffer->ref_raised_flags = snapshot.raised_flags;
            buffer->mx_expected_valid = valid;
          } else if (valid) {
            // Legacy API copies full physical TLEN/ALEN. Keep its numerical
            // algorithm, but never let it write into compact active buffers.
            std::vector<uint8_t> a(CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES),
                                 b(CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES),
                                 c(CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
            valid = proxy->get_amu_lazy(&amu_event, iter->res, a.data(), b.data(), c.data()) == 0;
            valid = valid && copy_active_rows(buffer->src1, buffer->capacities[0], a.data(), a.size(),
              amu_event.mtilem, buffer->row_pitches[0], CONFIG_DIFF_AMU_TRLEN / 8);
            valid = valid && copy_active_rows(buffer->src2, buffer->capacities[1], b.data(), b.size(),
              amu_event.mtilen, buffer->row_pitches[1], CONFIG_DIFF_AMU_TRLEN / 8);
            valid = valid && copy_active_rows(buffer->src3, buffer->capacities[2], c.data(), c.size(),
              amu_event.mtilem, buffer->row_pitches[2], CONFIG_DIFF_AMU_ARLEN / 8);
          }
          if (!valid) {
            printf("Failed to get REF operands for MMA verification: core %d, pc 0x%016lx\n", state->coreid,
                   amu_event.pc);
            mma_verifier->free_buffer(buffer);
            delete[] iter->res;
            iter->res = nullptr;
            return STATE_ERROR;
          }
          // Pass buffer to verification thread
          mma_verifier->add_to_verification_queue(buffer);
          delete[] iter->res;
          break;
        }
        case 1: // MLS
          if (is_dynamic_mxfp_load(amu_event)) {
            // Both expected destinations and local flags were frozen at REF
            // enqueue. Never re-execute against younger-mutated global mtr.
            if (!compare_dynamic_mxfp_result(*iter)) {
              Info("Independent dynamic TR/SF/local-flags snapshot mismatch\n");
              return STATE_ERROR;
            }
            const unsigned side = amu_event.types2 ? 0 : 1;
            auto &ref = iter->ref_scale_snapshots[side];
            RefAmuCtrlSnapshot expected{};
            expected.epoch = ref.epoch;
            expected.raised_flags = iter->ref_raised_flags;
            expected.expected_tr = {iter->ref_expected_tr.data(), iter->ref_expected_tr.size(), 32, 0};
            expected.scales[side].bytes = ref.bytes.data();
            expected.scales[side].capacity = ref.bytes.size();
            expected.scales[side].metadata = {ref.generation, ref.rows, ref.blocks, ref.rowPitch,
                                              ref.regId, ref.side, uint8_t(ref.valid), 0, ref.sourceStride};
            RefAmuMmaSnapshot apply{};
            apply.scales = &expected;
            // Apply retained REF bytes in SWROB order, never DUT result bytes.
            if (proxy->get_amu_lazy_snapshot(&amu_event, &apply) != 0) {
              Info("Independent dynamic REF ordered apply rejected\n");
              return STATE_ERROR;
            }
            delete[] iter->res;
            iter->res = nullptr;
            break;
          }
          if (amu_event.isScale) {
            const unsigned side = amu_event.types2 ? 0 : 1;
            if (!iter->ref_snapshot_seen ||
                !equal_scale_snapshots(iter->scale_snapshot, iter->ref_scale_snapshots[side])) {
              Info("Independent DUT/REF scale snapshot mismatch\n");
              return STATE_ERROR;
            }
            break;
          }
          [[fallthrough]];
        case 2: // MRelease
        case 3: // Arith
          if (proxy->get_amu_exec(&amu_event, iter->res) != 0) {
            printf("Mismatch for amu exec event: pc 0x%016lx, op %s\n", amu_event.pc, amu_ctrl_op_name(amu_event.op));
            if (iter->res != nullptr) {
              delete[] iter->res;
              iter->res = nullptr;
            }
            return STATE_ERROR;
          }
          if (iter->res != nullptr) {
            delete[] iter->res;
            iter->res = nullptr;
          }
          break;
        default:
          printf("Unknown amu event op: %d\n", op);
          if (iter->res != nullptr) {
            delete[] iter->res;
            iter->res = nullptr;
          }
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

int MsyncChecker::do_step() {
  while (!state->msync_event_queue.empty()) {
    const DifftestMsyncEvent dut_event = state->msync_event_queue.front();
    DifftestMsyncEvent ref_event = dut_event;
#ifdef CONFIG_DIFFTEST_SQUASH
    // TODO: What is squash? How to squash?
#endif // CONFIG_DIFFTEST_SQUASH
    int check_res = proxy->get_msync_event(&ref_event);

    if (check_res == 1) {
      // Compare the msync event info.
      // For mismatch, proxy returns 1 and sets the msync event info.
      // Use global difftest pointer to access the owning Difftest instance for this core
      printf("\n==============  Msync Event (Core %d)  ==============\n", state->coreid);
      printf("Mismatch for Msync event\n");
      print_msync_event("REF", ref_event);
      print_msync_event("DUT", dut_event);
      state->msync_event_queue.pop();
      return STATE_ERROR;
    } else if (check_res == -1) {
      printf("\n==============  Msync Event (Core %d)  ==============\n", state->coreid);
      printf("  No available REF Msync events\n");
      print_msync_event("DUT", dut_event);
      state->msync_event_queue.pop();
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
