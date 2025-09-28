/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef SIMFRONTEND_H
#define SIMFRONTEND_H

#include <cstdint>
#include <string>

// Align with the functions defined in the blackbox within chisel.
extern "C" void SimFrontFetch(int offset, uint64_t *pc, uint32_t *instr, uint32_t *preDecode);
extern "C" void SimFrontUpdatePtr(uint32_t updateCount);
extern "C" void SimFrontRedirect(uint32_t redirect_valid, uint32_t redirect_ftq_flag, uint32_t redirect_ftq_value,
                                 uint32_t redirect_type, uint64_t redirect_pc, uint64_t redirect_target);
extern "C" void SimFrontGetFtqToBackEnd(uint64_t *pc, uint32_t *pack_data, uint64_t *newest_pc,
                                        uint32_t *newest_pack_data);
extern "C" void SimFrontRobCommit(uint32_t valid, uint32_t ftqIdxFlag, uint32_t ftqIdxValue);

bool init_sim_frontend(const std::string &file);
#endif // SIMFRONTEND_H
