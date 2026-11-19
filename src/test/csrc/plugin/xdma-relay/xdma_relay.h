/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#ifndef __XDMA_RELAY_H__
#define __XDMA_RELAY_H__
#include <cstdint>
#include <mutex>
#include <sys/types.h>
#include <vector>

class XdmaRelayClient {
public:
  static bool enabled();
  XdmaRelayClient(size_t packet_bytes, unsigned channels);
  ~XdmaRelayClient();
  ssize_t read(unsigned channel, void *buffer, size_t size);
  ssize_t write(const void *buffer, size_t size);
  uint32_t read32(uint64_t address);
  void write32(uint64_t address, uint32_t value);
  void stop();

private:
  pid_t owner;
  int control = -1;
  std::vector<int> streams;
  std::vector<uint8_t> started;
  std::vector<std::vector<char>> pending;
  std::vector<size_t> offset;
  std::mutex commands;
};
#endif
