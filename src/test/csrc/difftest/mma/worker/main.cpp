/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#include "mma/worker/worker.h"
#include <cstdarg>
#include <string>

bool sim_verbose = false;
int eprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const int result = vfprintf(stderr, format, args);
  va_end(args);
  return result;
}

int main(int argc, char **argv) {
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  if (argc != 3)
    return 2;
  return mma_worker_main(static_cast<unsigned>(std::stoul(argv[1])), static_cast<pid_t>(std::stol(argv[2])));
#else
  return 0;
#endif
}
