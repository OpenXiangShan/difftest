/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __MATRIX_STORE_TRACKER_H__
#define __MATRIX_STORE_TRACKER_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <new>
#include <utility>

struct MatrixStoreRequest {
  uint64_t addr = 0;
  std::array<uint8_t, 64> data{};
  uint64_t mask = 0;
  size_t requestLane = 0;
  uint64_t requestCycle = 0;
};

struct MatrixStoreObservation {
  bool reqValid = false;
  uint64_t reqEncodedSourceId = 0;
  MatrixStoreRequest request{};
  bool respValid = false;
  uint64_t respEncodedSourceId = 0;
};

enum class MatrixStoreTrackerStatus {
  Ok,
  MalformedTag,
  DuplicateRequest,
  MissingResponse,
  AllocationFailure,
};

struct MatrixStoreFailure {
  MatrixStoreTrackerStatus status = MatrixStoreTrackerStatus::Ok;
  bool isRequest = false;
  size_t lane = 0;
  uint64_t cycle = 0;
  uint64_t encodedSourceId = 0;
};

class MatrixStoreTracker {
public:
  using AllocationHook = std::function<void()>;
  using RequestMap = std::map<uint64_t, MatrixStoreRequest>;

  explicit MatrixStoreTracker(AllocationHook allocationHook = {}) : allocationHook(std::move(allocationHook)) {}

  MatrixStoreTrackerStatus insert(uint64_t encodedSourceId, const MatrixStoreRequest &request) {
    if (!has_cwrite_tag(encodedSourceId)) {
      return MatrixStoreTrackerStatus::MalformedTag;
    }
    if (liveRequests.find(encodedSourceId) != liveRequests.end()) {
      return MatrixStoreTrackerStatus::DuplicateRequest;
    }

    try {
      if (allocationHook) {
        allocationHook();
      }
      liveRequests.emplace(encodedSourceId, request);
      return MatrixStoreTrackerStatus::Ok;
    } catch (const std::bad_alloc &) {
      return MatrixStoreTrackerStatus::AllocationFailure;
    }
  }

  MatrixStoreTrackerStatus complete(uint64_t encodedSourceId, MatrixStoreRequest &request) {
    if (!has_cwrite_tag(encodedSourceId)) {
      return MatrixStoreTrackerStatus::MalformedTag;
    }

    const auto entry = liveRequests.find(encodedSourceId);
    if (entry == liveRequests.end()) {
      return MatrixStoreTrackerStatus::MissingResponse;
    }
    request = entry->second;
    liveRequests.erase(entry);
    return MatrixStoreTrackerStatus::Ok;
  }

  template <size_t LaneCount, typename CompletionFn>
  MatrixStoreTrackerStatus process_cycle(const std::array<MatrixStoreObservation, LaneCount> &observations,
                                         uint64_t cycle, CompletionFn &&onCompletion,
                                         MatrixStoreFailure *failure = nullptr) {
    for (size_t lane = 0; lane < LaneCount; lane++) {
      const auto &observation = observations[lane];
      if (!observation.respValid) {
        continue;
      }

      MatrixStoreRequest request;
      const auto status = complete(observation.respEncodedSourceId, request);
      if (status != MatrixStoreTrackerStatus::Ok) {
        set_failure(failure, status, false, lane, cycle, observation.respEncodedSourceId);
        return status;
      }
      onCompletion(request);
    }

    for (size_t lane = 0; lane < LaneCount; lane++) {
      const auto &observation = observations[lane];
      if (!observation.reqValid) {
        continue;
      }

      MatrixStoreRequest request = observation.request;
      request.requestLane = lane;
      request.requestCycle = cycle;
      const auto status = insert(observation.reqEncodedSourceId, request);
      if (status != MatrixStoreTrackerStatus::Ok) {
        set_failure(failure, status, true, lane, cycle, observation.reqEncodedSourceId);
        return status;
      }
    }
    return MatrixStoreTrackerStatus::Ok;
  }

  MatrixStoreTrackerStatus snapshot() {
    try {
      replaySnapshot = liveRequests;
      return MatrixStoreTrackerStatus::Ok;
    } catch (const std::bad_alloc &) {
      return MatrixStoreTrackerStatus::AllocationFailure;
    }
  }

  MatrixStoreTrackerStatus restore() {
    try {
      liveRequests = replaySnapshot;
      return MatrixStoreTrackerStatus::Ok;
    } catch (const std::bad_alloc &) {
      return MatrixStoreTrackerStatus::AllocationFailure;
    }
  }

  void clear() {
    liveRequests.clear();
    replaySnapshot.clear();
  }

  size_t size() const {
    return liveRequests.size();
  }

  bool empty() const {
    return liveRequests.empty();
  }

  const RequestMap &entries() const {
    return liveRequests;
  }

private:
  static constexpr uint64_t CWriteTag = 3;
  static constexpr uint64_t SourceTagMask = 7;

  static bool has_cwrite_tag(uint64_t encodedSourceId) {
    return (encodedSourceId & SourceTagMask) == CWriteTag;
  }

  static void set_failure(MatrixStoreFailure *failure, MatrixStoreTrackerStatus status, bool isRequest, size_t lane,
                          uint64_t cycle, uint64_t encodedSourceId) {
    if (failure == nullptr) {
      return;
    }
    failure->status = status;
    failure->isRequest = isRequest;
    failure->lane = lane;
    failure->cycle = cycle;
    failure->encodedSourceId = encodedSourceId;
  }

  AllocationHook allocationHook;
  RequestMap liveRequests;
  RequestMap replaySnapshot;
};

#endif // __MATRIX_STORE_TRACKER_H__
