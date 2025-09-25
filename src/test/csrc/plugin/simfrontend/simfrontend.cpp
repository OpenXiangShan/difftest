/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
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

#include "simfrontend.h"
#include "debug.h"
#include "ftq.h"
#include "tracereader.h"
#include <assert.h>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <stdio.h>
#include <sys/types.h>
#include <vector>

uint64_t debug_line_number = 0;

bool get_debug_flag() {
  return true;
}

std::vector<std::string_view> debug_read_line_vector;

MmapLineReader trace_fetch;
ContinuityChecker PcChecker;
FetchTargetQueue ftq;
uint32_t tick_fetch_count = 0;

bool TryFetchNextLine(uint32_t read_count) {
  if (trace_fetch.is_final)
    return false;

  std::string_view line;
  for (uint32_t i = 0; i < read_count; ++i) {
    if (ftq.is_full()) {
      DEBUG_INFO(std::cout << "FTQ Full" << std::endl;)

      return false;
    }

    if (!trace_fetch.get_next_line(line)) {
      ftq.set_final();
      return false;
    }
    debug_line_number = trace_fetch.get_line_read_count();

    uint64_t current_pc;
    uint32_t current_instr;

    parse_line(line, current_pc, current_instr);

    bool is_discontinuous = PcChecker.is_discontinuous(current_pc, current_instr);
    uint64_t next_pc = current_pc;
    uint32_t next_instr = current_instr;
    std::string_view next_line;

    if (trace_fetch.probe_next_line(next_line)) {
      parse_line(next_line, next_pc, next_instr);
    };

    if (!ftq.enqueue(current_pc, current_instr, next_pc, next_instr, is_discontinuous))
      assert(false && "ftq full");
  }

  return false;
}

void SimFrontFetch(int offset, uint64_t *pc, uint32_t *instr, uint32_t *preDecode) {
  auto entry_option = ftq.peek_offset(offset);
  bool allow_fetch = false;
  if (entry_option.has_value()) {
    allow_fetch = (entry_option->fetch_group_id < ftq.get_id_read_ptr()) ^
                  (entry_option->fetch_group_id_wrap ^ ftq.get_id_read_wrap());
  }

  if (allow_fetch) {
    auto data = unpack_trace_data(entry_option->packed_data);
    auto entry = entry_option.value();
    *pc = entry.pc;
    *instr = entry.instr;
    *preDecode = entry.packed_data;

    DEBUG_INFO(std::cout << std::hex << "Fetch Line: 0x" << entry.line_number << ", 0x" << entry.pc << ": 0x"
                         << entry.instr << " ftq: 0x" << entry.fetch_group_id << ", " << entry.fetch_group_id_wrap
                         << std::dec << std::endl;)

    tick_fetch_count++;
  } else {
    *pc = 0;
    *instr = 0;
    *preDecode = 0;
  }

  return;
}

void SimFrontUpdatePtr(uint32_t updateCount) {
  if (updateCount != 0) {
    ftq.set_read_ptr_offset(tick_fetch_count);
  }
  tick_fetch_count = 0;
}

void SimFrontRedirect(uint32_t redirect_valid, uint32_t redirect_ftq_flag, uint32_t redirect_ftq_value,
                      uint32_t redirect_type, uint64_t redirect_pc, uint64_t redirect_target) {
  if (redirect_valid) {

    DEBUG_INFO(std::cout << std::hex << "Redirect start: ftq: 0x" << redirect_ftq_value << ", " << redirect_ftq_value
                         << " pc: 0x" << redirect_target << std::dec << std::endl;)

    bool rx = ftq.redirect(redirect_ftq_value, redirect_ftq_flag, redirect_type, redirect_pc, redirect_target);
    if (rx) {

      DEBUG_INFO(std::cout << std::hex << "Redirect finish: ftq: 0x" << ftq.get_read_ftq_id() << ", "
                           << ftq.get_read_wrap() << std::dec << std::endl;)

    } else {
      DEBUG_INFO(std::cout << std::hex << "Redirect failed ftq: 0x" << ftq.get_read_ftq_id() << std::dec << std::endl;)
    }
  }
}

void SimFrontGetFtqToBackEnd(uint64_t *pc, uint32_t *pack_data, uint64_t *newest_pc, uint32_t *newest_pack_data) {
  *pc = 0;
  *pack_data = 0;

  *newest_pc = 0;
  *newest_pack_data = 0;
  size_t ftq_id = ftq.get_read_ftq_id();
  auto ftq_info = ftq.read_id_group();
  bool ftq_read_empty = ftq_info.has_value() == false;
  bool ftq_full = ftq.is_full();

  if (ftq_read_empty) {
    if (trace_fetch.is_final) {
      ftq.set_final();
    }
    if (trace_fetch.is_final || ftq_full) {
      return;
    }

    TryFetchNextLine(32);
    ftq_info = ftq.read_id_group();
  }

  auto entry_opt = ftq.get_physical_entry(ftq_info->start_index);

  DEBUG_INFO(std::cout << std::hex << "Ftq to backend current pc: 0x" << entry_opt->pc << " ftq: 0x" << ftq_id
                       << " phy_id: " << ftq_info->start_index << std::dec << std::endl;)

  *pc = entry_opt->pc;
  *pack_data = (1ULL << 6) | ftq_id;

  size_t next_id = ftq.get_read_ftq_id();
  auto newest_ftq_info = ftq.peek_id_group(0);
  if (!newest_ftq_info.has_value()) {
    if (trace_fetch.is_final) {
      return;
    }

    std::string_view line;
    uint64_t current_pc;
    uint32_t current_instr;

    if (!trace_fetch.probe_next_line(line))
      return;
    parse_line(line, current_pc, current_instr);

    *newest_pc = current_pc;
    *newest_pack_data = (1ULL << 6) | ftq_id;
  } else {

    auto newest_entry_opt = ftq.get_physical_entry(newest_ftq_info->start_index);

    *newest_pc = newest_entry_opt->pc;
    *newest_pack_data = (1ULL << 6) | ftq_id;
  }

  DEBUG_INFO(std::cout << std::hex << "Ftq to backend next pc: 0x" << *newest_pc << " ftq: 0x" << next_id
                       << " phy_id: " << newest_ftq_info->start_index << std::dec << std::endl;)
}

void SimFrontRobCommit(uint32_t valid, uint32_t ftqIdxFlag, uint32_t ftqIdxValue) {
  if (valid) {

    DEBUG_INFO(std::cout << std::hex << "Commit start: ftqidexValue: 0x" << ftqIdxValue << ", " << ftqIdxFlag
                         << " current ftq: 0x" << ftq.get_read_ftq_id() << ", " << ftq.get_read_wrap() << std::dec
                         << std::endl;)

    ftq.commit_to_right_open(ftqIdxValue, ftqIdxFlag);

    DEBUG_INFO(std::cout << std::hex << "Commit finish: " << " current ftq: 0x" << ftq.get_read_ftq_id() << ", "
                         << ftq.get_read_wrap() << std::dec << std::endl;)
  }
}

bool init_sim_frontend(const std::string &file) {
  trace_fetch.init(file);
  if (!trace_fetch.is_open) {
    std::cerr << "Failed to open file." << std::endl;
    assert(false && "Failed to open file.");
    return 1;
  }
  std::cout << "Open instrction trace: " << file << std::endl;

  return true;
}
