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
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef FPGA_HOST
#include "svdpi.h"
#endif // FPGA_HOST

#define BUFFER_SIZE        65536
#define WORKLOAD_PATH_SIZE 4096

template <typename T> class xdma_shm_device {
private:
  int shm_fd = -1;
  std::string shm_path;
  size_t shm_size;

protected:
  T *shm_ptr = nullptr;
  bool is_host;

public:
  xdma_shm_device(const char *path, size_t size, bool _is_host, const char *desc)
      : shm_path(path), shm_size(size), is_host(_is_host) {
    shm_fd = shm_open(shm_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
      fprintf(stderr, "XDMA_SIM: Failed to open %s shared memory %s\n", desc, shm_path.c_str());
      perror("shm_open");
      exit(-1);
    }
    if (is_host && ftruncate(shm_fd, shm_size) != 0) {
      perror("XDMA_SIM: Failed to size shared memory device");
      exit(-1);
    }
    shm_ptr = (T *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
      perror("XDMA_SIM: Failed to mmap shared memory");
      exit(-1);
    }
    if (is_host) {
      memset(shm_ptr, 0, shm_size);
    }
  }

  ~xdma_shm_device() {
    if (is_host) {
      shm_unlink(shm_path.c_str());
    }
    munmap(shm_ptr, shm_size);
    close(shm_fd);
  }
};

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t read_cond;
  pthread_cond_t write_cond;
  bool read_waiting;
  bool data_valid;
  uint64_t tkeep;
  uint8_t tlast;
  int read_size;
  int write_size;
  char buffer[BUFFER_SIZE];
} xdma_shm;

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t write_cond;
  pthread_cond_t done_cond;
  bool valid;
  bool done;
  bool is_read;
  uint32_t addr;
  uint32_t data;
  uint8_t strb;
  uint32_t read_data;
} xdma_axilite_shm;

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t accepted_cond;
  bool valid;
  bool accepted;
  bool closed;
  char workload[WORKLOAD_PATH_SIZE];
} xdma_workload_shm;

class xdma_sim : private xdma_shm_device<xdma_shm> {
private:
  static std::string path(int channel) {
    char path[128];
    sprintf(path, "/xdma_sim_c2h%d", channel);
    return path;
  }

public:
  xdma_sim(int channel, bool is_host) : xdma_shm_device(path(channel).c_str(), sizeof(xdma_shm), is_host, "C2H") {
    if (is_host) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&shm_ptr->lock, &attr);
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
      pthread_cond_init(&shm_ptr->read_cond, &cattr);
      pthread_cond_init(&shm_ptr->write_cond, &cattr);
    }
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

class xdma_h2c_sim : private xdma_shm_device<xdma_shm> {
private:
  static std::string path(int channel) {
    char path[128];
    sprintf(path, "/xdma_sim_h2c%d", channel);
    return path;
  }

public:
  xdma_h2c_sim(int channel, bool is_host) : xdma_shm_device(path(channel).c_str(), sizeof(xdma_shm), is_host, "H2C") {
    if (is_host) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&shm_ptr->lock, &attr);
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
      pthread_cond_init(&shm_ptr->read_cond, &cattr);
      pthread_cond_init(&shm_ptr->write_cond, &cattr);
    }
  }

  int write(const char *buf, uint64_t tkeep, unsigned char tlast, size_t size) {
    assert(size <= BUFFER_SIZE);
    pthread_mutex_lock(&shm_ptr->lock);
    while (shm_ptr->data_valid) {
      pthread_cond_wait(&shm_ptr->read_cond, &shm_ptr->lock);
    }
    memcpy(shm_ptr->buffer, buf, size);
    shm_ptr->write_size = size;
    shm_ptr->tkeep = tkeep;
    shm_ptr->tlast = tlast;
    shm_ptr->data_valid = true;
    pthread_cond_broadcast(&shm_ptr->write_cond);
    while (shm_ptr->data_valid) {
      pthread_cond_wait(&shm_ptr->read_cond, &shm_ptr->lock);
    }
    pthread_mutex_unlock(&shm_ptr->lock);
    return size;
  }

  bool wait(char *buf, uint64_t *tkeep, uint8_t *tlast, size_t size, volatile bool *stop) {
    assert(size <= BUFFER_SIZE);
    pthread_mutex_lock(&shm_ptr->lock);
    while (!shm_ptr->data_valid && !*stop) {
      pthread_cond_wait(&shm_ptr->write_cond, &shm_ptr->lock);
    }
    if (*stop) {
      pthread_mutex_unlock(&shm_ptr->lock);
      return false;
    }

    size_t to_copy = size < (size_t)shm_ptr->write_size ? size : (size_t)shm_ptr->write_size;
    memcpy(buf, shm_ptr->buffer, to_copy);
    *tkeep = shm_ptr->tkeep;
    *tlast = shm_ptr->tlast;
    shm_ptr->data_valid = false;
    pthread_cond_broadcast(&shm_ptr->read_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
    return to_copy == size;
  }

  void notify() {
    pthread_mutex_lock(&shm_ptr->lock);
    pthread_cond_broadcast(&shm_ptr->read_cond);
    pthread_cond_broadcast(&shm_ptr->write_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
  }
};

class xdma_axilite_sim : private xdma_shm_device<xdma_axilite_shm> {
private:
  static const char *path() {
    return "/xdma_sim_axilite";
  }

public:
  xdma_axilite_sim(bool is_host) : xdma_shm_device(path(), sizeof(xdma_axilite_shm), is_host, "AXI-Lite") {
    if (is_host) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&shm_ptr->lock, &attr);
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
      pthread_cond_init(&shm_ptr->write_cond, &cattr);
      pthread_cond_init(&shm_ptr->done_cond, &cattr);
    }
  }

  int write(uint32_t addr, uint32_t data, uint8_t strb) {
    return request(false, addr, data, strb, nullptr);
  }

  int read(uint32_t addr, uint32_t *data) {
    return request(true, addr, 0, 0, data);
  }

  int request(bool is_read, uint32_t addr, uint32_t data, uint8_t strb, uint32_t *read_data) {
    pthread_mutex_lock(&shm_ptr->lock);
    while (shm_ptr->valid) {
      pthread_cond_wait(&shm_ptr->done_cond, &shm_ptr->lock);
    }

    shm_ptr->is_read = is_read;
    shm_ptr->addr = addr;
    shm_ptr->data = data;
    shm_ptr->strb = strb;
    shm_ptr->done = false;
    shm_ptr->valid = true;
    pthread_cond_signal(&shm_ptr->write_cond);

    while (!shm_ptr->done) {
      pthread_cond_wait(&shm_ptr->done_cond, &shm_ptr->lock);
    }
    if (read_data != nullptr) {
      *read_data = shm_ptr->read_data;
    }
    shm_ptr->valid = false;
    pthread_cond_broadcast(&shm_ptr->done_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
    return 0;
  }

  int wait(bool *is_read, uint32_t *addr, uint32_t *data, uint8_t *strb, volatile bool *stop) {
    pthread_mutex_lock(&shm_ptr->lock);
    while ((!shm_ptr->valid || shm_ptr->done) && !*stop) {
      pthread_cond_wait(&shm_ptr->write_cond, &shm_ptr->lock);
    }
    if (*stop) {
      pthread_mutex_unlock(&shm_ptr->lock);
      return 0;
    }

    *is_read = shm_ptr->is_read;
    *addr = shm_ptr->addr;
    *data = shm_ptr->data;
    *strb = shm_ptr->strb;
    pthread_mutex_unlock(&shm_ptr->lock);
    return 1;
  }

  void complete(uint32_t read_data = 0) {
    pthread_mutex_lock(&shm_ptr->lock);
    shm_ptr->read_data = read_data;
    shm_ptr->done = true;
    pthread_cond_broadcast(&shm_ptr->done_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
  }

  void notify() {
    pthread_mutex_lock(&shm_ptr->lock);
    pthread_cond_signal(&shm_ptr->write_cond);
    pthread_cond_broadcast(&shm_ptr->done_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
  }
};

class xdma_workload_sim : private xdma_shm_device<xdma_workload_shm> {
private:
  static const char *path() {
    return "/xdma_sim_workload";
  }

public:
  xdma_workload_sim(bool is_host) : xdma_shm_device(path(), sizeof(xdma_workload_shm), is_host, "workload") {
    if (is_host) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&shm_ptr->lock, &attr);
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
      pthread_cond_init(&shm_ptr->accepted_cond, &cattr);
    }
  }

  int set_workload(const char *workload) {
    if (workload == nullptr || strlen(workload) >= WORKLOAD_PATH_SIZE) {
      return -1;
    }

    pthread_mutex_lock(&shm_ptr->lock);
    while (shm_ptr->valid) {
      pthread_cond_wait(&shm_ptr->accepted_cond, &shm_ptr->lock);
    }

    shm_ptr->closed = false;
    strcpy(shm_ptr->workload, workload);
    shm_ptr->accepted = false;
    shm_ptr->valid = true;
    pthread_cond_broadcast(&shm_ptr->accepted_cond);
    while (!shm_ptr->accepted) {
      pthread_cond_wait(&shm_ptr->accepted_cond, &shm_ptr->lock);
    }
    pthread_mutex_unlock(&shm_ptr->lock);
    return 0;
  }

  int wait_workload(char *workload, size_t size) {
    if (workload == nullptr || size == 0) {
      return -1;
    }

    pthread_mutex_lock(&shm_ptr->lock);
    while (!shm_ptr->valid && !shm_ptr->closed) {
      pthread_cond_wait(&shm_ptr->accepted_cond, &shm_ptr->lock);
    }
    if (shm_ptr->closed) {
      pthread_mutex_unlock(&shm_ptr->lock);
      return 0;
    }

    strncpy(workload, shm_ptr->workload, size - 1);
    workload[size - 1] = '\0';
    pthread_mutex_unlock(&shm_ptr->lock);
    return 1;
  }

  void complete_workload() {
    pthread_mutex_lock(&shm_ptr->lock);
    shm_ptr->valid = false;
    shm_ptr->accepted = true;
    pthread_cond_broadcast(&shm_ptr->accepted_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
  }

  void cancel() {
    pthread_mutex_lock(&shm_ptr->lock);
    shm_ptr->closed = true;
    pthread_cond_broadcast(&shm_ptr->accepted_cond);
    pthread_mutex_unlock(&shm_ptr->lock);
  }
};

// API for shared XDMA Dev
xdma_sim *xsim[8] = {nullptr};
xdma_h2c_sim *h2c_sim[8] = {nullptr};
xdma_axilite_sim *axilite_sim = nullptr;
xdma_workload_sim *workload_sim = nullptr;
static pthread_mutex_t xsim_lock = PTHREAD_MUTEX_INITIALIZER;

#ifndef FPGA_HOST
static void xdma_axilite_stop_thread();
static void xdma_h2c_stop_thread();
#endif // FPGA_HOST

void xdma_sim_open(int channel, bool is_host) {
  pthread_mutex_lock(&xsim_lock);
  if (xsim[channel] == nullptr) {
    xsim[channel] = new xdma_sim(channel, is_host);
  }
  pthread_mutex_unlock(&xsim_lock);
}

void xdma_sim_close(int channel) {
  pthread_mutex_lock(&xsim_lock);
  delete xsim[channel];
  xsim[channel] = nullptr;
  pthread_mutex_unlock(&xsim_lock);
}

int xdma_sim_read(int channel, char *buf, size_t size) {
  return xsim[channel]->read(buf, size);
}

int xdma_sim_write(int channel, const char *buf, uint8_t tlast, size_t size) {
  return xsim[channel]->write(buf, tlast, size);
}

void xdma_sim_h2c_open(int channel, bool is_host) {
  pthread_mutex_lock(&xsim_lock);
  if (h2c_sim[channel] == nullptr) {
    h2c_sim[channel] = new xdma_h2c_sim(channel, is_host);
  }
  pthread_mutex_unlock(&xsim_lock);
}

void xdma_sim_h2c_close(int channel) {
#ifndef FPGA_HOST
  xdma_h2c_stop_thread();
#endif // FPGA_HOST
  pthread_mutex_lock(&xsim_lock);
  delete h2c_sim[channel];
  h2c_sim[channel] = nullptr;
  pthread_mutex_unlock(&xsim_lock);
}

int xdma_sim_h2c_write(int channel, const char *buf, uint64_t tkeep, uint8_t tlast, size_t size) {
  return h2c_sim[channel]->write(buf, tkeep, tlast, size);
}

void xdma_sim_axilite_open(bool is_host) {
  pthread_mutex_lock(&xsim_lock);
  if (axilite_sim == nullptr) {
    axilite_sim = new xdma_axilite_sim(is_host);
  }
  pthread_mutex_unlock(&xsim_lock);
}

void xdma_sim_axilite_close(bool is_host) {
#ifndef FPGA_HOST
  if (!is_host) {
    xdma_axilite_stop_thread();
  }
#endif // FPGA_HOST
  pthread_mutex_lock(&xsim_lock);
  delete axilite_sim;
  axilite_sim = nullptr;
  pthread_mutex_unlock(&xsim_lock);
}

void xdma_sim_workload_open(bool is_host) {
  pthread_mutex_lock(&xsim_lock);
  if (workload_sim == nullptr) {
    workload_sim = new xdma_workload_sim(is_host);
  }
  pthread_mutex_unlock(&xsim_lock);
}

void xdma_sim_workload_close(bool is_host) {
  (void)is_host;
  pthread_mutex_lock(&xsim_lock);
  delete workload_sim;
  workload_sim = nullptr;
  pthread_mutex_unlock(&xsim_lock);
}

int xdma_sim_axilite_write(uint32_t addr, uint32_t data, uint8_t strb) {
  return axilite_sim->write(addr, data, strb);
}

int xdma_sim_axilite_read(uint32_t addr, uint32_t *data) {
  return axilite_sim->read(addr, data);
}

int xdma_sim_set_workload(const char *workload) {
  return workload_sim->set_workload(workload);
}

int xdma_sim_wait_workload(char *workload, size_t size) {
  return workload_sim->wait_workload(workload, size);
}

void xdma_sim_complete_workload() {
  workload_sim->complete_workload();
}

void xdma_sim_cancel_workload() {
  workload_sim->cancel();
}

extern "C" void v_xdma_write(uint8_t channel, const char *axi_tdata, uint8_t axi_tlast) {
  xdma_sim_write(channel, axi_tdata, axi_tlast, 64);
}

extern "C" void v_xdma_c2h_write(uint8_t channel, const char *axi_tdata, uint8_t axi_tlast) {
  xdma_sim_write(channel, axi_tdata, axi_tlast, 64);
}

#ifndef FPGA_HOST
static svScope axilite_scope = nullptr;
static svScope h2c_scope = nullptr;
static pthread_t axilite_thread;
static pthread_t h2c_thread;
static bool axilite_thread_started = false;
static bool h2c_thread_started = false;
static volatile bool axilite_thread_stop = false;
static volatile bool h2c_thread_stop = false;

extern "C" void v_xdma_axilite_write(uint32_t addr, uint32_t data, uint8_t strb, uint8_t *accepted);
extern "C" void v_xdma_axilite_read(uint32_t addr, uint32_t *data, uint8_t *accepted);
extern "C" void v_xdma_h2c_write(const svBitVecVal *axi_tdata, uint64_t axi_tkeep, svBit axi_tlast, uint8_t *accepted);

static void *xdma_h2c_thread_main(void *) {
  xdma_sim_h2c_open(0, false);
  while (!h2c_thread_stop) {
    svBitVecVal data[16] = {};
    uint64_t tkeep = 0;
    uint8_t tlast = 0;
    if (!h2c_sim[0]->wait(reinterpret_cast<char *>(data), &tkeep, &tlast, 64, &h2c_thread_stop)) {
      continue;
    }

    uint8_t accepted = 0;
    while (!accepted && !h2c_thread_stop) {
      svSetScope(h2c_scope);
      v_xdma_h2c_write(data, tkeep, tlast, &accepted);
      if (!accepted) {
        usleep(1000);
      }
    }
  }
  return nullptr;
}

static void *xdma_axilite_thread_main(void *) {
  xdma_sim_axilite_open(false);
  while (!axilite_thread_stop) {
    bool is_read = false;
    uint32_t addr = 0;
    uint32_t data = 0;
    uint8_t strb = 0;
    if (!axilite_sim->wait(&is_read, &addr, &data, &strb, &axilite_thread_stop)) {
      continue;
    }

    uint8_t accepted = 0;
    while (!accepted && !axilite_thread_stop) {
      svSetScope(axilite_scope);
      if (is_read) {
        v_xdma_axilite_read(addr, &data, &accepted);
      } else {
        v_xdma_axilite_write(addr, data, strb, &accepted);
      }
      if (!accepted) {
        usleep(1000);
      }
    }

    axilite_sim->complete(is_read ? data : 0);
  }
  return nullptr;
}

extern "C" void v_xdma_axilite_set_scope() {
  axilite_scope = svGetScope();
  if (axilite_thread_started) {
    return;
  }

  axilite_thread_stop = false;
  int ret = pthread_create(&axilite_thread, nullptr, xdma_axilite_thread_main, nullptr);
  if (ret != 0) {
    errno = ret;
    perror("XDMA_SIM: Failed to create AXI-Lite thread");
    exit(-1);
  }
  axilite_thread_started = true;
}

extern "C" void v_xdma_h2c_set_scope() {
  h2c_scope = svGetScope();
  if (h2c_thread_started) {
    return;
  }

  h2c_thread_stop = false;
  int ret = pthread_create(&h2c_thread, nullptr, xdma_h2c_thread_main, nullptr);
  if (ret != 0) {
    errno = ret;
    perror("XDMA_SIM: Failed to create H2C thread");
    exit(-1);
  }
  h2c_thread_started = true;
}

static void xdma_axilite_stop_thread() {
  if (!axilite_thread_started) {
    return;
  }

  axilite_thread_stop = true;
  if (axilite_sim != nullptr) {
    axilite_sim->notify();
  }
  pthread_join(axilite_thread, nullptr);
  axilite_thread_started = false;
  axilite_scope = nullptr;
}

static void xdma_h2c_stop_thread() {
  if (!h2c_thread_started) {
    return;
  }

  h2c_thread_stop = true;
  if (h2c_sim[0] != nullptr) {
    h2c_sim[0]->notify();
  }
  pthread_join(h2c_thread, nullptr);
  h2c_thread_started = false;
  h2c_scope = nullptr;
}
#endif // FPGA_HOST
