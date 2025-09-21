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

#ifdef EMU_THREAD
#include <verilated_threads.h>
#endif

#include "simulator.h"

VerilatorSim::VerilatorSim() : dut(new VSimTop) {}

VerilatorSim::~VerilatorSim() {
#if VM_TRACE == 1
  if (waveform) {
    delete waveform;
    waveform = nullptr;
  }
#endif

#ifdef VM_SAVABLE
  if (snapshot_slot) {
    delete[] snapshot_slot;
    snapshot_slot = nullptr;
  }
#endif // VM_SAVABLE

  delete dut;
}

void VerilatorSim::atClone() {
#ifdef VERILATOR_VERSION_INTEGER // >= v4.220
#if VERILATOR_VERSION_INTEGER >= 5016000
  // This will cause 288 bytes leaked for each one fork call.
  // However, one million snapshots cause only 288MB leaks, which is still acceptable.
  // See verilator/test_regress/t/t_wrapper_clone.cpp:48 to avoid leaks.
  dut->atClone();
#else
#error Please use Verilator v5.016 or newer versions.
#endif                 // check VERILATOR_VERSION_INTEGER values
#elif EMU_THREAD > 1   // VERILATOR_VERSION_INTEGER not defined
#ifdef VERILATOR_4_210 // v4.210 <= version < 4.220
  dut->vlSymsp->__Vm_threadPoolp = new VlThreadPool(dut_ptr->contextp(), EMU_THREAD - 1, 0);
#else                  // older than v4.210
  dut->__Vm_threadPoolp = new VlThreadPool(dut_ptr->contextp(), EMU_THREAD - 1, 0);
#endif
#endif
}

#if VM_TRACE == 1
void VerilatorSim::waveform_init(uint64_t cycles) {
  waveform = new EmuWaveform(trace_bind, cycles);
}

void VerilatorSim::waveform_init(uint64_t cycles, const char *filename) {
  waveform = new EmuWaveform(trace_bind, cycles, filename);
}

void VerilatorSim::waveform_tick() {
  waveform->tick();
}
#endif // VM_TRACE == 1

#ifdef VM_SAVABLE
void VerilatorSim::snapshot_init() {
  snapshot_slot = new VerilatedSaveMem[2];
}

void VerilatorSim::snapshot_save(int index) {
  if (snapshot_slot) {
    if (index >= 0) {
      snapshot_slot[index].save();
    } else if (index == -1) {
      Info("Saving snapshots to file system. Please wait.\n");
      snapshot_slot[0].save();
      snapshot_slot[1].save();
      Info("Please remove unused snapshots manually\n");
    }
  }
}

std::function<void(void *, size_t)> VerilatorSim::snapshot_take() {
  static int last_slot = 0;
  VerilatedSaveMem &stream = snapshot_slot[last_slot];
  last_slot = !last_slot;

  stream.init(create_noop_filename(".snapshot"));
  stream << *dut;
  stream.flush();

  return [&stream](void *datap, size_t size) { stream.unbuf_write(datap, size); };
}

std::function<void(void *, size_t)> VerilatorSim::snapshot_load(const char *filename) {
  auto stream = std::make_shared<VerilatedRestoreMem>();
  stream->open(filename);
  *stream >> *dut;

  return [stream](void *datap, size_t size) { stream->read((uint8_t *)datap, size); };
}
#endif // VM_SAVABLE
