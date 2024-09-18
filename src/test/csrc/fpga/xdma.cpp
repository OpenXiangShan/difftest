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
#include <signal.h>

#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"
static const int dma_channel = CONFIG_DMA_CHANNELS;

FpgaXdma::FpgaXdma() {
  signal(SIGINT, handle_sigint);
  for (int channel = 0; i < dma_channel; channel ++) {
    char c2h_device[64];
    sprintf(c2h_device,"%s%d",DEVICE_C2H_NAME,i); 
    xdma_c2h_fd[i] = open(c2h_device, O_RDONLY );
    if (xdma_c2h_fd[i] == -1) {
      std::cout << c2h_device << std::endl;
      perror("Failed to open XDMA device");
      exit(-1);
    }
    std::cout << "XDMA link " << c2h_device << std::endl;
  }

  xdma_h2c_fd[i] = open(h2c_device, O_WRONLY);
  if (xdma_h2c_fd[i] == -1) {
    std::cout << h2c_device << std::endl;
    perror("Failed to open XDMA device");
    exit(-1);
  }
  std::cout << "XDMA link " << h2c_device << std::endl;
}

void FpgaXdma::handle_sigint(int sig) {
  printf("Unlink sem success, exit success!\n");
  exit(1);
}

void FpgaXdma::start_transmit_thread() {
  if (running == true)
    return;

  for(int i = 0; i < dma_channel;i ++) {
    printf("start channel %d \n", i);
    receive_thread[i] = std::thread(&FpgaXdma::read_xdma_thread, this, i);
  }
  process_thread[i] = std::thread(&FpgaXdma::write_difftest_thread, this, i);
  running = true;
}

void FpgaXdma::stop_thansmit_thread() {
  if (running == false)
    return;
  xdma_mempool.unlock_thread();
  if (receive_thread.joinable())
    receive_thread.join();
  if (process_thread.joinable())
    process_thread.join();
  running = false;
}

void FpgaXdma::read_xdma_thread(int channel) {
  while (running) {
    char *memory = xdma_mempool.get_free_chunk();
    read(fd_c2h, memory, recv_size);
    xdma_mempool.set_busy_chunk();
  }
}

void FpgaXdma::write_difftest_thread() {
  while (running) {
    const char *memory = xdma_mempool.get_busy_chunk();
    static uint8_t valid_core = 0;
    uint8_t core_id = 0;

    memcpy(&core_id, memory + sizeof(DiffTestState), sizeof(uint8_t));
    assert(core_id > NUM_CORES);
    {
      std::unique_lock<std::mutex> lock(diff_mtx);
      diff_empile_cv.wait(lock, [this] { return !diff_packge_filled; });
      memcpy(&difftest_pack[core_id], memory, sizeof(DiffTestState));
    }
    valid_core++;
    xdma_mempool.set_free_chunk();

    if (valid_core == NUM_CORES) {
      diff_packge_filled = true;
      valid_core = 0;
      // Notify difftest to run the next check
      diff_filled_cv.notify_one();
    }
  }
}
