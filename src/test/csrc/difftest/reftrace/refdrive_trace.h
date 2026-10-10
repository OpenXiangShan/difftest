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

#ifndef __REFDRIVE_TRACE_H__
#define __REFDRIVE_TRACE_H__

#include "args.h"
#include "difftrace.h"
#include <limits>
#include <type_traits>

// Stable opcode IDs across profiles; one argument word plus the required payload.
enum class RefDriveOp : uint8_t {
  Init = 1,
  Exec = 2,
  Skip = 3,
#ifdef CONFIG_DIFFTEST_ARCHEVENT
  Interrupt = 4,
  Exception = 5,
#endif
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  LrSc = 6,
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  InterruptPending = 7,
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  Overflow = 8,
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  Aia = 9,
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  Mflush = 10,
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  CriticalError = 11,
#endif
  End = 12
};
constexpr uint64_t REFDRIVE_BATCH_LIMIT = 65536;

// Skip argument: isRVC/rfwen/fpwen in the low three bits, then the register index.
inline bool refdrive_valid_skip(uint64_t arg) {
  if ((arg >> 3) >= std::extent_v<decltype(DifftestArchIntRegState::value)>)
    return false;
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  return !(arg & 4) || (arg >> 3) < std::extent_v<decltype(DifftestArchFpRegState::value)>;
#else
  return !(arg & 4);
#endif
}

#ifdef CONFIG_DIFFTEST_ARCHEVENT
constexpr auto REFDRIVE_INTERRUPT_BITS = std::numeric_limits<decltype(DifftestArchEvent::interrupt)>::digits;
constexpr uint64_t REFDRIVE_INTERRUPT_MASK = std::numeric_limits<decltype(DifftestArchEvent::interrupt)>::max();
static_assert(REFDRIVE_INTERRUPT_BITS + 4 <= 56);

inline bool refdrive_guided_exception(uint64_t cause) {
  return cause == 12 || cause == 13 || cause == 15 || cause == 19 || cause == 20 || cause == 21 || cause == 23;
}
#endif

void difftest_refdrive_trace_configure(const CommonArgs &args);
void difftest_refdrive_trace_record(const DiffTestState &dut);
void difftest_refdrive_trace_finish();
void difftest_refdrive_trace_check_load_correction();

#endif
