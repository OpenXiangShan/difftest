/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#include "splitview.h"
#include <climits>

namespace {

int get_uart_sink_fd() {
  const int native_uart_fd = common_splitview_uart_fd();
  if (native_uart_fd >= 0) {
    return native_uart_fd;
  }

  const char *fd_env = getenv("DIFFTEST_UART_FD");
  if (fd_env == NULL || *fd_env == '\0') {
    return STDOUT_FILENO;
  }

  char *end = nullptr;
  long fd = strtol(fd_env, &end, 10);
  if (end == fd_env || *end != '\0' || fd < 0 || fd > INT_MAX) {
    return STDOUT_FILENO;
  }

  return static_cast<int>(fd);
}

} // namespace

extern "C" void difftest_uart_putc(char ch) {
  const int fd = get_uart_sink_fd();
  const char byte = ch;
  while (true) {
    ssize_t written = write(fd, &byte, 1);
    if (written == 1) {
      return;
    }
    if (written < 0 && errno == EINTR) {
      continue;
    }
    return;
  }
}
