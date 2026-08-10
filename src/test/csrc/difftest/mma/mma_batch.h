/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#ifndef __MMA_BATCH_H__
#define __MMA_BATCH_H__

#include <cstddef>
#include <queue>
#include <vector>

class MmaBackend;
struct MmaVerificationBuffer;

/**
 * @brief Removes one bounded, consecutive compatible prefix from a FIFO.
 *
 * The first item always starts the batch. Later items are included only while
 * they remain compatible with that first item, so a type boundary is never
 * crossed and FIFO order is preserved.
 */
std::vector<MmaVerificationBuffer *> take_contiguous_batch(std::queue<MmaVerificationBuffer *> &queue,
                                                           std::size_t max_batch_size, const MmaBackend &backend);

#endif // __MMA_BATCH_H__
