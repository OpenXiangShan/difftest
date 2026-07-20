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

#ifndef __MMA_VERIFIER_H__
#define __MMA_VERIFIER_H__

#include "config.h"
#include "mma_backend/mma_backend.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

/**
 * Each Difftest instance owns one MmaVerifier. AmuExecChecker allocates a
 * verification buffer, fills it with the reference operands and DUT result,
 * then submits it to the verifier. Submission transfers ownership to the
 * verifier, whose single worker thread drains the FIFO queue and invokes the
 * compile-time-selected CPU or CUDA MmaBackend. The backend performs only the
 * numerical check; the verifier owns its lifetime and serializes all calls to
 * it on the worker thread.
 *
 * Buffers are caller-owned while being populated and verifier-owned after
 * submission. Successful buffers and all mismatches after the first are freed
 * by the worker; the first mismatching buffer is retained for DiffTest error
 * reporting and released by the verifier destructor. The queue mutex protects
 * the work queue, pending count, and stop request; condition variables wake the
 * worker and flush waiters. An atomic pointer publishes first-error state to
 * DiffTest. start() creates the worker, while stop() and destruction terminate
 * and join it.
 */

/**
 * @brief Carries the copied operands and DUT result for one asynchronous MMA check.
 *
 * See MmaVerifier's allocation and submission methods for ownership rules.
 */
struct MmaVerificationBuffer {
  DifftestAmuCtrlEvent amu_event; // AMU control event for this MMA instruction
  uint8_t *src1;                  // Pointer to source matrix 1 data
  uint8_t *src2;                  // Pointer to source matrix 2 data
  uint8_t *src3;                  // Pointer to source matrix 3 data (accumulator)
  uint8_t *dut_result;            // Pointer to DUT result data
};

/**
 * @brief Returns the element size encoded by the low two bits of a typed field.
 *
 * @param[in] typed Matrix element type encoding.
 * @return Element size in bytes.
 */
static inline size_t get_element_size(uint8_t typed) {
  switch (typed & 3) {
    case 0: // e8
      return 1;
    case 1: // e16
      return 2;
    case 2: // e32
      return 4;
    default: return 4; // Default to 32 bits if unknown
  }
}

/**
 * @brief Calculates the storage required for one MMA verification request.
 *
 * @param[in] amu_event Non-null event containing the element types and matrix geometry.
 * @return Total byte size of src1, src2, src3, and dut_result.
 */
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

/**
 * @brief Coordinates asynchronous MMA verification between DiffTest and one MmaBackend.
 *
 * AmuExecChecker is the producer: it gathers the DUT result, obtains the
 * reference operands, and submits a complete buffer. A single verifier worker
 * consumes those buffers and calls the selected CPU or CUDA backend.
 * MmaVerifier owns the backend and every buffer after submission.
 *
 * @note flush() establishes a logical completion boundary only. It leaves the
 * worker alive and its std::thread joinable, so fork integration must handle
 * the worker lifecycle separately.
 */
class MmaVerifier {
public:
  /**
   * @brief Constructs the verifier and selects the configured CPU or CUDA backend.
   */
  MmaVerifier();

  /**
   * @brief Stops and joins the worker, then releases queued buffers and the backend.
   *
   * The first mismatching buffer retained for error reporting is also released.
   */
  ~MmaVerifier();

  /**
   * @brief Starts the single verifier worker.
   *
   * @pre Call this before submitting buffers.
   * @note This is a no-op unless the previous worker has not started or has been joined.
   */
  void start();

  /**
   * @brief Requests worker exit and joins it.
   *
   * @note This does not flush queued work. Call flush() first when every
   * submitted request must be verified before the worker stops.
   */
  void stop();

  /**
   * @brief Waits until no request remains queued or executing in the backend.
   *
   * @return true if no mismatch has been recorded; false otherwise.
   * @pre The worker must be running while outstanding work remains.
   * @note The worker remains running after the call. The producer must not
   * submit new buffers before an operation that relies on the drained state.
   * @warning This is a logical-state barrier, not by itself a fork-safety barrier.
   */
  bool flush();

  /**
   * @brief Allocates a verification buffer from the event's matrix geometry.
   *
   * The event is copied into the buffer. The operand and result arrays are
   * uninitialized and must be filled by the caller before submission.
   *
   * @param[in] amu_event Non-null event describing the MMA request.
   * @return A newly allocated buffer initially owned by the caller.
   */
  MmaVerificationBuffer *allocate_buffer(const DifftestAmuCtrlEvent *amu_event);

  /**
   * @brief Releases a verification buffer and its operand/result arrays.
   *
   * @param[in] buffer Buffer to release, or nullptr.
   * @pre Ownership of buffer has not been transferred by submission.
   */
  void free_buffer(MmaVerificationBuffer *buffer);

  /**
   * @brief Submits a fully populated buffer for asynchronous verification.
   *
   * @param[in] buffer Non-null buffer returned by allocate_buffer().
   * @post Ownership is transferred to the verifier. The caller must not access
   * or free buffer after this call.
   */
  void add_to_verification_queue(MmaVerificationBuffer *buffer);

  /**
   * @brief Checks whether the backend has reported an MMA mismatch.
   *
   * @return true after any MMA verification has failed.
   */
  bool has_mma_verification_error() const;

  /**
   * @brief Gets the first mismatching verification request.
   *
   * @return A borrowed pointer to the first mismatch, or nullptr if none exists.
   * @note The verifier retains ownership. The pointer remains valid until the
   * verifier is destroyed and must not be freed or modified by the caller.
   */
  const MmaVerificationBuffer *get_error_buffer() const;

private:
  std::thread mma_verification_thread;                        // Verification thread
  std::queue<MmaVerificationBuffer *> mma_verification_queue; // Pending verification queue

  // Mutex for queue access
  std::mutex mma_queue_mutex;

  // Condition variable for queue. When it is notified, the worker thread will
  // wake up and check if there are any buffers to process.
  std::condition_variable mma_worker_cv;

  // Notifies flush waiters of completed work
  std::condition_variable mma_flush_cv;

  // Protected by mma_queue_mutex.
  // Worker thread check it to know when to exit.
  bool mma_stop_requested = false;

  // Protected by mma_queue_mutex.
  // Queued or executing count. When it reaches zero, it'll be safe to fork.
  size_t mma_pending_count = 0;

  // Buffer of the first failed MMA instruction (retained for error reporting)
  std::atomic<MmaVerificationBuffer *> mma_error_buffer{nullptr};

  // MMA verification backend (CPU by default)
  MmaBackend *backend = nullptr;

  /**
   * @brief Runs the single-consumer verification loop.
   *
   * Requests are verified in FIFO order. The worker retains the first mismatch
   * for reporting and completes later requests so flush() can observe a fully
   * drained state.
   */
  void mma_verification_thread_func();
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_VERIFIER_H__
