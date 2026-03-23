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
#include "xdma.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE     65536
#define H2C_BUFFER_SIZE (64 * 1024) // 64KB for H2C
static const int AXILITE_HOST_START_GAP_US = 2 * 1000 * 1000;
static const int AXILITE_SIM_BOOT_TIMEOUT_US = 5 * 1000 * 1000;
static const int AXILITE_TX_TIMEOUT_US = AXILITE_HOST_START_GAP_US + AXILITE_SIM_BOOT_TIMEOUT_US;
static const int H2C_CONSUME_TIMEOUT_US = 30 * 1000 * 1000;
static const uint32_t AXILITE_SHM_MAGIC = 0x58444d41; // XDMA
#if defined(USE_XDMA_DDR_LOAD)
extern "C" void difftest_ram_write(uint64_t wIdx, uint64_t wdata, uint64_t wmask);
#endif

// C2H: Card to Host (DiffTest data from FPGA to Host)
typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t read_cond;
  bool read_waiting;
  int read_size;
  int write_size;
  char buffer[BUFFER_SIZE];
} xdma_shm;

// H2C: Host to Card (Workload data from Host to FPGA)
typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t write_cond;
  bool write_waiting;
  int read_size;
  int write_size;
  char buffer[H2C_BUFFER_SIZE];
} xdma_h2c_shm;

// AXI4-Lite Config BAR transaction state shared between host and simulator
typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t resp_cond;
  uint32_t magic;

  // Write address channel
  uint32_t aw_pending;
  uint32_t aw_addr;

  // Write data channel
  uint32_t w_pending;
  uint32_t w_data;
  uint32_t w_strb;

  // Write response channel
  uint32_t b_waiting;

  // Read address channel
  uint32_t ar_pending;
  uint32_t ar_addr;

  // Read data channel
  uint32_t r_waiting;
  uint32_t r_data;
} xdma_axilite_shm;

static inline timespec deadline_after_us(int timeout_us) {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += timeout_us / 1000000;
  ts.tv_nsec += (timeout_us % 1000000) * 1000;
  if (ts.tv_nsec >= 1000000000L) {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000L;
  }
  return ts;
}

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

// H2C simulator class
class xdma_h2c_sim {
private:
  int shm_fd = -1;
  xdma_h2c_shm *shm_ptr = nullptr;
  char path[128];
  bool is_host;

public:
  xdma_h2c_sim(bool _is_host) {
    is_host = _is_host;
    sprintf(path, "/xdma_sim_h2c0");
    shm_fd = shm_open(path, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
      perror("XDMA_H2C_SIM: Failed to open shared memory device\n");
      exit(-1);
    }
    if (is_host) {
      ftruncate(shm_fd, sizeof(xdma_h2c_shm));
    }
    shm_ptr = (xdma_h2c_shm *)mmap(NULL, sizeof(xdma_h2c_shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (is_host) {
      memset(shm_ptr, 0, sizeof(xdma_h2c_shm));
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
      pthread_mutex_init(&shm_ptr->lock, &attr);
      pthread_condattr_t cattr;
      pthread_condattr_init(&cattr);
      pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
      pthread_cond_init(&shm_ptr->write_cond, &cattr);
    }
  }
  ~xdma_h2c_sim() {
    if (is_host) {
      shm_unlink(path);
    }
    munmap(shm_ptr, sizeof(xdma_h2c_shm));
    close(shm_fd);
  }

  // Host side: write data to FPGA
  int write(const char *buf, size_t size) {
    assert(size <= H2C_BUFFER_SIZE);
    pthread_mutex_lock(&shm_ptr->lock);

    shm_ptr->write_waiting = true;
    shm_ptr->read_size = 0;
    shm_ptr->write_size = size;
    memcpy(shm_ptr->buffer, buf, size);

    pthread_cond_signal(&shm_ptr->write_cond);

    // Wait for FPGA to consume
    timespec deadline = deadline_after_us(H2C_CONSUME_TIMEOUT_US);
    while (shm_ptr->read_size < size) {
      int rc = pthread_cond_timedwait(&shm_ptr->write_cond, &shm_ptr->lock, &deadline);
      if (rc == ETIMEDOUT) {
        size_t consumed = shm_ptr->read_size;
        size_t total = shm_ptr->write_size;
        pthread_mutex_unlock(&shm_ptr->lock);
        fprintf(stderr, "XDMA H2C: write consume timeout after %d us (%zu/%zu bytes consumed)\n",
                H2C_CONSUME_TIMEOUT_US, consumed, total);
        return -1;
      }
      if (rc != 0 && rc != EINTR) {
        pthread_mutex_unlock(&shm_ptr->lock);
        fprintf(stderr, "XDMA H2C: write wait error (%d)\n", rc);
        return -1;
      }
    }

    pthread_mutex_unlock(&shm_ptr->lock);
    return size;
  }

  // FPGA side: read data from host (called by Verilog DPI-C)
  bool can_read() {
    pthread_mutex_lock(&shm_ptr->lock);
    bool has_data = shm_ptr->write_size > shm_ptr->read_size;
    pthread_mutex_unlock(&shm_ptr->lock);
    return has_data;
  }

  bool is_last_beat(size_t beat_bytes) {
    pthread_mutex_lock(&shm_ptr->lock);
    size_t remaining = shm_ptr->write_size - shm_ptr->read_size;
    bool last = remaining > 0 && remaining <= beat_bytes;
    pthread_mutex_unlock(&shm_ptr->lock);
    return last;
  }

  int read(char *buf, size_t size) {
    pthread_mutex_lock(&shm_ptr->lock);

    size_t available = shm_ptr->write_size - shm_ptr->read_size;
    size_t to_read = size < available ? size : available;

    if (to_read > 0) {
      memcpy(buf, shm_ptr->buffer + shm_ptr->read_size, to_read);
      shm_ptr->read_size += to_read;

      // Signal host that data has been consumed
      if (shm_ptr->read_size >= shm_ptr->write_size) {
        pthread_cond_signal(&shm_ptr->write_cond);
      }
    }

    pthread_mutex_unlock(&shm_ptr->lock);
    return to_read;
  }
};

class xdma_axilite_sim {
private:
  int shm_fd = -1;
  xdma_axilite_shm *shm_ptr = nullptr;
  char path[128];
  bool is_host;

  void init_shared_state() {
    memset(shm_ptr, 0, sizeof(xdma_axilite_shm));

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm_ptr->lock, &mattr);

    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shm_ptr->resp_cond, &cattr);

    shm_ptr->magic = AXILITE_SHM_MAGIC;
  }

public:
  xdma_axilite_sim(bool _is_host) {
    is_host = _is_host;
    sprintf(path, "/xdma_sim_axilite0");

    shm_fd = shm_open(path, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
      perror("XDMA AXILITE: Failed to open shared memory device\n");
      exit(-1);
    }

    bool need_init = is_host;
    struct stat st;
    if (fstat(shm_fd, &st) == 0 && st.st_size < (off_t)sizeof(xdma_axilite_shm)) {
      need_init = true;
    }
    if (need_init) {
      ftruncate(shm_fd, sizeof(xdma_axilite_shm));
    }

    shm_ptr = (xdma_axilite_shm *)mmap(NULL, sizeof(xdma_axilite_shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
      perror("XDMA AXILITE: Failed to mmap shared memory\n");
      close(shm_fd);
      exit(-1);
    }

    if (need_init) {
      init_shared_state();
    } else if (shm_ptr->magic != AXILITE_SHM_MAGIC) {
      // If simulator starts before host, wait briefly for host-side initialization.
      int retries = 2000; // 2 seconds
      while (retries > 0 && shm_ptr->magic != AXILITE_SHM_MAGIC) {
        usleep(1000);
        retries--;
      }
      if (shm_ptr->magic != AXILITE_SHM_MAGIC) {
        init_shared_state();
      }
    }
  }

  ~xdma_axilite_sim() {
    if (is_host) {
      shm_unlink(path);
    }
    munmap(shm_ptr, sizeof(xdma_axilite_shm));
    close(shm_fd);
  }

  xdma_axilite_shm *state() {
    return shm_ptr;
  }
};

// API for shared XDMA Dev (C2H)
xdma_sim *xsim_c2h[8] = {nullptr};

// H2C simulator instance
static xdma_h2c_sim *xsim_h2c = nullptr;
// AXI-Lite Config BAR simulator instance
static xdma_axilite_sim *xsim_axilite = nullptr;
static bool h2c_seq_ready = false;

void xdma_c2h_sim_open(int channel, bool is_host) {
  xsim_c2h[channel] = new xdma_sim(channel, is_host);
}

void xdma_c2h_sim_close(int channel) {
  delete xsim_c2h[channel];
  xsim_c2h[channel] = nullptr;
}

int xdma_c2h_sim_read(int channel, char *buf, size_t size) {
  return xsim_c2h[channel]->read(buf, size);
}

int xdma_c2h_sim_write(int channel, const char *buf, uint8_t tlast, size_t size) {
  return xsim_c2h[channel]->write(buf, tlast, size);
}

extern "C" void v_xdma_c2h_write(uint8_t channel, const char *axi_tdata, uint8_t axi_tlast) {
  xdma_c2h_sim_write(channel, axi_tdata, axi_tlast, 64);
}

// ===== H2C APIs =====

void xdma_h2c_sim_open(bool is_host) {
  xsim_h2c = new xdma_h2c_sim(is_host);
}

void xdma_h2c_sim_close() {
  if (xsim_h2c) {
    delete xsim_h2c;
    xsim_h2c = nullptr;
  }
  h2c_seq_ready = false;
}

void xdma_config_bar_open(bool is_host) {
  if (!xsim_axilite) {
    xsim_axilite = new xdma_axilite_sim(is_host);
  }
}

void xdma_config_bar_close() {
  if (xsim_axilite) {
    delete xsim_axilite;
    xsim_axilite = nullptr;
  }
  h2c_seq_ready = false;
}

// Host side: write workload data to FPGA via H2C stream
int xdma_h2c_write_ddr(const char *workload, size_t size) {
  if (!xsim_h2c) {
    fprintf(stderr, "XDMA H2C: sim not initialized\n");
    return -1;
  }
  if (!h2c_seq_ready) {
    fprintf(stderr, "XDMA H2C: init sequence is not ready, skip write\n");
    return -1;
  }
  return xsim_h2c->write(workload, size);
}

// FPGA side (DPI-C functions)
extern "C" unsigned char v_xdma_h2c_tvalid() {
  if (!xsim_h2c)
    return 0;
  return xsim_h2c->can_read() ? 1 : 0;
}

extern "C" unsigned char v_xdma_h2c_tlast() {
  if (!xsim_h2c)
    return 1;
  return xsim_h2c->is_last_beat(8) ? 1 : 0;
}

extern "C" void v_xdma_h2c_read(uint8_t channel, char *axi_tdata) {
  (void)channel;
  memset(axi_tdata, 0, 8);
  if (!xsim_h2c) {
    return;
  }
  xsim_h2c->read(axi_tdata, 8);
}

extern "C" void v_xdma_h2c_commit(int beat_idx, const uint32_t *axi_tdata) {
#if defined(USE_XDMA_DDR_LOAD)
  if (!axi_tdata || beat_idx < 0) {
    return;
  }
  uint64_t lo = (uint64_t)axi_tdata[0];
  uint64_t hi = (uint64_t)axi_tdata[1];
  uint64_t wdata = (hi << 32) | lo;
  difftest_ram_write((uint64_t)beat_idx, wdata, ~0ULL);
#else
  (void)beat_idx;
  (void)axi_tdata;
#endif
}

// ===== Config BAR Implementation for FPGA_SIM =====
// Config BAR register map (in hardware, accessed via AXI4-Lite):
// 0x00: HOST_IO_RESET (RW)
// 0x04: HOST_IO_DIFFTEST_ENABLE (RW)
// 0x08: HOST_IO_DDR_ARB_SEL (RW) - Bit 0: 0=CPU owns DDR, 1=H2C owns DDR
// 0x0C: HOST_IO_H2C_LENGTH (RW)  - Transfer length in beats (0=unlimited/tlast-based)
// 0x10: HOST_IO_H2C_STATUS (RO)  - Bit 1: H2C active, Bit 2: H2C done
// 0x14: HOST_IO_H2C_BEAT_CNT (RO) - Current beat counter
// 0x18: HOST_IO_H2C_BEAT_BYTES (RO) - Bytes per H2C beat

enum wait_result {
  WAIT_OK = 0,
  WAIT_TIMEOUT = 1,
  WAIT_ERROR = 2,
};

static inline xdma_axilite_shm *get_axilite_state() {
  if (!xsim_axilite) {
    return nullptr;
  }
  return xsim_axilite->state();
}

static int wait_for_flag_clear_locked(xdma_axilite_shm *state, uint32_t *flag, int timeout_us) {
  timespec deadline = deadline_after_us(timeout_us);
  while (*flag != 0) {
    int rc = pthread_cond_timedwait(&state->resp_cond, &state->lock, &deadline);
    if (rc == ETIMEDOUT) {
      return *flag != 0 ? WAIT_TIMEOUT : WAIT_OK;
    }
    if (rc != 0 && rc != EINTR) {
      return WAIT_ERROR;
    }
  }
  return WAIT_OK;
}

static bool xdma_config_bar_write_impl(uint32_t offset, uint32_t data, uint8_t strb) {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state) {
    fprintf(stderr, "XDMA Config BAR: AXI-Lite simulator not initialized\n");
    return false;
  }

  pthread_mutex_lock(&state->lock);
  if (state->aw_pending || state->w_pending || state->b_waiting) {
    fprintf(stderr, "XDMA Config BAR: write transaction already in progress\n");
    pthread_mutex_unlock(&state->lock);
    return false;
  }

  state->aw_pending = true;
  state->aw_addr = offset;
  state->w_pending = true;
  state->w_data = data;
  state->w_strb = strb;
  state->b_waiting = true;
  printf("XDMA Config BAR: write offset=%x data=%x strb=%x\n", offset, data, strb);

  int wait_ret = wait_for_flag_clear_locked(state, &state->b_waiting, AXILITE_TX_TIMEOUT_US);
  if (wait_ret != WAIT_OK) {
    bool aw_pending = state->aw_pending;
    bool w_pending = state->w_pending;
    if (wait_ret == WAIT_TIMEOUT && !aw_pending && !w_pending) {
      // Some simulation configurations do not return B response, but AW/W are consumed.
      state->b_waiting = false;
      pthread_mutex_unlock(&state->lock);
      fprintf(stderr, "XDMA Config BAR: write at offset %x has no B response, continue after AW/W accepted\n", offset);
      return true;
    }
    state->aw_pending = false;
    state->w_pending = false;
    state->b_waiting = false;
    pthread_mutex_unlock(&state->lock);
    if (wait_ret == WAIT_TIMEOUT) {
      fprintf(stderr, "XDMA Config BAR: write timeout at offset %x (aw_pending=%d w_pending=%d)\n", offset, aw_pending,
              w_pending);
    } else {
      fprintf(stderr, "XDMA Config BAR: write wait error at offset %x\n", offset);
    }
    return false;
  }

  pthread_mutex_unlock(&state->lock);
  printf("XDMA Config BAR: write completed at offset %x\n", offset);
  return true;
}

static bool xdma_config_bar_read_impl(uint32_t offset, uint32_t *data) {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state) {
    fprintf(stderr, "XDMA Config BAR: AXI-Lite simulator not initialized\n");
    return false;
  }

  pthread_mutex_lock(&state->lock);
  if (state->ar_pending || state->r_waiting) {
    fprintf(stderr, "XDMA Config BAR: read transaction already in progress\n");
    pthread_mutex_unlock(&state->lock);
    return false;
  }

  state->ar_pending = true;
  state->ar_addr = offset;
  state->r_waiting = true;
  printf("XDMA Config BAR: read transaction started at offset %x\n", offset);

  int wait_ret = wait_for_flag_clear_locked(state, &state->r_waiting, AXILITE_TX_TIMEOUT_US);
  if (wait_ret != WAIT_OK) {
    bool ar_pending = state->ar_pending;
    state->ar_pending = false;
    state->r_waiting = false;
    pthread_mutex_unlock(&state->lock);
    if (wait_ret == WAIT_TIMEOUT) {
      fprintf(stderr, "XDMA Config BAR: read timeout at offset %x (ar_pending=%d)\n", offset, ar_pending);
    } else {
      fprintf(stderr, "XDMA Config BAR: read wait error at offset %x\n", offset);
    }
    return false;
  }

  *data = state->r_data;
  pthread_mutex_unlock(&state->lock);
  printf("XDMA Config BAR: read offset=%x -> data=%x\n", offset, *data);
  return true;
}

// DPI-C functions called from xdma_axilite.v
extern "C" bool v_xdma_axilite_aw_ready() {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return false;
  pthread_mutex_lock(&state->lock);
  bool ready = state->aw_pending;
  pthread_mutex_unlock(&state->lock);
  return ready;
}

extern "C" void v_xdma_axilite_get_aw(uint32_t *addr, uint32_t *prot) {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state) {
    *addr = 0;
    *prot = 0;
    return;
  }
  pthread_mutex_lock(&state->lock);
  *addr = state->aw_addr;
  *prot = 0; // Fixed prot value
  state->aw_pending = false;
  pthread_mutex_unlock(&state->lock);
}

extern "C" bool v_xdma_axilite_w_ready() {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return false;
  pthread_mutex_lock(&state->lock);
  bool ready = state->w_pending;
  pthread_mutex_unlock(&state->lock);
  return ready;
}

extern "C" void v_xdma_axilite_get_w(uint32_t *data, uint32_t *strb) {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state) {
    *data = 0;
    *strb = 0;
    return;
  }
  pthread_mutex_lock(&state->lock);
  *data = state->w_data;
  *strb = state->w_strb;
  state->w_pending = false;
  pthread_mutex_unlock(&state->lock);
}

extern "C" bool v_xdma_axilite_b_ready() {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return false;
  pthread_mutex_lock(&state->lock);
  bool ready = state->b_waiting;
  pthread_mutex_unlock(&state->lock);
  return ready;
}

extern "C" void v_xdma_axilite_set_b(uint32_t resp) {
  (void)resp;
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return;
  pthread_mutex_lock(&state->lock);
  state->b_waiting = false;
  pthread_cond_broadcast(&state->resp_cond);
  pthread_mutex_unlock(&state->lock);
}

extern "C" bool v_xdma_axilite_ar_ready() {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return false;
  pthread_mutex_lock(&state->lock);
  bool ready = state->ar_pending;
  pthread_mutex_unlock(&state->lock);
  return ready;
}

extern "C" void v_xdma_axilite_get_ar(uint32_t *addr, uint32_t *prot) {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state) {
    *addr = 0;
    *prot = 0;
    return;
  }
  pthread_mutex_lock(&state->lock);
  *addr = state->ar_addr;
  *prot = 0; // Fixed prot value
  state->ar_pending = false;
  pthread_mutex_unlock(&state->lock);
}

extern "C" bool v_xdma_axilite_r_ready() {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return false;
  pthread_mutex_lock(&state->lock);
  bool ready = state->r_waiting;
  pthread_mutex_unlock(&state->lock);
  return ready;
}

extern "C" void v_xdma_axilite_set_r(uint32_t data, uint32_t resp) {
  (void)resp;
  xdma_axilite_shm *state = get_axilite_state();
  if (!state)
    return;
  pthread_mutex_lock(&state->lock);
  state->r_data = data;
  state->r_waiting = false;
  pthread_cond_broadcast(&state->resp_cond);
  pthread_mutex_unlock(&state->lock);
}

// C++ API for Config BAR access (through AXI4-Lite)
void xdma_config_bar_init() {
  xdma_axilite_shm *state = get_axilite_state();
  if (!state) {
    fprintf(stderr, "XDMA Config BAR: AXI-Lite simulator not initialized\n");
    return;
  }
  pthread_mutex_lock(&state->lock);
  state->aw_pending = false;
  state->aw_addr = 0;
  state->w_pending = false;
  state->w_data = 0;
  state->w_strb = 0;
  state->b_waiting = false;
  state->ar_pending = false;
  state->ar_addr = 0;
  state->r_waiting = false;
  state->r_data = 0;
  pthread_cond_broadcast(&state->resp_cond);
  pthread_mutex_unlock(&state->lock);
  h2c_seq_ready = false;
  printf("XDMA Config BAR initialized\n");
}

void xdma_config_bar_write(uint32_t offset, uint32_t data, uint8_t strb) {
  if ((offset & 0x3) != 0) {
    fprintf(stderr, "XDMA Config BAR: misaligned write offset %x\n", offset);
    return;
  }

  uint32_t idx = (offset >> 2) & 0x7;
  if (idx >= 8) {
    fprintf(stderr, "XDMA Config BAR: invalid write offset %x\n", offset);
    return;
  }

  // Only allow writes to writable registers (0x00/0x04/0x08/0x0C)
  if (idx <= 3) {
    (void)xdma_config_bar_write_impl(offset, data, strb);
  } else {
    fprintf(stderr, "XDMA Config BAR: write to read-only register offset %x\n", offset);
  }
}

uint32_t xdma_config_bar_read(uint32_t offset) {
  if ((offset & 0x3) != 0) {
    fprintf(stderr, "XDMA Config BAR: misaligned read offset %x\n", offset);
    return 0;
  }

  uint32_t idx = (offset >> 2) & 0x7;
  if (idx >= 8) {
    fprintf(stderr, "XDMA Config BAR: invalid read offset %x\n", offset);
    return 0;
  }

  uint32_t data = 0;
  if (!xdma_config_bar_read_impl(offset, &data)) {
    return 0;
  }
  return data;
}

// Helper function: Initialize H2C transfer sequence
// This function configures the Config BAR and performs H2C transfer
void xdma_h2c_init_sequence(uint32_t transfer_len) {
  printf("=== H2C Initialization Sequence ===\n");
  h2c_seq_ready = false;

  // Step 1: Initialize Config BAR
  xdma_config_bar_init();

  // Step 2: Set H2C transfer length
  if (!xdma_config_bar_write_impl(HOST_IO_H2C_LENGTH, transfer_len, 0xF)) {
    fprintf(stderr, "H2C init failed: unable to set transfer length\n");
    return;
  }

  // Step 3: Enable H2C arbitration (set bit 0 of HOST_IO_DDR_ARB_SEL)
  // This will:
  //   - Set ddr_arb_sel = 1 (H2C owns DDR)
  //   - Block CPU from accessing DDR
  if (!xdma_config_bar_write_impl(HOST_IO_DDR_ARB_SEL, 0x1, 0xF)) {
    fprintf(stderr, "H2C init failed: unable to enable H2C arbitration\n");
    return;
  }

  h2c_seq_ready = true;
  printf("H2C arbitration enabled, CPU blocked from DDR\n");

  // At this point:
  // 1. CPU is blocked from accessing DDR (ddr_arb_sel = 1)
  // 2. H2C owns DDR
  // 3. H2C Stream is ready to receive data
  //
  // The caller should now call xdma_h2c_write_ddr() to transfer data
  // After transfer completes, the caller should:
  //   - Check H2C_STATUS for completion
  //   - Clear ddr_arb_sel to re-enable CPU
}

// Helper function: Complete H2C transfer and re-enable CPU
void xdma_h2c_complete_sequence() {
  printf("=== Completing H2C Transfer ===\n");
  if (!h2c_seq_ready) {
    fprintf(stderr, "H2C complete skipped: init sequence is not ready\n");
    return;
  }

  // Step 1: Sample H2C status once (stream consumption has already completed on host side)
  uint32_t status = 0;
  uint32_t beat_count = 0;
  bool status_ok = xdma_config_bar_read_impl(HOST_IO_H2C_STATUS, &status);
  bool beat_ok = xdma_config_bar_read_impl(HOST_IO_H2C_BEAT_CNT, &beat_count);
  if (status_ok && (status & 0x4)) {
    printf("H2C transfer completed, beat_count=%u\n", beat_ok ? beat_count : 0);
  } else {
    fprintf(stderr, "H2C transfer status not done, continue restoring CPU arbitration\n");
  }

  // Step 2: Disable H2C arbitration (clear bit 0 of HOST_IO_DDR_ARB_SEL)
  // This will:
  //   - Set ddr_arb_sel = 0 (CPU owns DDR)
  //   - Enable CPU clock (cpu_clock_enable = 1)
  if (!xdma_config_bar_write_impl(HOST_IO_DDR_ARB_SEL, 0x0, 0xF)) {
    fprintf(stderr, "H2C complete failed: unable to restore CPU arbitration\n");
    h2c_seq_ready = false;
    return;
  }

  printf("CPU arbitration restored, CPU clock enabled\n");
  printf("=== H2C Initialization Complete ===\n");
  h2c_seq_ready = false;
}

uint32_t xdma_h2c_get_beat_bytes() {
  uint32_t beat_bytes = xdma_config_bar_read(HOST_IO_H2C_BEAT_BYTES);
  if (beat_bytes == 0) {
    beat_bytes = 8;
  }
  return beat_bytes;
}
