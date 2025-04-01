#ifndef __TRACE_GLOBAL_HISTORY_H__
#define __TRACE_GLOBAL_HISTORY_H__

#include <bitset>
#include <cstdint>

class GlobalHistory {

private:

public:
  std::bitset<120> history;

  GlobalHistory() {
    history.reset();
  }

  void reset() {
    history.reset();
  }

  uint64_t getPart(uint8_t from, uint8_t to) {
    uint64_t result = 0;
    for (int i = from; i < to; i ++) {
      uint64_t tmp = history[i] ? 0x1 : 0x0;
      result <<= 1;
      result |= tmp;
    }
    return result;
  }

  size_t get_folded_history(size_t history_len, size_t unit_len) {
    if (unit_len == 0) {
      fprintf(stderr, "Error, unit_len is 0\n");
      exit(1);
    }

    size_t folded_history = 0;
    for (size_t i = 0; i < history_len; i += unit_len) {
      folded_history ^= getPart(i, std::min(i+unit_len, history_len));
    }
    return folded_history;
  }

  void update(bool taken) {
    history <<= 1;
    if (taken) history.set(0);
    else history.reset(0);
  }
};

class BankTickCounter {
protected:

  typedef uint8_t bank_ctr_t;
  int BC_Bits = 7;
  bank_ctr_t bankCounter = 0;

public:
  BankTickCounter(int bits) : BC_Bits(bits) {}

  bool updateAndIsFull(uint8_t inc) {
    if (bankCounter + inc >= (1 << BC_Bits)) {
      bankCounter = 0;
      return true;
    } else {
      bankCounter += inc;
      return false;
    }
  }

  void decrease(uint8_t dec) {
    if (bankCounter >= dec) {
      bankCounter -= dec;
    } else {
      bankCounter = 0;
    }
  }
};

#endif