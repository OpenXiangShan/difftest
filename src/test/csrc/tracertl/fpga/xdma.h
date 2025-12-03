/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
#ifndef __XDMA_H__
#define __XDMA_H__

#include <stdint.h>

// #define CONFIG_DMA_CHANNELS 1 // from compile

#define XDMA_USER       "/dev/xdma0_user"
#define XDMA_BYPASS     "/dev/xdma0_bypass"
#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_"

class FpgaXdma {
public:
  FpgaXdma();

  void set_xdma(int channel, uint64_t addr, uint64_t value);

  size_t write_xdma(int channel, char *buf, size_t size);

  size_t read_xdma(int channel, char *buf, size_t size);

  void ddr_load_workload(const char *workload) {
    core_reset();
    device_write(true, workload, 0, 0);
    core_restart();
  }

  void fpga_reset_io(bool enable) {
    if (enable)
      device_write(false, nullptr, 0x0, 0x1);
    else
      device_write(false, nullptr, 0x0, 0x0);
  }

private:
  int xdma_h2c_fd[CONFIG_DMA_CHANNELS];
  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
  // int xdma_c2d_fd; // change to multiple channels

  void device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value);
  void core_reset() {
    device_write(false, nullptr, 0x20000, 0x1);
    device_write(false, nullptr, 0x100000, 0x1);
    device_write(false, nullptr, 0x10000, 0x8);
  }

  void core_restart() {
    device_write(false, nullptr, 0x20000, 0);
    device_write(false, nullptr, 0x100000, 0);
  }
};

#endif // __XDMA_H__