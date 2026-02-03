/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#if VM_TRACE == 1
#include "Vtb_top.h"
#include "common.h"
#include "verilated.h"
#if VM_TRACE_FST == 1
#include <verilated_fst_c.h>
#else
#include <verilated_vcd_c.h>
#endif // VM_TRACE_FST

int main(int argc, char **argv) {
  // Use global Verilated context APIs
  Verilated::commandArgs(argc, argv);

  bool dump_wave = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "+dump-wave") == 0) {
      dump_wave = true;
      break;
    }
  }

  // Enable tracing before instantiating the model
  Verilated::traceEverOn(true);

  // Construct the Verilated model
  Vtb_top *topp = new Vtb_top;

#if VM_TRACE_FST == 1
  VerilatedFstC *tfp = nullptr;
  const char *suffix = ".fst";
#else
  VerilatedVcdC *tfp = nullptr;
  const char *suffix = ".vcd";
#endif

  if (dump_wave) {
#if VM_TRACE_FST == 1
    tfp = new VerilatedFstC;
#else
    tfp = new VerilatedVcdC;
#endif
    topp->trace(tfp, 99);
    const char *wave_name = create_noop_filename(suffix);
    Info("dump wave to %s...\n", wave_name);
    tfp->open(wave_name);
  }

  // Simulate until $finish
  while (!Verilated::gotFinish()) {
    topp->eval();

    // Dump waveform at current simulation time
    if (tfp)
      tfp->dump(Verilated::time());

    // Advance time using event-driven API
    if (!topp->eventsPending())
      break;
    Verilated::time(topp->nextTimeSlot());
  }

  if (tfp) {
    tfp->close();
    delete tfp;
    tfp = nullptr;
  }

  // Execute 'final' processes
  topp->final();

  return 0;
}
#endif // VM_TRACE == 1
