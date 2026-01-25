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
