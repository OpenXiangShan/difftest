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

FpgaXdma::FpgaXdma() {
  signal(SIGINT, handle_sigint);
  fd_c2h = open("/dev/xdma0_c2h_0", O_RDWR);
  set_dma_fd_block();
}

void FpgaXdma::handle_sigint(int sig) {
  printf("Unlink sem success, exit success!\n");
  exit(1);
}

void FpgaXdma::set_dma_fd_block() {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl get error");
    return;
  }
  // Clear the O NONBLOCK flag and set it to blocking mode
  flags &= ~O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    perror("fcntl set error");
    return;
  }
}

void FpgaXdma::thread_read_xdma() {
  while (running) {
    char *memory = memory_pool.get_free_chunk();
    read(fd_c2h, memory, recv_size);
    memory_pool.set_busy_chunk();
  }
}

void FpgaXdma::write_difftest_thread() {
  while (running) {
    const char *memory = memory_pool.get_busy_chunk();
    memcpy(&diffteststate, memory, sizeof(diffteststate));

    stream_receiver_cout ++;
    memory_pool.set_free_chunk();

// Notify difftest to run the next beat
  

  }
}
