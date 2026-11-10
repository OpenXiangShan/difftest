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

#include "mma/mma_batch.h"
#include "mma/backend/mma_backend.h"
#include <algorithm>

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

std::vector<MmaVerificationBuffer *> take_contiguous_batch(std::queue<MmaVerificationBuffer *> &queue,
                                                           std::size_t max_batch_size, const MmaBackend &backend) {
  std::vector<MmaVerificationBuffer *> batch;
  if (queue.empty() || max_batch_size == 0) {
    return batch;
  }

  batch.reserve(std::min(max_batch_size, queue.size()));
  MmaVerificationBuffer *first = queue.front();
  queue.pop();
  batch.push_back(first);

  while (batch.size() < max_batch_size && !queue.empty() && backend.is_batch_compatible(first, queue.front())) {
    batch.push_back(queue.front());
    queue.pop();
  }
  return batch;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
