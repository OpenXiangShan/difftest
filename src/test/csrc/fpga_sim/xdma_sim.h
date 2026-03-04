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
#ifndef __XDMA_SIM_H__
#define __XDMA_SIM_H__
#include <stddef.h>
#include <stdint.h>

// C2H APIs
void xdma_c2h_sim_open(int channel, bool is_host);
void xdma_c2h_sim_close(int channel);
int xdma_c2h_sim_read(int channel, char *buf, size_t size);
int xdma_c2h_sim_write(int channel, const char *buf, uint8_t tlast, size_t size);

// H2C APIs
void xdma_h2c_sim_open(bool is_host);
void xdma_h2c_sim_close();
int xdma_h2c_write_ddr(const char *workload, size_t size);

// Config BAR APIs (for FPGA_SIM)
void xdma_config_bar_open(bool is_host);
void xdma_config_bar_close();
void xdma_config_bar_init();
void xdma_config_bar_write(uint32_t offset, uint32_t data, uint8_t strb);
uint32_t xdma_config_bar_read(uint32_t offset);

// Backward-compatible aliases
static inline void xdma_config_bar_sim_open(bool is_host) {
  xdma_config_bar_open(is_host);
}
static inline void xdma_config_bar_sim_close() {
  xdma_config_bar_close();
}

// Helper functions for H2C initialization
void xdma_h2c_init_sequence(uint32_t transfer_len);
void xdma_h2c_complete_sequence();

#endif // __XDMA_SIM_H__
