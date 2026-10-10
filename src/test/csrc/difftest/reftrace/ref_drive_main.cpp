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
#include "ram.h"
#include "refdrive_trace.h"
#include "refproxy.h"
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
  for (;;) {
    if (signal_num)
      throw std::runtime_error("REF drive interrupted");
    const auto word = read_word(trace);
    const auto op = RefDriveOp(word & 0xff);
    const auto arg = word >> 8;
    if (!initialized && (op == RefDriveOp::Exec || op == RefDriveOp::Skip
#ifdef CONFIG_DIFFTEST_ARCHEVENT
                         || op == RefDriveOp::Interrupt || op == RefDriveOp::Exception
#endif
                         ))
      throw std::runtime_error("REF execution precedes initialization");
    switch (op) {
      case RefDriveOp::Init: {
        if (arg || initialized)
          throw std::runtime_error("Invalid or repeated REF initialization");
        const auto regs = read_payload<DiffTestRegState>(trace);
        proxy.flash_init(reinterpret_cast<const uint8_t *>(flash_dev.base), flash_dev.img_size, flash_dev.img_path);
        simMemory->clone_on_demand(
            [&](uint64_t offset, void *src, size_t n) { proxy.mem_init(PMEM_BASE + offset, src, n, DUT_TO_REF); },
            true);
        proxy.regcpy(&regs, FIRST_INST_ADDRESS);
        initialized = true;
        break;
      }
      case RefDriveOp::Exec: {
        if (!arg || arg > REFDRIVE_BATCH_LIMIT)
          throw std::runtime_error("Invalid REF instruction batch");
        const auto before = proxy.ref_get_instr_count();
        proxy.ref_exec(arg);
        if (proxy.ref_get_instr_count() - before != arg)
          throw std::runtime_error("REF stopped before completing the recorded instruction count");
        break;
      }
      case RefDriveOp::Skip: {
        if (!refdrive_valid_skip(arg))
          throw std::runtime_error("Invalid scalar skip flags");
        const auto data = arg & 6 ? read_word(trace) : 0;
        proxy.skip_one(arg & 1, arg & 2, arg & 4, false, arg >> 3, data);
        break;
      }
#ifdef CONFIG_DIFFTEST_ARCHEVENT
      case RefDriveOp::Interrupt: {
        const auto cause = arg & REFDRIVE_INTERRUPT_MASK;
        const auto flags = arg >> REFDRIVE_INTERRUPT_BITS;
        if (flags >> 4 || !cause)
          throw std::runtime_error("Invalid interrupt flags");
        if (flags & 1)
          proxy.trigger_nmi(true);
        else if (flags & 2)
          proxy.virtual_interrupt_is_hvictl_inject(true);
        InterruptDelegate delegate{bool(flags & 4), bool(flags & 8)};
        proxy.intr_delegate(delegate);
        proxy.raise_intr(cause | (1ULL << 63));
        break;
      }
      case RefDriveOp::Exception:
        if (arg > std::numeric_limits<decltype(DifftestArchEvent::exception)>::max())
          throw std::runtime_error("Invalid exception number");
        if (refdrive_guided_exception(arg)) {
          ExecutionGuide guide{};
          guide.force_raise_exception = true;
          guide.exception_num = arg;
          guide.mtval = read_word(trace);
          guide.stval = read_word(trace);
#ifdef CONFIG_DIFFTEST_HCSRSTATE
          guide.mtval2 = read_word(trace);
          guide.htval = read_word(trace);
          guide.vstval = read_word(trace);
#endif
          proxy.guided_exec(guide);
        } else {
          proxy.ref_exec(1);
        }
        break;
#endif
#ifdef CONFIG_DIFFTEST_LRSCEVENT
      case RefDriveOp::LrSc: {
        if (arg > 1)
          throw std::runtime_error("Invalid LR/SC result");
        SyncState sync{arg};
        proxy.uarchstatus_sync(reinterpret_cast<uint64_t *>(&sync));
        break;
      }
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
      case RefDriveOp::InterruptPending: {
        if (arg >> 10)
          throw std::runtime_error("Invalid interrupt pending flags");
        NonRegInterruptPending ip{bool(arg & 1),  bool(arg & 2),  bool(arg & 4),   bool(arg & 8),   bool(arg & 16),
                                  bool(arg & 32), bool(arg & 64), bool(arg & 128), bool(arg & 256), bool(arg & 512)};
        proxy.non_reg_interrupt_pending(ip);
        break;
      }
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
      case RefDriveOp::Overflow:
        if (arg)
          throw std::runtime_error("Invalid overflow command");
        proxy.mhpmevent_overflow(read_word(trace));
        break;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
      case RefDriveOp::Aia: {
        if (arg)
          throw std::runtime_error("Invalid AIA command");
        auto aia = read_payload<FromAIA>(trace);
        proxy.sync_aia(aia);
        break;
      }
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
      case RefDriveOp::Mflush:
        if (arg > 1)
          throw std::runtime_error("Invalid mflushpwr result");
        proxy.sync_custom_mflushpwr(arg);
        break;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
      case RefDriveOp::CriticalError:
        if (arg > 1 || proxy.raise_critical_error() != bool(arg))
          throw std::runtime_error("REF critical-error mismatch");
        if (read_word(trace) != uint8_t(RefDriveOp::End))
          throw std::runtime_error("Data after terminal critical error");
        return finish(trace, initialized);
#endif
      case RefDriveOp::End:
        if (arg)
          throw std::runtime_error("Invalid End command");
        return finish(trace, initialized);
      default: throw std::runtime_error("Unknown RefDriveTrace opcode");
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
      if (!proxy.ref_set_exec_mode || !proxy.ref_get_instr_count)
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
