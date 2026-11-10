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

#include "checkers.h"
#include "flash.h"
#include "fork_ref/ref_drive.h"
#include "ram.h"
#include "reftrace/refdrive_trace.h"
#include <stdexcept>

static void drive(RefProxy &proxy, DiffTrace<RefDriveTraceEntry> &trace) {
  bool initialized = false;
  DiffState state(0);
  RefDriveExecutor executor(&proxy, &state);
  for (;;) {
    if (signal_num)
      throw std::runtime_error("REF drive interrupted");
    RefDriveTraceEntry entry;
    trace.read_next(&entry);
    if (!initialized) {
      FirstInstrCommitChecker::init_ref(proxy, entry.state.regs);
      initialized = true;
    }
    const auto result = executor.run(entry.state);
    if (result.status == RefDriveExecutor::ERROR)
      throw std::runtime_error("REF stopped before completing the recorded instruction count");
    if (result.status == RefDriveExecutor::TRAP)
      throw std::runtime_error("REF checker terminated before the trace endpoint");
    if (result.status == RefDriveExecutor::CRITICAL_MISMATCH)
      throw std::runtime_error("REF critical-error mismatch");
    if (entry.terminal) {
      if (!executor.flush())
        throw std::runtime_error("REF stopped before completing the recorded instruction count");
      return;
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
      DiffTrace<RefDriveTraceEntry> trace(args.refdrive_trace_name, true, 4096);
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
