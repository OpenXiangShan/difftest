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

#include "device.h"
#include "diffstate.h"
#include "difftest.h"
#include "flash.h"
#include "goldenmem.h"
#include "mpool.h"
#include "ram.h"
#include "refproxy.h"
#include "xdma.h"
#include <condition_variable>
#include <getopt.h>
#include <mutex>

void fpga_finish();

enum {
  FPGA_RUN,
  FPGA_DONE,
  FPGA_FAIL,
} simv_state;
struct sematit_ckpt_info {
  uint64_t pc;
  uint64_t last_inst;
  uint64_t count;
};

static char work_load[256] = "/dev/zero";
static std::vector<sematit_ckpt_info> sematic_ckpt_list;
static std::atomic<uint8_t> simv_result{FPGA_RUN};
static std::mutex simv_mtx;
static std::condition_variable simv_cv;
static uint64_t max_instrs = 0;
static FpgaXdma *xdma_device = NULL;

struct core_end_info_t {
  bool core_trap[NUM_CORES];
  double core_cpi[NUM_CORES];
  uint8_t core_trap_num;
};
static core_end_info_t core_end_info;

void fpga_init();
void fpga_step();
void cpu_endtime_check();
void set_diff_ref_so(char *s);
void args_parsing(int argc, char *argv[]);
void sematic_checkpoint_out(const char *output_file);

int main(int argc, char *argv[]) {
  args_parsing(argc, argv);

  fpga_init();
  printf("simv init\n");
  {
    std::unique_lock<std::mutex> lock(simv_mtx);
    xdma_device->start_transmit_thread();
    while (simv_result.load() == FPGA_RUN) {
      simv_cv.wait(lock);
    }
  }
  xdma_device->running = false;
  fpga_finish();
#ifdef SEMATIC_CHECKPOINT
  sematic_checkpoint_out("sematic_ckpt_result.txt");
#endif // SEMATIC_CHECKPOINT
  printf("difftest releases the fpga device and exits\n");
  return 0;
}

void set_diff_ref_so(char *s) {
  extern const char *difftest_ref_so;
  char *buf = (char *)malloc(256);
  strcpy(buf, s);
  difftest_ref_so = buf;
}

void sematic_checkpoint_init(const char *file) {
  FILE *fd = fopen(file, "r");
  if (!fd) {
    std::cerr << "Error: Could not open file " << file << std::endl;
    exit(EXIT_FAILURE);
  }

  char line[256];
  while (fgets(line, sizeof(line), fd)) {
    uint64_t value = std::stoull(line, nullptr, 16);
    sematic_ckpt_list.push_back({value, 0, 0});
  }

  fclose(fd);
}

void sematic_checkpoint_check(uint64_t pc, uint64_t inst) {
  for (auto &ckpt: sematic_ckpt_list) {
    if (ckpt.pc == pc) {
      ckpt.last_inst = inst;
      ckpt.count++;
      break;
    }
  }
}

void sematic_checkpoint_out(const char *output_file) {
  FILE *fd = fopen(output_file, "w");
  if (!fd) {
    std::cerr << "Error: Could not open file " << output_file << " for writing" << std::endl;
    return;
  }

  for (const auto &ckpt: sematic_ckpt_list) {
    fprintf(fd, "0x%lx,0x%lx,0x%ld\n", ckpt.pc, ckpt.last_inst, ckpt.count);
  }

  fclose(fd);
}

void fpga_init() {
  xdma_device = new FpgaXdma();
  init_ram(work_load, DEFAULT_EMU_RAM_SIZE);
  init_flash(NULL);

  difftest_init();

  init_device();
  init_goldenmem();
  init_nemuproxy(DEFAULT_EMU_RAM_SIZE);
  xdma_device->ddr_load_workload(work_load);
}

void fpga_finish() {
  free(xdma_device);
  common_finish();

  difftest_finish();
  goldenmem_finish();
  finish_device();

  delete simMemory;
  simMemory = nullptr;
}

extern "C" void fpga_nstep(uint8_t step) {
  for (int i = 0; i < step; i++) {
    fpga_step();
  }
}

void fpga_step() {
  if (difftest_step()) {
    printf("FPGA_FAIL\n");
    simv_result.store(FPGA_FAIL);
    simv_cv.notify_one();
  }
  if (difftest_state() != -1) {
    int trapCode = difftest_state();
    for (int i = 0; i < NUM_CORES; i++) {
      printf("Core %d: ", i);
      uint64_t pc = difftest[i]->get_trap_event()->pc;
      switch (trapCode) {
        case 0: eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
        default: eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n" ANSI_COLOR_RESET, trapCode);
      }
      difftest[i]->display_stats();
    }
    if (trapCode == 0)
      simv_result.store(FPGA_DONE);
    else
      simv_result.store(FPGA_FAIL);
    simv_cv.notify_one();
  }
  cpu_endtime_check();
#ifdef SEMATIC_CHECKPOINT
  for (size_t i = 0; i < NUM_CORES; i++) {
    auto trap = difftest[i]->get_trap_event();
    sematic_checkpoint_check(trap->pc, trap->instrCnt);
  }
#endif // SEMATIC_CHECKPOINT
}

void cpu_endtime_check() {
  if (max_instrs != 0) { // 0 for no limit
    for (int i = 0; i < NUM_CORES; i++) {
      if (core_end_info.core_trap[i])
        continue;
      auto trap = difftest[i]->get_trap_event();
      if (max_instrs < trap->instrCnt) {
        core_end_info.core_trap[i] = true;
        core_end_info.core_trap_num++;
        eprintf(ANSI_COLOR_GREEN "EXCEEDED CORE-%d MAX INSTR: %ld\n" ANSI_COLOR_RESET, i, max_instrs);
        difftest[i]->display_stats();
        core_end_info.core_cpi[i] = (double)trap->cycleCnt / (double)trap->instrCnt;
        if (core_end_info.core_trap_num == NUM_CORES) {
          simv_result.store(FPGA_DONE);
          simv_cv.notify_one();
        }
      }
    }
  }
}

void args_parsing(int argc, char *argv[]) {
  int opt;
  int option_index = 0;
  static struct option long_options[] = {{"diff", required_argument, 0, 0},
                                         {"max-instrs", required_argument, 0, 0},
                                         {"sematic-checkpoint", required_argument, 0, 0},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "i:", long_options, &option_index)) != -1) {
    switch (opt) {
      case 0:
        if (strcmp(long_options[option_index].name, "diff") == 0) {
          set_diff_ref_so(optarg);
        } else if (strcmp(long_options[option_index].name, "max-instrs") == 0) {
          max_instrs = std::stoul(optarg, nullptr, 10);
        } else if (strcmp(long_options[option_index].name, "sematic-checkpoint") == 0) {
          sematic_checkpoint_init(optarg);
        }
        break;
      case 'i': strncpy(work_load, optarg, sizeof(work_load) - 1); break;
      default:
        std::cerr << "Usage: " << argv[0]
                  << " [--diff <path>] [-i <workload>] [--max-instrs <num>] [--sematic-checkpoint <path>]" << std::endl;
        exit(EXIT_FAILURE);
    }
  }
}
