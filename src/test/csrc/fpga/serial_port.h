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

#include <cstddef>
#include <cstring>
#include <termios.h>
#include <thread>
#include <unistd.h>
#ifdef USE_SERIAL_PORT

class SerialPort {
public:
  SerialPort(const char *device) : fd_(-1), device_(device) {}
  ~SerialPort();
  void start() {
    running = true;
    open_port(B115200);
    read_thread = std::thread(&SerialPort::start_read_thread, this);
    write_thread = std::thread(&SerialPort::start_write_thread, this);
  }
  void stop() {
    running = false;
    if (read_thread.joinable()) {
      read_thread.join();
    }
    if (write_thread.joinable()) {
      write_thread.join();
    }
  }

private:
  bool running = false;
  int fd_;
  const char *device_;
  std::thread read_thread;
  std::thread write_thread;
  bool open_port(int baudrate);
  void close_port();
  void start_read_thread();
  void start_write_thread();
};

#endif // USE_SERIAL_PORT
