#ifndef __PERFPROCESS_H
#define __PERFPROCESS_H

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
#include "isa_parser.h"
#include "disasm.h"

extern int get_perfCnt_id(std::string perfName);

#define DEFAULT_ISA  "rv64imafdc_zicntr_zihpm"
#define DEFAULT_PRIV "MSU"

class Perfprocess {
public:
  Perfprocess(int commit_width);
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
  std::vector<uint64_t> perfCnts;
  bool get_simulation_stats(long int dse_epoch);

private:
  int commit_width = 6;
  isa_parser_t isa_parser;
  disassembler_t* disassembler;
  std::vector<std::string> traces;
};

#endif