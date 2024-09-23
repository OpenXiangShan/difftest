/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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
#include "xdma.h"
#include "mpool.h"
#include <fcntl.h>
#include <iostream>
#include <signal.h>

#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"
static const int dma_channel = CONFIG_DMA_CHANNELS;

FpgaXdma::FpgaXdma() {
  signal(SIGINT, handle_sigint);
  for (int i = 0; i < dma_channel; i++) {
    char c2h_device[64];
    sprintf(c2h_device, "%s%d", XDMA_C2H_DEVICE, i);
    xdma_c2h_fd[i] = open(c2h_device, O_RDONLY);
    if (xdma_c2h_fd[i] == -1) {
      std::cout << c2h_device << std::endl;
      perror("Failed to open XDMA device");
      exit(-1);
    }
    std::cout << "XDMA link " << c2h_device << std::endl;
  }

  xdma_h2c_fd = open(XDMA_H2C_DEVICE, O_WRONLY);
  if (xdma_h2c_fd == -1) {
    std::cout << XDMA_H2C_DEVICE << std::endl;
    perror("Failed to open XDMA device");
    exit(-1);
  }
  std::cout << "XDMA link " << XDMA_H2C_DEVICE << std::endl;
}

void FpgaXdma::handle_sigint(int sig) {
  printf("Unlink sem success, exit success!\n");
  exit(1);
}

void FpgaXdma::start_transmit_thread() {
  if (running == true)
    return;

  for (int i = 0; i < dma_channel; i++) {
    printf("start channel %d \n", i);
    receive_thread[i] = std::thread(&FpgaXdma::read_xdma_thread, this, i);
  }
  process_thread = std::thread(&FpgaXdma::write_difftest_thread, this);
  running = true;
}

void FpgaXdma::stop_thansmit_thread() {
  if (running == false)
    return;
  running = false;

  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    if (receive_thread[i].joinable())
      receive_thread[i].join();
    close(xdma_c2h_fd[i]);
  }

  if (process_thread.joinable())
    process_thread.join();

  close(xdma_h2c_fd);
  xdma_mempool.cleanupMemoryPool();
}

void FpgaXdma::read_xdma_thread(int channel) {
  FpgaPackgeHead packge;
  bool result = true;
  while (running) {
    size_t size = read(xdma_c2h_fd[channel], &packge, sizeof(FpgaPackgeHead));
    uint8_t idx = packge.packge_idx;
    if (xdma_mempool.write_free_chunk(idx, (char *)&packge) == false) {
      printf("It should not be the case that no available block can be found\n");
      assert(0);
    }
  }
}

void FpgaXdma::write_difftest_thread() {
  FpgaPackgeHead packge;
  bool result = true;
  while (running) {
    if (xdma_mempool.read_busy_chunk((char *)&packge) == false) {
      printf("Failed to read data from the XDMA memory pool\n");
      assert(0);
    }
    // packge unpack

    // difftest run
  }
}