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
#include "xdma_sim.h"
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 65536

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t read_cond;
  bool read_waiting;
  int read_size;
  int write_size;
  char buffer[BUFFER_SIZE];
} xdma_shm;

class xdma_sim {
private:
  int shm_fd = -1;
  xdma_shm *shm_ptr = nullptr;
  char path[128];
  bool is_host;

public:
  xdma_sim(int channel, bool _is_host) {
    is_host = _is_host;
    sprintf(path, "/xdma_sim_c2h%d", channel);
    shm_fd = shm_open(path, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
      perror("XDMA_SIM: Failed to open shared memory device\n");
      exit(-1);
    }
    if (is_host) {
      ftruncate(shm_fd, sizeof(xdma_shm));
    }
    shm_ptr = (xdma_shm *)mmap(NULL, sizeof(xdma_shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (is_host) {
      memset(shm_ptr, 0, sizeof(xdma_shm));
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&shm_ptr->lock, &attr);
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
      pthread_cond_init(&shm_ptr->read_cond, &cattr);
    }
  }
  ~xdma_sim() {
    if (is_host) {
      shm_unlink(path);
    }
    munmap(shm_ptr, sizeof(xdma_shm));
    close(shm_fd);
  }
  int read(char *buf, size_t size) {
    assert(size <= BUFFER_SIZE);
    pthread_mutex_lock(&shm_ptr->lock);

    shm_ptr->read_waiting = true;
    shm_ptr->write_size = 0;
    shm_ptr->read_size = size;
    while (shm_ptr->write_size < size) {
      pthread_cond_wait(&shm_ptr->read_cond, &shm_ptr->lock);
    }
    size_t to_copy = size < shm_ptr->write_size ? size : shm_ptr->write_size;
    memcpy(buf, shm_ptr->buffer, to_copy);

    pthread_mutex_unlock(&shm_ptr->lock);

    return to_copy;
  }
  int write(const char *buf, unsigned char tlast, size_t size) {
    pthread_mutex_lock(&shm_ptr->lock);
    while (!shm_ptr->read_waiting) {
      pthread_mutex_unlock(&shm_ptr->lock);
      pthread_mutex_lock(&shm_ptr->lock);
    }
    size_t space = shm_ptr->read_size - shm_ptr->write_size;
    size_t to_write = size < space ? size : space;

    memcpy(shm_ptr->buffer + shm_ptr->write_size, buf, to_write);
    shm_ptr->write_size += to_write;
    if (shm_ptr->write_size >= shm_ptr->read_size) {
      assert(tlast == 1); // Check if tlast set properly
      shm_ptr->read_waiting = false;
      pthread_cond_signal(&shm_ptr->read_cond);
    } else {
      assert(tlast == 0);
    }
    pthread_mutex_unlock(&shm_ptr->lock);

    return to_write;
  }
};

// API for shared XDMA Dev
xdma_sim *xsim[8] = {nullptr};

void xdma_sim_open(int channel, bool is_host) {
  xsim[channel] = new xdma_sim(channel, is_host);
}

void xdma_sim_close(int channel) {
  delete xsim[channel];
  xsim[channel] = nullptr;
}

int xdma_sim_read(int channel, char *buf, size_t size) {
  return xsim[channel]->read(buf, size);
}

int xdma_sim_write(int channel, const char *buf, uint8_t tlast, size_t size) {
  return xsim[channel]->write(buf, tlast, size);
}

extern "C" unsigned char v_xdma_tready() {
  return 1;
}

extern "C" void v_xdma_write(uint8_t channel, const char *axi_tdata, uint8_t axi_tlast) {
  xdma_sim_write(channel, axi_tdata, axi_tlast, 64);
}
