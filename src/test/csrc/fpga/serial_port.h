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

#pragma once

#include <thread>
#ifdef USE_SERIAL_PORT

class SerialPort {
public:
  SerialPort(const char *device) : fd_(-1), device_(device) {}
  ~SerialPort();
  void start();
  void stop();

private:
  int fd_;
  int stop_pipe_[2] = {-1, -1};
  const char *device_;
  std::thread read_thread;
  std::thread write_thread;
  bool open_port(int baudrate);
  void close_port();
  void start_read_thread();
  void start_write_thread();
};

#endif // USE_SERIAL_PORT
