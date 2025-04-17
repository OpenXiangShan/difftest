#ifndef __TRACE_HISTORY_DEDUPER_H__
#define __TRACE_HISTORY_DEDUPER_H__

#include <bitset>
#include <vector>
#include <map>

#include "spikedasm.h"
#include "trace_common.h"
#include "trace_format.h"

#define HISTORY_DEDUPER_DEBUG

#define HISTORY_LENGTH (119)
#define SIGNATURE_LEFT_NUM (10)

class TraceHistoryLabeler {
private:
  // std::bitset<HISTORY_LENGTH> history;
  uint64_t history[2];

  struct BranchSignature {
    uint64_t pc;
    uint64_t target;
    // std::bitset<HISTORY_LENGTH> history;
    uint64_t history0;
    uint64_t history1;
    bool taken;

    // BranchSignature() {}
    BranchSignature(uint64_t pc, uint64_t target,
      uint64_t history0, uint64_t history1, bool taken) :
      pc(pc), target(target),
      history0(history0), history1(history1), taken(taken) {}
    bool operator=(BranchSignature &other) {
      return (pc == other.pc) &&
             (target == other.target) &&
             (history0 == other.history0) &&
             (history1 == other.history1) &&
             (taken == other.taken);
    }

    bool operator<(const BranchSignature& other) const {
      if (pc != other.pc) return pc < other.pc;
      if (target != other.target) return target < other.target;
      if (history1 != other.history1) return history1 < other.history1;
      if (history0 != other.history0) return history0 < other.history0;
      return taken < other.taken;
    }
  };
  std::vector<BranchSignature> signatures;
  std::vector<bool> branch_filtered;
  std::map<BranchSignature, uint64_t> signature_count;

  void updateHistory(bool taken) {
    history[1] <<= 1;
    history[1] |= history[0] >> 63;
    history[1] &= (1UL << (HISTORY_LENGTH - 64)) - 1;

    history[0] <<= 1;
    uint64_t new_coming = taken ? 1 : 0;
    history[0] |= new_coming;
    // history.set(0, taken);
  }

public:
  TraceHistoryLabeler() {
    history[0] = 0;
    history[1] = 0;

    if (HISTORY_LENGTH > 123) {
      printf("HISTORY_LENGTH %d too large, use more uint64_t\n", HISTORY_LENGTH);
      fflush(stdout);
      exit(1);
    }
  }

  void assignLabel(const std::vector<Instruction> &src, size_t from_index, size_t end_index, std::vector<bool> &branch_dedupable_list) {
    // read all the src history until end_index, ignore from_index
    if (branch_dedupable_list.size() != 0) {
      printf("[Error] origin branch_dedupable_list size %lu is not empty\n", branch_dedupable_list.size());
    }

    for (size_t i = 0; i < end_index; i++) {
      if (src[i].branch_type != BRANCH_None) {
        signatures.emplace_back(
          BranchSignature(src[i].instr_pc_va,
                          src[i].target,
                          history[0],
                          history[1],
                          src[i].branch_taken)
        );
        updateHistory(src[i].branch_taken);
      }
    }
    printf("Labeler collect all the signatures %lu\n", signatures.size());

    std::map<BranchSignature, uint64_t> signature_map;
    std::vector<bool> is_unique_signature_list(signatures.size(), false);
    for (int i = signatures.size() - 1; i >= 0; i--) {
      // only the youngest signatures is left, other signatures are filtered
      if (signature_map.find(signatures[i]) == signature_map.end()) {
        signature_map[signatures[i]] = 1;
        is_unique_signature_list[i] = true;
      } else if (signature_map[signatures[i]] < SIGNATURE_LEFT_NUM) {
        signature_map[signatures[i]]++;
        is_unique_signature_list[i] = true;
      } else {
        is_unique_signature_list[i] = false;
      }
    }

    printf("Labler assign signature_map size %lu\n", signature_map.size());

    size_t non_dedupable_num = 0;
    branch_dedupable_list.resize(signatures.size(), true);
    for (int i = is_unique_signature_list.size() -1; i >= 0; i--) {
      if (is_unique_signature_list[i]) {
        for (int j = 0; j <= i && j < HISTORY_LENGTH; j++) {
          if (branch_dedupable_list[i - j]) {
            non_dedupable_num ++;
          }
          branch_dedupable_list[i - j] = false;
        }
      }
    }

    printf("Labler: branch instruction num %lu non-dedupable %lu dedupable %lu\n", branch_dedupable_list.size(), non_dedupable_num, branch_dedupable_list.size()- non_dedupable_num);
  }

  size_t tool_signature_count(const std::vector<Instruction> &src, size_t end_index) {
    std::map<BranchSignature, uint64_t> signature_map;
    uint64_t history_tool[2];
    // NOTE: start from 0;
    for (size_t i = 0; i < end_index; i++) {
      if (src[i].branch_type != BRANCH_None && !src[i].is_squashed) {
        signature_map[BranchSignature(src[i].instr_pc_va,
                          src[i].target,
                          history_tool[0],
                          history_tool[1],
                          src[i].branch_taken)] ++;

        history_tool[1] <<= 1;
        history_tool[1] |= history[0] >> 63;
        history_tool[1] &= (1UL << (HISTORY_LENGTH - 64)) - 1;

        history_tool[0] <<= 1;
        uint64_t new_coming = src[i].branch_taken ? 1 : 0;
        history_tool[0] |= new_coming;
      }
    }
    return signature_map.size();
  }
};

#endif