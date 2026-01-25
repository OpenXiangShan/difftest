#include "stopwatch.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>

std::vector<Stopwatch *> all_stopwatches;
std::vector<std::string> StopwatchTypeNames = {"Difftest Checkers", "Others"};

Stopwatch::Stopwatch(std::string name, StopwatchType type) : name(std::move(name)), type(type) {
  all_stopwatches.push_back(this);
}

Stopwatch::~Stopwatch() {
  auto it = std::remove(all_stopwatches.begin(), all_stopwatches.end(), this);
  all_stopwatches.erase(it, all_stopwatches.end());
}

void Stopwatch::start() {
  if (!is_running) {
    start_time = Clock::now();
    is_running = true;
  }
}

void Stopwatch::stop() {
  if (is_running) {
    auto end_time = Clock::now();
    accumulated += end_time - start_time;
    is_running = false;
  }
}

void Stopwatch::reset() {
  accumulated = Clock::duration::zero();
  is_running = false;
}

double Stopwatch::elapsed_ms() const {
  auto total_duration = accumulated;
  if (is_running) {
    auto current_time = Clock::now();
    total_duration += current_time - start_time;
  }
  return std::chrono::duration<double, std::milli>(total_duration).count();
}

void Stopwatch::print_stats(StopwatchType type) {
  std::map<std::string, double> stats;
  double total_sum = 0;

  for (auto *c: all_stopwatches) {
    auto time = c->elapsed_ms();
    if (c->type == type && time > 0) {
      stats[c->name] += time;
      total_sum += time;
    }
  }

  using Pair = std::pair<std::string, double>;
  std::vector<Pair> sorted_stats(stats.begin(), stats.end());

  // sort by time descending
  std::sort(sorted_stats.begin(), sorted_stats.end(), [](const Pair &a, const Pair &b) { return a.second > b.second; });

  size_t type_index = static_cast<size_t>(type);
  std::string type_name = (type_index < StopwatchTypeNames.size()) ? StopwatchTypeNames[type_index] : "Unknown";
  std::cout << "\n===== DiffTest " << type_name << " Performance Stats =====" << std::endl;
  std::cout << std::left << std::setw(40) << "Name" << "Time (ms)" << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  for (const auto &p: sorted_stats) {
    std::cout << std::left << std::setw(40) << p.first << p.second << std::endl;
  }
  std::cout << std::string(60, '-') << std::endl;
  std::cout << std::left << std::setw(40) << "TOTAL" << total_sum << std::endl;
  std::cout << "==============================================\n" << std::endl;
}
