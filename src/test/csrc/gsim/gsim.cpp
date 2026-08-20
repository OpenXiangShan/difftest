/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "simulator.h"

extern "C" {
void globalSimNegedge();
uint64_t createInstMeta(uint64_t order_id, uint64_t pc, uint64_t instcode);
void updateInstPos(uint64_t sn, uint64_t uop_idx, int pos);
void updateInstMeta(uint64_t sn, uint64_t uop_idx, uint64_t meta, uint64_t data);
void commitInstMeta(uint64_t order_id, uint64_t sn, uint64_t block_size);

void InstTable0_write(
  uint64_t except_type, uint64_t replay_cnt,
  uint64_t cause_0, uint64_t cause_1, uint64_t cause_2, uint64_t cause_3,
  uint64_t cause_4, uint64_t cause_5, uint64_t cause_6, uint64_t cause_7,
  uint64_t cause_8, uint64_t cause_9, uint64_t cause_10, uint64_t cause_11,
  uint64_t cause_12, uint64_t s3_replay, uint64_t s3_replay_rs,
  uint64_t s3_replay_slow, uint64_t s3_replay_fast,
  uint64_t s2_forward_fail, uint64_t s2_dcache_first_miss,
  uint64_t s2_bank_conflict, uint64_t s1_load_to_load_forward,
  uint64_t s1_tlb_first_miss, uint64_t tlb_latency, uint64_t rs_fu_latency,
  uint64_t execute_latency, uint64_t issue_latency, uint64_t select_latency,
  uint64_t enq_rs_latency, uint64_t dispatch_latency, uint64_t writeback_time,
  uint64_t issue_time, uint64_t dpaddr, uint64_t dvaddr, uint64_t rob_idx,
  uint64_t stamp, char *site);
void LoadDebugTable0_write(
  uint64_t exe_latency, uint64_t tlb_query_latency, uint64_t cache_miss,
  uint64_t paddr, uint64_t vaddr, uint64_t pc, uint64_t stamp, char *site);
void TLLog_write(
  uint64_t echo, uint64_t user, uint64_t data_0, uint64_t data_1,
  uint64_t data_2, uint64_t data_3, uint64_t address, uint64_t sink,
  uint64_t source, uint64_t param, uint64_t opcode, uint64_t channel,
  uint64_t stamp, char *site);
void CHILog_write(
  uint64_t channel, uint64_t flit_0, uint64_t flit_1, uint64_t flit_2,
  uint64_t flit_3, uint64_t flit_4, uint64_t flit_5, uint64_t flit_6,
  uint64_t stamp, char *site);
}

// GSIM lowers inline Verilog black boxes to calls named after the modules. These
// bridges preserve the clock/reset guards and forward to the underlying DPI APIs.
void GlobalSimNegedge_DPIC_Helper(uint8_t reset, uint8_t en) {
  if (en && !reset) globalSimNegedge();
}

void CreateInstMeta_DPIC_Helper(
  uint8_t reset, uint8_t en, uint64_t &sn,
  uint64_t order_id, uint64_t pc, uint64_t instcode) {
  if (reset) sn = 0;
  else if (en) sn = createInstMeta(order_id, pc, instcode);
}

void UpdateInstPos_DPIC_Helper(
  uint8_t reset, uint8_t en, uint64_t sn, uint64_t uop_idx, uint64_t pos) {
  if (en && !reset) updateInstPos(sn, uop_idx, static_cast<int>(pos));
}

void UpdateInstMeta_DPIC_Helper(
  uint8_t reset, uint8_t en, uint64_t sn, uint64_t uop_idx,
  uint64_t meta, uint64_t data) {
  if (en && !reset) updateInstMeta(sn, uop_idx, meta, data);
}

void CommitInstMeta_DPIC_Helper(
  uint8_t reset, uint8_t en, uint64_t order_id,
  uint64_t sn_first, uint64_t block_num_insts) {
  if (en && !reset) commitInstMeta(order_id, sn_first, block_num_insts);
}

void TLLogWriter(
  const char *site, uint8_t reset, uint8_t en, uint64_t stamp,
  uint8_t channel, uint8_t opcode, uint8_t param, uint8_t source, uint8_t sink,
  uint64_t address, uint64_t data_0, uint64_t data_1, uint64_t data_2,
  uint64_t data_3, uint64_t user, uint64_t echo) {
  if (en && !reset) {
    TLLog_write(echo, user, data_0, data_1, data_2, data_3, address, sink,
                source, param, opcode, channel, stamp, const_cast<char *>(site));
  }
}

void LoadDebugTable0Writer(
  const char *site, uint8_t reset, uint8_t en, uint64_t stamp,
  uint64_t pc, uint64_t vaddr, uint64_t paddr, uint8_t cache_miss,
  uint64_t tlb_query_latency, uint64_t exe_latency) {
  if (en && !reset) {
    LoadDebugTable0_write(exe_latency, tlb_query_latency, cache_miss, paddr,
                          vaddr, pc, stamp, const_cast<char *>(site));
  }
}

void InstTable0Writer(
  const char *site, uint8_t reset, uint8_t en, uint64_t stamp,
  uint16_t rob_idx, uint64_t dvaddr, uint64_t dpaddr, uint64_t issue_time,
  uint64_t writeback_time, uint64_t dispatch_latency, uint64_t enq_rs_latency,
  uint64_t select_latency, uint64_t issue_latency, uint64_t execute_latency,
  uint64_t rs_fu_latency, uint64_t tlb_latency,
  uint8_t s1_tlb_first_miss, uint8_t s1_load_to_load_forward,
  uint8_t s2_bank_conflict, uint8_t s2_dcache_first_miss,
  uint8_t s2_forward_fail, uint8_t s3_replay_fast, uint8_t s3_replay_slow,
  uint8_t s3_replay_rs, uint8_t s3_replay,
  uint8_t cause_0, uint8_t cause_1, uint8_t cause_2, uint8_t cause_3,
  uint8_t cause_4, uint8_t cause_5, uint8_t cause_6, uint8_t cause_7,
  uint8_t cause_8, uint8_t cause_9, uint8_t cause_10, uint8_t cause_11,
  uint8_t cause_12, uint64_t replay_cnt, uint32_t except_type) {
  if (en && !reset) {
    InstTable0_write(
      except_type, replay_cnt,
      cause_0, cause_1, cause_2, cause_3, cause_4, cause_5, cause_6,
      cause_7, cause_8, cause_9, cause_10, cause_11, cause_12,
      s3_replay, s3_replay_rs, s3_replay_slow, s3_replay_fast,
      s2_forward_fail, s2_dcache_first_miss, s2_bank_conflict,
      s1_load_to_load_forward, s1_tlb_first_miss,
      tlb_latency, rs_fu_latency, execute_latency, issue_latency,
      select_latency, enq_rs_latency, dispatch_latency, writeback_time,
      issue_time, dpaddr, dvaddr, rob_idx, stamp, const_cast<char *>(site));
  }
}

void CHILogWriter(
  const char *site, uint8_t reset, uint8_t en, uint64_t stamp,
  unsigned _BitInt(448) flit, uint8_t channel) {
  if (en && !reset) {
    CHILog_write(
      channel,
      static_cast<uint64_t>(flit), static_cast<uint64_t>(flit >> 64),
      static_cast<uint64_t>(flit >> 128), static_cast<uint64_t>(flit >> 192),
      static_cast<uint64_t>(flit >> 256), static_cast<uint64_t>(flit >> 320),
      static_cast<uint64_t>(flit >> 384), stamp, const_cast<char *>(site));
  }
}

GsimSim::GsimSim() : dut(new SSimTop) {}

GsimSim::~GsimSim() {
  delete dut;
}
