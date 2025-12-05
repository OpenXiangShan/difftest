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
// #ifdef FPGA_SIM

#include <getopt.h>
#include <iostream>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <atomic>
#include <mutex>

// #define FPGA_SIM
// #define FPGA_TRACEINST // or just test fpga xdma

#include "xdma.h"

#ifdef FPGA_TRACEINST
#include <trace_reader.h>
#include <tracertl.h>
#include <tracertl_dut_info.h>
extern TraceReader *trace_reader;
#endif // FPGA_TRACEINST

#ifdef FPGA_SIM
#include "fpga_sim.h"
#endif // FPGA_SIM

std::string tracertl_file;

FpgaXdma *xdma_device = NULL;
std::atomic<bool> running{false};
std::mutex trace_reader_mutex;

void args_parsing(int argc, char *argv[]);
void fpga_init();
void fpga_finish();
void write_loop();
void read_and_check_loop();
void trace_reader_init();

int main(int argc, char *argv[]) {
  args_parsing(argc, argv);

#ifdef FPGA_TRACEINST
  trace_reader_init();
#endif
  // init fpga-xdma
  printf("fpga init...\n");
  fflush(stdout);
  fpga_init();

  std::thread write_trace_thread(write_loop);
  std::thread read_check_thread(read_and_check_loop);

  write_trace_thread.join();
  read_check_thread.join();

  fpga_finish();
  printf("TraceRTL release the fpga device and exits\n");
  return 0;
}

void *posix_memalignd_malloc(size_t size) {
  void *ptr = nullptr;
  int ret = posix_memalign(&ptr, 4096, size);
  if (ret != 0) {
    perror("posix_memalign failed");
    return nullptr;
  }
  return ptr;
}

size_t upAlign(size_t size, size_t align) {
  return (size + align - 1) & ~(align - 1);
}

void write_loop() {
  running.store(true);

#ifdef FPGA_TRACEINST
  size_t trace_batch_inst_num = 16;
  size_t trace_batch_size = trace_reader->getFpgaPacketSize(trace_batch_inst_num);
  char *trace_buf = (char *)posix_memalignd_malloc(trace_batch_size);
  printf("Write Loop: inst Num: %lu, batch size: %lu\n", trace_batch_inst_num, trace_batch_size);
  fflush(stdout);
#else
  size_t trace_batch_size = 512 / 8 * 7;
  char *trace_buf = (char *)posix_memalignd_malloc(trace_batch_size);
  size_t times = 10;
  size_t max_times = 10;
  size_t count = 0;
#endif // FPGA_TRACEINST

  bool finished = false;
  do {
#ifdef FPGA_TRACEINST
    {
      if (!trace_reader->readFpgaInsts(trace_buf, trace_batch_inst_num)) {
        finished = true;
        running.store(false);
        printf("Trace over\n");
        fflush(stdout);
        break;
      };
    }
#else
    // test xdma
    for (int i = 0; i < trace_batch_size / 8; i++) {
      *(((uint64_t *)trace_buf) + i) = count ++;
    }
    printf("Write[%lu]:]", max_times - times);
    for (int i = 0; i < trace_batch_size / 8; i++) {
      printf("%lu ", *(((uint64_t *)trace_buf) + i));
    }
    printf("\n");
    fflush(stdout);
#endif // FPGA_TRACEINST

#ifdef FPGA_SIM
    fpga_sim_write(trace_buf, trace_batch_size);
    usleep(1000);
#else
    xdma_device->write_xdma(0,  trace_buf, trace_batch_size);
#endif // FPGA_SIM


#ifndef FPGA_TRACEINST
    times++;
    finished = times >= max_times;
#endif // FPGA_TRACEINST

  } while (!finished);
  running.store(false);
}

void read_and_check_loop() {
#ifdef FPGA_TRACEINST
  size_t trace_batch_inst_num = 128;
  size_t trace_batch_size = trace_reader->getFpgaResponseSize(trace_batch_inst_num);
  char *trace_buf = (char *)posix_memalignd_malloc(trace_batch_size);
#else
  size_t trace_batch_size = 512 / 8 * 7;
  char *trace_buf = (char *)posix_memalignd_malloc(trace_batch_size);
#endif // FPGA_TRACEINST

  size_t count = 0;
  static size_t times = 0;
  while (running.load()) {
    // read xdma-c2h
#ifdef FPGA_SIM
    {
      // std::lock_guard<std::mutex> lock(trace_reader_mutex);
      // if (!fpga_sim_read(trace_buf, trace_batch_size)) {
      //   // trace over
      //   running.store(false);
      //   break;
      // }
    }
#else
    xdma_device->read_xdma(0, trace_buf, trace_batch_size);
#endif // FPGA_SIM

    // check the result
#ifdef FPGA_TRACEINST
    // TraceFpgaCollectStruct *trace_inst_buf = (TraceFpgaCollectStruct *)trace_buf;
    // for (int i = 0; i < trace_batch_inst_num; i++) {
    //   trace_inst_buf[i].dump();
    // }
    {
      // std::lock_guard<std::mutex> lock(trace_reader_mutex);
      // trace_reader->fpgaCommitDiff(trace_inst_buf, trace_batch_inst_num);
    }
#else
    printf("Read[%lu]:", times++);
    for (int i = 0; i < trace_batch_size / 8; i++) {
      printf("%lu ", *(((uint64_t *)trace_buf) + i));
    }
    printf("\n");
#endif
    fflush(stdout);
  }
}

void fpga_init() {
  xdma_device = new FpgaXdma();
}

void fpga_finish() {
  delete xdma_device;

  fflush(stdout);
  fflush(stderr);
}

void args_parsing(int argc, char *argv[]) {
  int opt;
  int option_index = 0;
  static struct option long_options[] = {
                                         {"tracertl-file", required_argument, 0, 't'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "t:", long_options, &option_index)) != -1) {
    switch (opt) {
      case 0:
        break;
      case 't': tracertl_file = optarg; break;
      default:
        std::cerr
            << "Usage: " << argv[0]
            << " [-t <trace_file>]"
            << std::endl;
        exit(EXIT_FAILURE);
    }
  }

  if (tracertl_file.empty()) {
    std::cerr << "Usage: " << argv[0]
              << " [-t <trace_file>]"
              << std::endl;
    exit(EXIT_FAILURE);
  }
}

void trace_reader_init() {
#ifdef FPGA_TRACEINST
  trace_icache = new TraceICache(nullptr);
  trace_fastsim = new TraceFastSimManager(false, 0);
  trace_reader = new TraceReader(tracertl_file.c_str(), false, 40000000, 0);
#endif
}