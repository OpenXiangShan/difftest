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

#ifndef __COMMON_H
#define __COMMON_H

#include "config.h"
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifdef WITH_DRAMSIM3
#include "cosimulation.h"
#endif

enum {
  STATE_GOODTRAP = 0,
  STATE_BADTRAP = 1,
  STATE_ABORT = 2,
  STATE_LIMIT_EXCEEDED = 3,
  STATE_SIG = 4,
  STATE_AMBIGUOUS = 5,
  STATE_SIM_EXIT = 6,
  STATE_FUZZ_COND = 7,
  STATE_RUNNING = -1
};

extern int assert_count;
extern const char *emu_path;

extern int signal_num;
void sig_handler(int signo);

typedef uint64_t rtlreg_t;
typedef uint64_t vaddr_t;
typedef uint16_t ioaddr_t;

extern bool sim_verbose;

int eprintf(const char *fmt, ...);

#define Info(...)           \
  do {                      \
    if (sim_verbose) {      \
      eprintf(__VA_ARGS__); \
    }                       \
  } while (0)

#define Assert(cond, ...)           \
  do {                              \
    if (!(cond)) {                  \
      fflush(stdout);               \
      fprintf(stderr, "\33[1;31m"); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\33[0m\n");  \
      assert(cond);                 \
    }                               \
  } while (0)

#define panic(...) Assert(0, __VA_ARGS__)

#define fprintf_with_pid(stream, ...)   \
  do {                                  \
    fprintf(stream, "(%d) ", getpid()); \
    fprintf(stream, __VA_ARGS__);       \
  } while (0)

#define printf_with_pid(...)               \
  do {                                     \
    fprintf_with_pid(stdout, __VA_ARGS__); \
  } while (0)

#define TODO() panic("please implement me")

// Initialize common functions, such as buffering, assertions, siganl handlers.
void common_init(const char *program_name);

// Some designs may raise assertions during the reset stage.
// Use common_init_without_assertion with common_enable_assert to manually control assertions.
void common_init_without_assertion(const char *program_name);
void common_enable_assert();

// Enable external log system
typedef int (*eprintf_handle_t)(const char *fmt, va_list ap);
extern "C" void common_enable_log(eprintf_handle_t h);

void common_finish();

uint32_t uptime(void);

extern "C" void xs_assert(long long line);
extern "C" void xs_assert_v2(const char *filename, long long line);

const char *create_noop_filename(const char *suffix);

#endif // __COMMON_H
