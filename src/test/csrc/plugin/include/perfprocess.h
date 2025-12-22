#ifndef PERFPROCESS_H
#define PERFPROCESS_H

#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <memory>
#include <array>
#include <iostream>
#include <iomanip>
#include "verilated.h"
#include "VSimTop.h"

extern std::vector<uint64_t> getIOPerfCnts(VSimTop *dut_ptr);
extern std::vector<std::string> getIOPerfNames();

class Perfprocess {
public:
  Perfprocess(VSimTop *dut_ptr, int commit_width);
  ~Perfprocess();
  uint64_t find_perfCnt(std::string perfName);
  double get_ipc();
  double get_cpi();
  void update_deg();
  int update_deg_v2();
  std::string get_trace(int i) const { 
    if (i >= traces.size()) {
      throw std::out_of_range("Index out of range");
    }
    return traces[i];
  }
  void clear_traces() { traces.clear(); }
  bool get_simulation_stats(long int dse_epoch);

private:
  VSimTop *dut_ptr = nullptr;
  std::vector<std::string> perfNames = getIOPerfNames();
  int commit_width = 4;
  std::vector<std::string> traces;
};

#endif