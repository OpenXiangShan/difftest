#include "fpga_sim.h"

#ifdef FPGA_TRACEINST
#include "../trace_reader.h"
#include <tracertl_dut_info.h>
#endif

#include <iostream>
#include <unistd.h>
#include <queue>
#include <mutex>

#ifdef FPGA_TRACEINST
std::queue<TraceFpgaInstruction> fpga_sim_pending_insts;
std::mutex fpga_sim_pending_insts_lock;
#endif

bool fpga_sim_write(char *buf, size_t size) {
// #ifdef FPGA_TRACEINST
//   TraceFpgaInstruction *fpga_buf = (TraceFpgaInstruction *)buf;
//   std::lock_guard<std::mutex> lock(fpga_sim_pending_insts_lock);
//   for (size_t i = 0; i < size / sizeof(TraceFpgaInstruction); i++) {
//     if (fpga_buf[i].traceType != 4) {
//       fpga_sim_pending_insts.push(fpga_buf[i]);
//     } else {
//       printf("fpga_sim write, pending insts push %lx\n", fpga_buf[i].pcVA);
//       fflush(stdout);
//     }
//   }
// #else

// #endif
  return true;
}
bool fpga_sim_read(char *buf, size_t size) {

// #ifdef FPGA_TRACEINST
//   TraceFpgaCollectStruct *fpga_buf = (TraceFpgaCollectStruct *)buf;
//   size_t inst_num = size / sizeof(TraceFpgaCollectStruct);
//   while (fpga_sim_pending_insts.size() < inst_num) { usleep(100); }
//   std::lock_guard<std::mutex> lock(fpga_sim_pending_insts_lock);
//   for (size_t i = 0; i < inst_num; i++) {
//     fpga_buf[i].pcVA = fpga_sim_pending_insts.front().pcVA;
//     fpga_buf[i].instNum = 1;
//     fpga_sim_pending_insts.pop();
//   }
// #else

// #endif // FPGA_TRACEINST
  return true;
};