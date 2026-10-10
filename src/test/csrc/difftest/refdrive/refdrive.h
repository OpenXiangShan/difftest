/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#ifndef DIFFTEST_REFDRIVE_H
#define DIFFTEST_REFDRIVE_H
#include "refproxy.h"
#ifdef CONFIG_DIFFTEST_FAST_REF
struct RefDriveState {
  enum Kind {
    EXEC,
    SKIP,
    LRSC,
    PENDING,
    OVERFLOW,
    CRITICAL,
    AIA,
    MFLUSH,
    ARCH_EVENT,
    END
  };
  struct Skip {
    uint64_t data;
    uint32_t dest;
    bool rvc, integer, floating, vector;
  };
  struct ArchEvent {
    uint64_t interrupt;
    bool nmi, virtual_inject;
    InterruptDelegate delegation;
    bool guided;
    ExecutionGuide guide;
  };
  struct Command {
    Kind kind;
    union {
      uint64_t count;
      Skip skip;
      SyncState lrsc;
      NonRegInterruptPending pending;
      uint64_t overflow;
      bool critical;
      FromAIA aia;
      bool mflush;
      ArchEvent event;
    };
  };
  static constexpr unsigned MAX_COMMANDS = CONFIG_DIFF_COMMIT_WIDTH * 2 + 8;
  Command commands[MAX_COMMANDS];
  unsigned count = 0;
  uint32_t committed = 0;
  bool arch_event = false;
  RefDriveState() = default;
  explicit RefDriveState(const DiffTestState &dut);

private:
  Command &append(Kind kind);
};

class RefDriveExecutor {
public:
  enum Result {
    OK,
    ERROR,
    CRITICAL_MATCH,
    CRITICAL_MISMATCH
  };
  explicit RefDriveExecutor(RefProxy *proxy) : proxy(proxy) {}
  Result run(const RefDriveState &state);
  bool flush();
  uint64_t completed() const {
    return executed;
  }

private:
  static constexpr uint64_t MAX_PENDING = 65536;
  RefProxy *proxy;
  uint64_t pending = 0;
  uint64_t executed = 0;
};
#endif
#endif
