#ifndef __TRACE_BRANCH_TYPE_FIXER_H__
#define __TRACE_BRANCH_TYPE_FIXER_H__

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "../trace_common.h"

class TraceBranchTypeFixer {
private:
  std::map<uint32_t, uint8_t> cached_map;
  std::map<uint32_t, bool> cached_nonret_jr_map;

  inline bool is_c_j(uint32_t inst_encoding) {
    return ((inst_encoding & 0x3) == 0x1) &&
           (((inst_encoding >> 13) & 0x7) == 0x5);
  }

  inline bool is_c_jal(uint32_t inst_encoding) {
    return ((inst_encoding & 0x3) == 0x1) &&
           (((inst_encoding >> 13) & 0x7) == 0x1);
  }

  inline bool is_c_jalr(uint32_t inst_encoding) {
    return ((inst_encoding & 0x3) == 0x2) &&
            (((inst_encoding >> 2) & 0x1f) == 0) &&
            (((inst_encoding >> 7) & 0x1f) != 0) &&
            (((inst_encoding >> 12) & 0xf) == 0x9);
  }

  inline bool is_jal(uint32_t inst_encoding) {
    return ((inst_encoding & 0x7f) == 0x6f);
  }

  inline bool is_jalr(uint32_t inst_encoding) {
    return ((inst_encoding & 0x7f) == 0x67) &&
           (((inst_encoding >> 12) & 0x7) == 0);
  }

  inline bool is_c_jr(uint32_t inst_encoding) {
    return ((inst_encoding & 0x3) == 0x2) &&
            (((inst_encoding >> 2) & 0x1f) == 0) &&
            (((inst_encoding >> 7) & 0x1f) != 0) &&
            (((inst_encoding >> 12) & 0xf) == 0x8);
  }

  inline bool is_c(uint32_t inst_encoding) {
    return ((inst_encoding & 0x3) != 0x3);
  }

public:
  bool is_nonret_jr(uint32_t inst_encoding, uint8_t branch_type) {
    if (branch_type == BRANCH_None || branch_type == BRANCH_Cond) {
      return false;
    }

    if (cached_nonret_jr_map.find(inst_encoding) != cached_nonret_jr_map.end()) {
      return cached_nonret_jr_map[inst_encoding];
    }

    bool is_rvc_inst = is_c(inst_encoding);
    bool is_c_j_inst = is_c_j(inst_encoding);
    bool is_c_jal_inst = is_c_jal(inst_encoding);
    bool is_c_jalr_inst = is_c_jalr(inst_encoding);
    bool is_c_jr_inst = is_c_jr(inst_encoding);


    bool is_jal_inst = is_jal(inst_encoding) || is_c_j_inst || is_c_jal_inst;
    bool is_jalr_inst = is_jalr(inst_encoding) || is_c_jalr_inst || is_c_jr_inst;

    // printf("inst_encoding: %x\n", inst_encoding);
    // printf("rvc %d c_j %d c_jal %d c_jalr %d c_jr %d jal %d jalr %d\n", is_rvc_inst, is_c_j_inst, is_c_jal_inst, is_c_jalr_inst, is_c_jr_inst, is_jal_inst, is_jalr_inst);

    uint8_t rd = is_rvc_inst ? (inst_encoding >> 12) & 0x1 :
                          (inst_encoding >> 7) & 0x1f;
    bool rd_isLink = rd == 1 || rd == 5;

    // printf("rd %d isLink %d\n", rd, rd_isLink);

    uint8_t rs_rvc = (is_c_j_inst || is_c_jal_inst) ? 0 :
                      (inst_encoding >> 7) & 0x1f;
    uint8_t rs = is_rvc_inst ? rs_rvc : (inst_encoding >> 15) & 0x1f;
    uint8_t rs_isLink = rs == 1 || rs == 5;


    // printf("rs %d rs_rvc %d isLink %d\n", rs, rs_rvc);

    bool isCall = rd_isLink && ((is_jal_inst && !is_rvc_inst) || is_jalr_inst);
    bool isReturn = rs_isLink && is_jalr_inst && !isCall;

    // printf("isCall %d isReturn %d\n", isCall, isReturn);
    bool is_nonret_jr = is_jalr_inst && !isReturn;
    cached_nonret_jr_map[inst_encoding] = is_nonret_jr;
    return is_nonret_jr;
  }

  uint8_t fix_branch_type(uint32_t inst_encoding, uint8_t branch_type) {

    if (branch_type == BRANCH_None || branch_type == BRANCH_Cond) {
      return branch_type;
    }

    if (cached_map.find(inst_encoding) != cached_map.end()) {
      return cached_map[inst_encoding];
    }

    bool is_rvc_inst = is_c(inst_encoding);
    bool is_c_j_inst = is_c_j(inst_encoding);
    bool is_c_jal_inst = is_c_jal(inst_encoding);
    bool is_c_jalr_inst = is_c_jalr(inst_encoding);
    bool is_c_jr_inst = is_c_jr(inst_encoding);

    bool is_jal_inst = is_jal(inst_encoding) || is_c_j_inst || is_c_jal_inst;
    bool is_jalr_inst = is_jalr(inst_encoding) || is_c_jalr_inst || is_c_jr_inst;

    // printf("inst_encoding: %x\n", inst_encoding);
    // printf("rvc %d c_j %d c_jal %d c_jalr %d c_jr %d jal %d jalr %d\n", is_rvc_inst, is_c_j_inst, is_c_jal_inst, is_c_jalr_inst, is_c_jr_inst, is_jal_inst, is_jalr_inst);

    uint8_t rd = is_rvc_inst ? (inst_encoding >> 12) & 0x1 :
                          (inst_encoding >> 7) & 0x1f;
    bool rd_isLink = rd == 1 || rd == 5;

    // printf("rd %d isLink %d\n", rd, rd_isLink);

    uint8_t rs_rvc = (is_c_j_inst || is_c_jal_inst) ? 0 :
                      (inst_encoding >> 7) & 0x1f;
    uint8_t rs = is_rvc_inst ? rs_rvc : (inst_encoding >> 15) & 0x1f;
    uint8_t rs_isLink = rs == 1 || rs == 5;

    // printf("rs %d rs_rvc %d isLink %d\n", rs, rs_rvc);

    bool isCall = rd_isLink && ((is_jal_inst && !is_rvc_inst) || is_jalr_inst);
    bool isReturn = rs_isLink && is_jalr_inst && !isCall;

    // printf("isCall %d isReturn %d\n", isCall, isReturn);

    if (isCall) {
      cached_map[inst_encoding] = BRANCH_Call;
      return BRANCH_Call;
    } else if (isReturn) {
      cached_map[inst_encoding] = BRANCH_Return;
      return BRANCH_Return;
    } else {
      cached_map[inst_encoding] = BRANCH_Uncond;
      return BRANCH_Uncond;
    }
  }
};

#endif // __TRACE_BRANCH_TYPE_FIXER_H__
