/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip
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

#ifndef __STOPWATCH_H
#define __STOPWATCH_H

#include <chrono>
#include <iostream>
#include <vector>

enum StopwatchType {
  CHECKERS,
  OTHERS
};

class Stopwatch {
private:
  using Clock = std::chrono::steady_clock;

  std::string name;
  StopwatchType type;

  Clock::duration accumulated = Clock::duration::zero();
  Clock::time_point start_time;
  bool is_running = false;

public:
  Stopwatch(std::string name, StopwatchType type);
  ~Stopwatch();

  void start();
  void stop();
  void reset();

  double elapsed_ms() const;
  std::string getName() const {
    return name;
  }

  static void print_stats(StopwatchType type);
};

#endif // __STOPWATCH_H
