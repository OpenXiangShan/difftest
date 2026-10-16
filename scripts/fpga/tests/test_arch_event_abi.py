#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'src/test/csrc/difftest'
text = (SOURCE / 'difftest.cpp').read_text()
start = text.index('  num_commit = 0; // reset num_commit this cycle to 0', text.index('inline int Difftest::check_all()'))
end = text.index('#if defined(CONFIG_DIFFTEST_STOREEVENT)', start)
loop = text[start:end]

HARNESS = r'''
#include "arch_event.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

struct LegacyEvent { bool valid; };
struct OrderedEvent { bool valid; bool isLatter; };
struct Commit { bool valid; uint64_t pc; unsigned nFused; };
struct Checker {
  std::vector<std::string> *trace;
  std::string name;
  int rc = 0;
  int step() { trace->push_back(name); return rc; }
};
template <typename Event>
struct Harness {
  struct Dut { Event event{}; std::array<Commit, CONFIG_DIFF_COMMIT_WIDTH> commit{}; } data;
  Dut *dut = &data;
  struct Proxy { struct { uint64_t pc = 0x80000000; } state; } ref;
  Proxy *proxy = &ref;
  unsigned num_commit = 0;
  uint64_t dut_commit_batch_pc = 0, ref_commit_batch_pc = 0;
  bool pc_mismatch = false;
  std::vector<std::string> trace;
  Checker arch{&trace, "E"};
  Checker *arch_event_checker = &arch;
  std::array<Checker, CONFIG_DIFF_COMMIT_WIDTH> commits;
  std::array<Checker *, CONFIG_DIFF_COMMIT_WIDTH> instr_commit_checker;
  Harness() {
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
      data.commit[i] = {true, 0x80000000 + uint64_t(i) * 4, 0};
      commits[i] = {&trace, "C" + std::to_string(i)};
      instr_commit_checker[i] = &commits[i];
    }
  }
  int check() {
@LOOP@
    return 0;
  }
};
int main() {
  for (bool valid : {false, true}) {
    Harness<LegacyEvent> h;
    h.data.event.valid = valid;
    assert(h.check() == 0);
    if (valid) {
      assert((h.trace == std::vector<std::string>{"E"}));
      assert(h.num_commit == 0 && !h.data.commit[0].valid);
    } else {
      assert(h.trace.size() == CONFIG_DIFF_COMMIT_WIDTH);
      assert(h.num_commit == CONFIG_DIFF_COMMIT_WIDTH);
    }
  }
  for (bool latter : {false, true}) {
    Harness<OrderedEvent> h;
    h.data.event = {true, latter};
    h.data.commit[0].nFused = 2;
    assert(h.check() == 0);
    assert((h.trace == (latter ? std::vector<std::string>{"C0", "E"} : std::vector<std::string>{"E"})));
    assert(h.num_commit == (latter ? 3u : 0u));
    if (latter && CONFIG_DIFF_COMMIT_WIDTH > 1) assert(!h.data.commit[1].valid);
  }
  Harness<OrderedEvent> empty;
  empty.data.event = {true, true};
  empty.data.commit[0].valid = false;
  assert(empty.check() == 0 && empty.num_commit == 0);
  assert((empty.trace == std::vector<std::string>{"E"}));
  Harness<OrderedEvent> error;
  error.data.event = {true, true};
  error.commits[0].rc = 9;
  assert(error.check() == 9 && error.trace.size() == 1);
  Harness<LegacyEvent> event_error;
  event_error.data.event.valid = true;
  event_error.arch.rc = 7;
  assert(event_error.check() == 7 && event_error.trace.size() == 1);
}
'''

with tempfile.TemporaryDirectory(prefix='arch-event-abi-') as temp:
    root = Path(temp)
    cpp = root / 'test.cpp'
    cpp.write_text(HARNESS.replace('@LOOP@', loop))
    for width in (1, 2, 4):
        for basic in (False, True):
            binary = root / ('event-' + str(width) + '-' + str(basic))
            command = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-O1', '-g',
                       '-fsanitize=undefined,bounds', '-fno-sanitize-recover=all', '-I' + str(SOURCE),
                       '-DCONFIG_DIFF_COMMIT_WIDTH=' + str(width), str(cpp), '-o', str(binary)]
            if basic:
                command.append('-DBASIC_DIFFTEST_ONLY')
            subprocess.run(command, check=True, timeout=30)
            subprocess.run([str(binary)], check=True, timeout=5)
print('ALL PASS: production commit loop, legacy/ordered ArchEvent ABI, widths 1/2/4, terminal event, fusion and checker errors')
