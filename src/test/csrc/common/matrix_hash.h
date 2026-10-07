#ifndef DIFFTEST_MATRIX_HASH_H_
#define DIFFTEST_MATRIX_HASH_H_

#include <cstdint>

// Natural-layout mload/mzero hash ABI, matching NEMU amu_hash_t.
struct MatrixHash128 {
  uint64_t lo;
  uint64_t hi;
  uint32_t bytes;
  bool operator==(const MatrixHash128 &other) const {
    return lo == other.lo && hi == other.hi && bytes == other.bytes;
  }
};

#endif // DIFFTEST_MATRIX_HASH_H_
