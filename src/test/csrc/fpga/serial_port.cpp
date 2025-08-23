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

#include "serial_port.h"
#include <fcntl.h>
#include <iostream>
#include <sys/select.h>

#ifdef USE_SERIAL_PORT

bool SerialPort::open_port(int baudrate) {
  fd_ = open(device_, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0) {
    std::cerr << "Failed to open " << device_ << std::endl;
    return false;
  }
  struct termios tty;
  memset(&tty, 0, sizeof tty);
  if (tcgetattr(fd_, &tty) != 0) {
    std::cerr << "Error from tcgetattr" << std::endl;
    return false;
  }
  cfsetospeed(&tty, baudrate);
  cfsetispeed(&tty, baudrate);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
  tty.c_iflag &= ~IGNBRK;                     // disable break processing
  tty.c_lflag = 0;                            // no signaling chars, no echo,
                                              // no canonical processing
  tty.c_oflag = 0;                            // no remapping, no delays
  tty.c_cc[VMIN] = 1;                         // read blocks
  tty.c_cc[VTIME] = 1;                        // 0.1 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                     // enable reading
  tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    std::cerr << "Error from tcsetattr" << std::endl;
    return false;
  }
  return true;
}

void SerialPort::close_port() {
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

SerialPort::~SerialPort() {
  close_port();
}
void SerialPort::start_read_thread() {
  char buf[256];
  try {
    printf("SerailPort: start read from %s\n", device_);
    setvbuf(stdout, NULL, _IONBF, 0);
    while (running) {
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(fd_, &readfds);
      int ret = select(fd_ + 1, &readfds, nullptr, nullptr, nullptr);
      if (ret > 0 && FD_ISSET(fd_, &readfds)) {
        ssize_t n = read(fd_, buf, sizeof(buf) - 1);
        if (n > 0) {
          buf[n] = '\0';
          printf("%s", buf);
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "SerialPort: exception " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "SerialPort: unknown exception" << std::endl;
  }
}

void SerialPort::start_write_thread() {
  try {
    printf("SerialPort: start write to %s\n", device_);
    std::string line;
    while (running) {
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(STDIN_FILENO, &readfds);
      timeval tv{1, 0}; // eheck timeout per second
      int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
      if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
        std::string line;
        if (std::getline(std::cin, line)) {
          line.push_back('\n');
          write(fd_, line.c_str(), line.size());
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "SerialPort: exception " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "SerialPort: unknown exception" << std::endl;
  }
}

#endif // USE_SERIAL_PORT
