#ifndef DIFFTEST_MATRIX_HASH_H_
#define DIFFTEST_MATRIX_HASH_H_

#include <cstdint>

// Natural-layout CRC32C completion, matching NEMU amu_hash_t.
// hi is reserved and must be zero; retain the existing field order and size.
struct MatrixHashResult {
  uint64_t lo;
  uint64_t hi;
  uint32_t bytes;
  bool operator==(const MatrixHashResult &other) const {
    return lo == other.lo && hi == other.hi && bytes == other.bytes;
  }
};

#endif // DIFFTEST_MATRIX_HASH_H_
