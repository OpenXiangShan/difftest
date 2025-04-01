#ifndef TRACE_RAS_H
#define TRACE_RAS_H

#include <cstdio>
#include <cstdint>
#include <stack>

#include "debug_macro.h"

// when call, push sequent next pc
// when ret, pop
class TraceRAS {
protected:
  std::stack<uint64_t> ras;
  uint64_t overful_num = 0;
  const uint64_t RAS_SIZE = 16;

public:
  void push(uint64_t seq_pc) {
    if (ras.size() < RAS_SIZE) {
#ifdef RAS_DEBUG
      fprintf(stderr, "  [RAS Push] seq_pc %lx\n", seq_pc);
#endif
      ras.push(seq_pc);
    } else {
#ifdef RAS_DEBUG
      fprintf(stderr, "  [RAS Push] seq_pc %lx overfull %ld\n", seq_pc, overful_num + 1);
#endif
      overful_num ++;
    }
  }

  bool pop(uint64_t &target) {
    if (overful_num > 0) {
#ifdef RAS_DEBUG
      fprintf(stderr, "  [RAS Pop] overfull %ld\n", overful_num + 1);
#endif
      overful_num --;
      return false;
    } else if (ras.size() > 0){
      target = ras.top();
#ifdef RAS_DEBUG
      fprintf(stderr, "  [RAS Pop] target %lx\n", target);
#endif
      ras.pop();
      return true;
    }
#ifdef RAS_DEBUG
      fprintf(stderr, "  [RAS Pop] empty\n");
#endif
    return false;
  }
};

#endif // TRACE_RAS_H