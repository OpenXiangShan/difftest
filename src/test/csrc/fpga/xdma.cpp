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
#include "ram.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sys/mman.h>

#define XDMA_USER       "/dev/xdma0_user"
#define XDMA_BYPASS     "/dev/xdma0_bypass"
#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"

FpgaXdma::FpgaXdma(const char *workload) {
  signal(SIGINT, handle_sigint);
  ddr_load_workload(workload);

  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
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

// write xdma_bypass memory or xdma_user
void FpgaXdma::device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value) {
  uint64_t pg_size = sysconf(_SC_PAGE_SIZE);
  uint64_t size = !is_bypass ? 0x1000 : 0x10000;
  uint64_t aligned_size = (size + 0xffful) & ~0xffful;
  uint64_t base = addr & ~0xffful;
  uint32_t offset = addr & 0xfffu;
  int fd = -1;

  if (base % pg_size != 0) {
    printf("base must be a multiple of system page size\n");
    exit(-1);
  }

  if (is_bypass)
    fd = open(XDMA_BYPASS, O_RDWR | O_SYNC);
  else
    fd = open(XDMA_USER, O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("failed to open %s\n", is_bypass ? XDMA_BYPASS : XDMA_USER);
    exit(-1);
  }

  void *m_ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (m_ptr == MAP_FAILED) {
    close(fd);
    printf("failed to mmap\n");
    exit(-1);
  }

  if (is_bypass) {
    if (simMemory->get_load_img_size() > aligned_size) {
      printf("The loaded workload size exceeds the xdma bypass size");
      exit(-1);
    }
    memcpy(static_cast<char *>(m_ptr) + offset, static_cast<const void *>(simMemory->as_ptr()),
           simMemory->get_load_img_size());
  } else {
    ((volatile uint32_t *)m_ptr)[offset >> 2] = value;
  }

  munmap(m_ptr, aligned_size);
  close(fd);
}

void FpgaXdma::start_transmit_thread() {
  if (running == true)
    return;

  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
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
    v_difftest_Batch((uint8_t *)packge.diff_batch_pack);
    // difftest run
    diff_packge_count.fetch_add(1, std::memory_order_relaxed);
  }
}
