#include "xdma_sim.h"
#include <thread>

extern "C" void ila_window_test_initialize() {
  xdma_sim_open(0, false);
  std::thread([] {
    xdma_sim_workload_open(false);
    char workload[4096];
    if (xdma_sim_wait_workload(workload, sizeof(workload)) == 1)
      xdma_sim_complete_workload();
  }).detach();
}
