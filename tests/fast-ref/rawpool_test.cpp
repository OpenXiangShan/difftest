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
#include "rawpool.h"
#include <sys/wait.h>
#include <unistd.h>

int main() {
  RawPacketPool pool(8, 64);
  const auto reader = pool.add_reader();
  assert(reader == 1);
  char *partial = pool.get_free();
  partial[0] = 42;
  assert(pool.get_busy() == nullptr); // Unpublished partial packet.
  for (unsigned i = 0; i < 8; ++i) {
    char *slot = pool.get_free();
    assert(slot);
    memset(slot, i, 64);
    pool.publish();
  }
  for (unsigned i = 0; i < 8; ++i) {
    assert(static_cast<unsigned char>(pool.get_busy()[0]) == i);
    pool.release();
  }
  assert(pool.get_free() == nullptr); // Fork reader retains all eight slots.
  const pid_t pid = fork();
  assert(pid >= 0);
  if (pid == 0) {
    pool.enter_reader(reader);
    for (unsigned i = 0; i < 8; ++i) {
      char *slot = pool.get_busy();
      if (!slot || static_cast<unsigned char>(slot[0]) != i)
        _exit(2);
      pool.release();
    }
    _exit(0); // Child never retires its own reader ID.
  }
  int status = 0;
  assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert(pool.add_reader() == 2); // Reader 1 remains reserved until parent reap/retire.
  pool.retire_reader(reader);
  assert(pool.add_reader() == reader);
  for (unsigned i = 3; i < RawPacketPool::MAX_READERS; ++i)
    assert(pool.add_reader() == i);
  assert(pool.add_reader() == RawPacketPool::MAX_READERS);
  char *wrapped = pool.get_free();
  assert(wrapped == partial);
  wrapped[0] = 99;
  pool.publish();
  assert(pool.get_busy()[0] == 99);
  pool.fail();
  assert(!pool.get_busy() && !pool.get_free());
  for (size_t count: {size_t(0), size_t(1), size_t(3), size_t(1) << 63}) {
    bool threw = false;
    try {
      RawPacketPool invalid(count, 64);
    } catch (const std::runtime_error &) {
      threw = true;
    }
    assert(threw);
  }
  puts("RawPacketPool retention, publication, wraparound, fork and reader reuse: PASS");
}
