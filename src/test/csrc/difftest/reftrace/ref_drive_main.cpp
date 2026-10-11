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

#include "flash.h"
#include "fork_ref/ref_drive.h"
#include "ram.h"
#include "refdrive_trace.h"
#include <array>
#include <stdexcept>

static uint64_t read_word(DiffTrace<uint64_t> &trace) {
  uint64_t word;
  if (!trace.try_read_next(&word))
    throw std::runtime_error("Incomplete RefDriveTrace command or missing End");
  return word;
}

template <typename T> static T read_payload(DiffTrace<uint64_t> &trace) {
  std::array<uint64_t, (sizeof(T) + 7) / 8> words;
  for (auto &word: words)
    word = read_word(trace);
  T data{};
  memcpy(&data, words.data(), sizeof(T));
  return data;
}

static void finish(DiffTrace<uint64_t> &trace, bool initialized) {
  uint64_t word;
  if (trace.try_read_next(&word))
    throw std::runtime_error("Data after RefDriveTrace End");
  if (!initialized)
    throw std::runtime_error("RefDriveTrace contains no initialization");
}

static void drive(RefProxy &proxy, DiffTrace<uint64_t> &trace) {
  bool initialized = false;
  RefDriveExecutor executor(&proxy);
  for (;;) {
    if (signal_num)
      throw std::runtime_error("REF drive interrupted");
    const auto word = read_word(trace);
    const auto op = RefDriveTraceOp(word & 0xff);
    const auto arg = word >> 8;
    if (!initialized && op != RefDriveTraceOp::Init && op != RefDriveTraceOp::End)
      throw std::runtime_error("REF execution precedes initialization");
    RefDriveState state;
    auto &c = state.commands[0];
    state.num_commands = 1;
    switch (op) {
      case RefDriveTraceOp::Init: {
        if (arg || initialized)
          throw std::runtime_error("Invalid or repeated REF initialization");
        const auto regs = read_payload<DiffTestRegState>(trace);
        proxy.flash_init(reinterpret_cast<const uint8_t *>(flash_dev.base), flash_dev.img_size, flash_dev.img_path);
        simMemory->clone_on_demand(
            [&](uint64_t offset, void *src, size_t n) { proxy.mem_init(PMEM_BASE + offset, src, n, DUT_TO_REF); },
            true);
        proxy.regcpy(&regs, FIRST_INST_ADDRESS);
        initialized = true;
        continue;
      }
      case RefDriveTraceOp::Exec:
        if (!arg || arg > REFDRIVE_BATCH_LIMIT)
          throw std::runtime_error("Invalid REF instruction batch");
        c.op = RefDriveOp::EXEC;
        c.instr_count = arg;
        break;
      case RefDriveTraceOp::Skip:
        if (!refdrive_valid_skip(arg))
          throw std::runtime_error("Invalid scalar skip flags");
        c.op = RefDriveOp::SKIP;
        c.skip = {
          arg & 6 ? read_word(trace) : 0, uint32_t(arg >> 3), bool(arg & 1), bool(arg & 2), bool(arg & 4), false};
        break;
#ifdef CONFIG_DIFFTEST_ARCHEVENT
      case RefDriveTraceOp::Interrupt: {
        const auto cause = arg & REFDRIVE_INTERRUPT_MASK;
        const auto flags = arg >> REFDRIVE_INTERRUPT_BITS;
        if (flags >> 4 || !cause)
          throw std::runtime_error("Invalid interrupt flags");
        c.op = RefDriveOp::ARCH_EVENT;
        c.event.interrupt = cause;
        c.event.nmi = flags & 1;
        c.event.virtual_inject = flags & 2;
        c.event.delegation = {bool(flags & 4), bool(flags & 8)};
        break;
      }
      case RefDriveTraceOp::Exception:
        if (arg > std::numeric_limits<decltype(DifftestArchEvent::exception)>::max())
          throw std::runtime_error("Invalid exception number");
        c.op = RefDriveOp::ARCH_EVENT;
        c.event.interrupt = 0;
        c.event.guided = ref_exception_is_guided(arg);
        if (c.event.guided) {
          auto &guide = c.event.guide;
          guide = {};
          guide.force_raise_exception = true;
          guide.exception_num = arg;
          guide.mtval = read_word(trace);
          guide.stval = read_word(trace);
#ifdef CONFIG_DIFFTEST_HCSRSTATE
          guide.mtval2 = read_word(trace);
          guide.htval = read_word(trace);
          guide.vstval = read_word(trace);
#endif
        }
        break;
#endif
#ifdef CONFIG_DIFFTEST_LRSCEVENT
      case RefDriveTraceOp::LrSc:
        if (arg > 1)
          throw std::runtime_error("Invalid LR/SC result");
        c.op = RefDriveOp::LRSC;
        c.lrsc = {arg};
        break;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
      case RefDriveTraceOp::InterruptPending:
        if (arg >> 10)
          throw std::runtime_error("Invalid interrupt pending flags");
        c.op = RefDriveOp::PENDING;
        c.pending = {bool(arg & 1),  bool(arg & 2),  bool(arg & 4),   bool(arg & 8),   bool(arg & 16),
                     bool(arg & 32), bool(arg & 64), bool(arg & 128), bool(arg & 256), bool(arg & 512)};
        break;
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
      case RefDriveTraceOp::Overflow:
        if (arg)
          throw std::runtime_error("Invalid overflow command");
        c.op = RefDriveOp::OVERFLOW;
        c.overflow = read_word(trace);
        break;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
      case RefDriveTraceOp::Aia:
        if (arg)
          throw std::runtime_error("Invalid AIA command");
        c.op = RefDriveOp::AIA;
        c.aia = read_payload<FromAIA>(trace);
        break;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
      case RefDriveTraceOp::Mflush:
        if (arg > 1)
          throw std::runtime_error("Invalid mflushpwr result");
        c.op = RefDriveOp::MFLUSH;
        c.mflush = arg;
        break;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
      case RefDriveTraceOp::CriticalError:
        if (arg > 1)
          throw std::runtime_error("Invalid critical-error result");
        c.op = RefDriveOp::CRITICAL;
        c.critical = arg;
        break;
#endif
      case RefDriveTraceOp::End:
        if (arg)
          throw std::runtime_error("Invalid End command");
        if (!executor.flush())
          throw std::runtime_error("REF stopped before completing the recorded instruction count");
        return finish(trace, initialized);
      default: throw std::runtime_error("Unknown RefDriveTrace opcode");
    }
    const auto result = executor.run(state);
    if (result == RefDriveExecutor::ERROR || (op == RefDriveTraceOp::Exec && !executor.flush()))
      throw std::runtime_error("REF stopped before completing the recorded instruction count");
    if (result == RefDriveExecutor::CRITICAL_MISMATCH)
      throw std::runtime_error("REF critical-error mismatch");
    if (result == RefDriveExecutor::CRITICAL_MATCH) {
      if (read_word(trace) != uint8_t(RefDriveTraceOp::End))
        throw std::runtime_error("Data after terminal critical error");
      return finish(trace, initialized);
    }
  }
}

int main(int argc, const char *argv[]) {
  try {
    common_set_locale();
    auto args = parse_args(argc, argv);
    if (!args.refdrive_trace_name || !args.refdrive_trace_is_read || !args.enable_diff || NUM_CORES != 1)
      throw std::runtime_error("Use ref-drive --load-refdrive-trace=DIR -i IMAGE -F BOOTROM --diff=REF.so");
    if (args.enable_fork || args.gcpt_restore || args.snapshot_path || args.image_as_footprints || args.cst_file ||
        args.copy_ram_offset)
      throw std::runtime_error("ref-drive requires the original startup image");
    if (args.max_instr != uint64_t(-1) || args.max_cycles != uint64_t(-1) || args.log_begin ||
        args.log_end != uint64_t(-1))
      throw std::runtime_error("ref-drive does not support live DUT run controls");
    common_init(argv[0]);
    init_ram(args.image, args.ram_size ? parse_ramsize(args.ram_size) : DEFAULT_EMU_RAM_SIZE, args.random_mem,
             args.seed);
    init_flash(args.flash_bin);
    {
      REF_PROXY proxy(0, simMemory->get_size());
      if (!proxy.require_exec_mode_interfaces(false))
        throw std::runtime_error("REF drive requires difftest_set_exec_mode/get_instr_count");
      proxy.ref_set_exec_mode(REF_EXEC_FAST);
      proxy.set_debug(args.enable_ref_trace);
      DiffTrace<uint64_t> trace(args.refdrive_trace_name, true, 4096);
      drive(proxy, trace);
      proxy.sync();
      printf("REF_DRIVE_COMPLETE PC=0x%lx\n", proxy.state.pc);
      proxy.ref_reg_display();
    }
    flash_finish();
    delete simMemory;
    simMemory = nullptr;
    common_finish();
    return 0;
  } catch (const std::exception &error) {
    fprintf(stderr, "RefDriveTrace drive failed: %s\n", error.what());
    return 1;
  }
}
