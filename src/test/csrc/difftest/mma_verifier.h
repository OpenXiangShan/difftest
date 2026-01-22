/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#ifndef __MMA_VERIFIER_H__
#define __MMA_VERIFIER_H__

#include "common.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include "refproxy.h"

// MMA verification buffer structure
typedef struct {
  DifftestAmuCtrlEvent amu_event;  // AMU control event for this MMA instruction
  uint8_t *src1;                   // Pointer to source matrix 1 data
  uint8_t *src2;                   // Pointer to source matrix 2 data
  uint8_t *src3;                   // Pointer to source matrix 3 data (accumulator)
  uint8_t *dut_result;             // Pointer to DUT result data
  bool in_use;                     // Whether this buffer is currently in use
} MmaVerificationBuffer;

// Calculate element size based on typed field
static inline size_t get_element_size(uint8_t typed) {
  switch (typed & 3) {
    case 0:  // e8
      return 1;
    case 1:  // e16
      return 2;
    case 2:  // e32
      return 4;
    default:
      return 4;  // Default to 32 bits if unknown
  }
}

// Calculate total buffer size needed for MMA verification
static inline size_t calculate_mma_buffer_size(const DifftestAmuCtrlEvent *amu_event) {
  size_t element_size_s1 = get_element_size(amu_event->types1);
  size_t element_size_s2 = get_element_size(amu_event->types2);
  size_t element_size_d = get_element_size(amu_event->typed);

  uint16_t m = amu_event->mtilem;
  uint16_t n = amu_event->mtilen;
  uint16_t k = amu_event->mtilek;
  
  // Calculate size for each matrix:
  // src1: m×k matrix
  // src2: k×n matrix  
  // src3: m×n matrix (accumulator)
  // result: m×n matrix
  size_t src1_size = element_size_s1 * m * k;
  size_t src2_size = element_size_s2 * k * n;
  size_t src3_size = element_size_d * m * n;
  size_t result_size = element_size_d * m * n;
  
  // Total buffer size
  return src1_size + src2_size + src3_size + result_size;
}

class MmaVerifier {
public:
  MmaVerifier(REF_PROXY *proxy);
  ~MmaVerifier();
  
  // Thread management methods
  void start();
  void stop();
  
  // Buffer management methods
  MmaVerificationBuffer* allocate_buffer(const DifftestAmuCtrlEvent *amu_event);
  void free_buffer(MmaVerificationBuffer *buffer);
  
  // Add buffer to verification queue
  void add_to_verification_queue(MmaVerificationBuffer *buffer);
  
private:
  // Reference proxy for getting reference results
  REF_PROXY *proxy;
  
  // MMA verification thread related
  std::thread mma_verification_thread;      // Verification thread
  std::atomic<bool> mma_thread_running;      // Thread running flag
  std::queue<MmaVerificationBuffer*> mma_verification_queue; // Buffer queue
  std::mutex mma_queue_mutex;                // Mutex for queue access
  std::condition_variable mma_queue_cv;      // Condition variable for queue
  
  // MMA verification buffer management
  std::vector<MmaVerificationBuffer*> mma_buffers;
  
  // Thread function
  void mma_verification_thread_func();

  bool mmacc(MmaVerificationBuffer *buffer);
  bool mmaccu(MmaVerificationBuffer *buffer);
  bool mmaccus(MmaVerificationBuffer *buffer);
  bool mmaccsu(MmaVerificationBuffer *buffer);
  bool mfmacc(MmaVerificationBuffer *buffer);
};

#endif // __MMA_VERIFIER_H__
