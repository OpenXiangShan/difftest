/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include "common.h"
#include <locale.h>
#include <signal.h>
#include <time.h>

int assert_count = 0;
int signal_num = 0;
const char *emu_path = NULL;

// Usage in SV/Verilog: xs_assert(`__LINE__);
void xs_assert(long long line) {
  if (assert_count >= 0) {
    printf("Assertion failed at line %lld.\n", line);
    assert_count++;
  }
}

// Usage in SV/Verilog: xs_assert_v2(`__FILE__, `__LINE__);
void xs_assert_v2(const char *filename, long long line) {
  if (assert_count >= 0) {
    printf("Assertion failed at %s:%lld.\n", filename, line);
    assert_count++;
  }
}

void sig_handler(int signo) {
  if (signal_num != 0)
    exit(0);
  signal_num = signo;
}

static struct timeval boot = {};

uint32_t uptime(void) {
  struct timeval t;
  gettimeofday(&t, NULL);

  int s = t.tv_sec - boot.tv_sec;
  int us = t.tv_usec - boot.tv_usec;
  if (us < 0) {
    s--;
    us += 1000000;
  }

  return s * 1000 + (us + 500) / 1000;
}

static char mybuf[BUFSIZ];

void common_init_without_assertion(const char *program_name) {
  // set emu_path
  emu_path = program_name;

  const char *elf_name = strrchr(program_name, '/');
  elf_name = elf_name ? elf_name + 1 : program_name;
  Info("%s compiled at %s, %s\n", elf_name, __DATE__, __TIME__);

  // set buffer for stderr
  setbuf(stderr, mybuf);

  // enable thousands separator for printf()
  setlocale(LC_NUMERIC, "");

  // set up SIGINT handler
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    Info("\ncan't catch SIGINT\n");
  }

  gettimeofday(&boot, NULL);

  assert_count = -1;
  signal_num = 0;
}

void common_enable_assert() {
  assert_count = 0;
}

void common_init(const char *program_name) {
  common_init_without_assertion(program_name);
  common_enable_assert();
}

void common_finish() {
  fflush(stdout);
}

static eprintf_handle_t eprintf_handle = vprintf;

extern "C" void common_enable_log(eprintf_handle_t h) {
  assert(h != NULL);
  eprintf_handle = h;
}

int eprintf(const char *fmt, ...) {
  va_list args;
  int ret;
  va_start(args, fmt);
  ret = (*eprintf_handle)(fmt, args);
  va_end(args);
  return ret;
}

bool sim_verbose = true;

extern "C" void enable_sim_verbose() {
  sim_verbose = true;
}

extern "C" void disable_sim_verbose() {
  sim_verbose = false;
}

// formatted as follows: <NOOP_HOME>/build/<timestamp>
const char *create_noop_filename(const char *suffix) {
  static char noop_filename[1024] = "";

  static int noop_len = 0;
  // initialize the NOOP_HOME only once
  if (!noop_len) {
    // get NOOP_HOME environment variable
    const char *noop_home = getenv("NOOP_HOME");
#ifdef NOOP_HOME
    if (noop_home == nullptr) {
      noop_home = NOOP_HOME;
    }
#endif
    assert(noop_home != NULL);

    noop_len = snprintf(noop_filename, sizeof(noop_filename), "%s/build/", noop_home);
  }

  // get formatted time
  time_t now = time(NULL);
  int time_len = strftime(noop_filename + noop_len, sizeof(noop_filename) - noop_len, "%F-%H-%M-%S", localtime(&now));

  // copy the suffix if provided
  if (suffix) {
    snprintf(noop_filename + noop_len + time_len, sizeof(noop_filename) - noop_len - time_len, "%s", suffix);
  } else {
    noop_filename[noop_len + time_len] = '\0'; // ensure null-termination
  }

  return noop_filename;
}
