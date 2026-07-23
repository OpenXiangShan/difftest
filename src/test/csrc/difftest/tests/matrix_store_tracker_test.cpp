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

#include "matrix_store_tracker.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

#define CHECK(condition)                                                                   \
  do {                                                                                     \
    if (!(condition)) {                                                                    \
      std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
      return false;                                                                        \
    }                                                                                      \
  } while (0)

MatrixStoreRequest make_request(uint64_t seed) {
  MatrixStoreRequest request{};
  request.addr = 0x80000000ULL + seed * 64;
  request.mask = 0xffffffffffffffffULL ^ seed;
  for (size_t byte = 0; byte < request.data.size(); byte++) {
    request.data[byte] = static_cast<uint8_t>(seed + byte);
  }
  return request;
}

bool same_payload(const MatrixStoreRequest &lhs, const MatrixStoreRequest &rhs) {
  return lhs.addr == rhs.addr && lhs.data == rhs.data && lhs.mask == rhs.mask;
}

bool test_more_than_legacy_capacity_and_out_of_order_completion() {
  constexpr size_t request_count = 129;
  MatrixStoreTracker tracker;
  std::vector<uint64_t> ids;
  std::vector<MatrixStoreRequest> requests;
  ids.reserve(request_count);
  requests.reserve(request_count);

  for (size_t index = 0; index < request_count; index++) {
    const uint64_t id = index == 0 ? 0x3803ULL : ((0x1000ULL + index * 0x101ULL) << 3) | 3ULL;
    const MatrixStoreRequest request = make_request(index + 1);
    std::array<MatrixStoreObservation, 1> observations{};
    observations[0].reqValid = true;
    observations[0].reqEncodedSourceId = id;
    observations[0].request = request;
    size_t completion_count = 0;

    CHECK(tracker.process_cycle(observations, index, [&](const MatrixStoreRequest &) { completion_count++; }) ==
          MatrixStoreTrackerStatus::Ok);
    CHECK(completion_count == 0);
    ids.push_back(id);
    requests.push_back(request);
  }

  CHECK(tracker.size() == request_count);

  size_t completion_count = 0;
  bool payload_matches = true;
  for (size_t offset = 0; offset < request_count; offset++) {
    const size_t index = request_count - 1 - offset;
    std::array<MatrixStoreObservation, 1> observations{};
    observations[0].respValid = true;
    observations[0].respEncodedSourceId = ids[index];

    CHECK(tracker.process_cycle(observations, request_count + offset, [&](const MatrixStoreRequest &completed) {
      payload_matches = payload_matches && same_payload(completed, requests[index]);
      completion_count++;
    }) == MatrixStoreTrackerStatus::Ok);
  }

  CHECK(payload_matches);
  CHECK(completion_count == request_count);
  CHECK(tracker.empty());
  return true;
}

bool test_completion_only_after_response_and_id_reuse() {
  MatrixStoreTracker tracker;
  constexpr uint64_t id = 0x3803ULL;
  const MatrixStoreRequest first = make_request(1);
  const MatrixStoreRequest second = make_request(2);
  size_t completion_count = 0;
  bool payload_matches = true;

  std::array<MatrixStoreObservation, 1> request_cycle{};
  request_cycle[0].reqValid = true;
  request_cycle[0].reqEncodedSourceId = id;
  request_cycle[0].request = first;
  CHECK(tracker.process_cycle(request_cycle, 10, [&](const MatrixStoreRequest &) { completion_count++; }) ==
        MatrixStoreTrackerStatus::Ok);
  CHECK(completion_count == 0);

  std::array<MatrixStoreObservation, 1> response_cycle{};
  response_cycle[0].respValid = true;
  response_cycle[0].respEncodedSourceId = id;
  CHECK(tracker.process_cycle(response_cycle, 11, [&](const MatrixStoreRequest &completed) {
    payload_matches = payload_matches && same_payload(completed, first);
    completion_count++;
  }) == MatrixStoreTrackerStatus::Ok);
  CHECK(completion_count == 1);

  request_cycle[0].request = second;
  CHECK(tracker.process_cycle(request_cycle, 12, [&](const MatrixStoreRequest &) { completion_count++; }) ==
        MatrixStoreTrackerStatus::Ok);
  CHECK(tracker.process_cycle(response_cycle, 13, [&](const MatrixStoreRequest &completed) {
    payload_matches = payload_matches && same_payload(completed, second);
    completion_count++;
  }) == MatrixStoreTrackerStatus::Ok);
  CHECK(payload_matches);
  CHECK(completion_count == 2);
  CHECK(tracker.empty());
  return true;
}

bool test_duplicate_request_preserves_original_payload() {
  MatrixStoreTracker tracker;
  constexpr uint64_t id = 0x3803ULL;
  const MatrixStoreRequest original = make_request(3);
  const MatrixStoreRequest duplicate = make_request(4);
  MatrixStoreFailure failure;

  std::array<MatrixStoreObservation, 2> observations{};
  observations[0].reqValid = true;
  observations[0].reqEncodedSourceId = id;
  observations[0].request = original;
  CHECK(tracker.process_cycle(observations, 20, [](const MatrixStoreRequest &) {}) == MatrixStoreTrackerStatus::Ok);

  observations = {};
  observations[1].reqValid = true;
  observations[1].reqEncodedSourceId = id;
  observations[1].request = duplicate;
  CHECK(tracker.process_cycle(
            observations, 21, [](const MatrixStoreRequest &) {}, &failure) ==
        MatrixStoreTrackerStatus::DuplicateRequest);
  CHECK(failure.isRequest);
  CHECK(failure.lane == 1);
  CHECK(failure.encodedSourceId == id);
  CHECK(tracker.size() == 1);

  bool payload_matches = false;
  observations = {};
  observations[0].respValid = true;
  observations[0].respEncodedSourceId = id;
  CHECK(tracker.process_cycle(observations, 22, [&](const MatrixStoreRequest &completed) {
    payload_matches = same_payload(completed, original);
  }) == MatrixStoreTrackerStatus::Ok);
  CHECK(payload_matches);
  return true;
}

bool test_response_tag_and_allocation_failures() {
  constexpr uint64_t id = 0x400bULL;
  MatrixStoreTracker tracker;
  MatrixStoreFailure failure;
  size_t completion_count = 0;
  std::array<MatrixStoreObservation, 2> observations{};

  observations[0].respValid = true;
  observations[0].respEncodedSourceId = id;
  CHECK(tracker.process_cycle(
            observations, 30, [&](const MatrixStoreRequest &) { completion_count++; }, &failure) ==
        MatrixStoreTrackerStatus::MissingResponse);
  CHECK(!failure.isRequest);
  CHECK(completion_count == 0);

  observations = {};
  observations[0].reqValid = true;
  observations[0].reqEncodedSourceId = id;
  observations[0].request = make_request(5);
  CHECK(tracker.process_cycle(observations, 31, [](const MatrixStoreRequest &) {}) == MatrixStoreTrackerStatus::Ok);
  observations = {};
  observations[0].respValid = true;
  observations[0].respEncodedSourceId = id;
  observations[1].respValid = true;
  observations[1].respEncodedSourceId = id;
  CHECK(tracker.process_cycle(
            observations, 32, [&](const MatrixStoreRequest &) { completion_count++; }, &failure) ==
        MatrixStoreTrackerStatus::MissingResponse);
  CHECK(completion_count == 1);
  CHECK(failure.lane == 1);

  observations = {};
  observations[0].reqValid = true;
  observations[0].reqEncodedSourceId = 0x3801ULL;
  CHECK(tracker.process_cycle(
            observations, 33, [](const MatrixStoreRequest &) {}, &failure) == MatrixStoreTrackerStatus::MalformedTag);
  CHECK(failure.isRequest);
  observations = {};
  observations[0].respValid = true;
  observations[0].respEncodedSourceId = 0x3801ULL;
  CHECK(tracker.process_cycle(
            observations, 34, [](const MatrixStoreRequest &) {}, &failure) == MatrixStoreTrackerStatus::MalformedTag);
  CHECK(!failure.isRequest);

  MatrixStoreTracker allocation_failure_tracker([]() { throw std::bad_alloc(); });
  observations = {};
  observations[0].reqValid = true;
  observations[0].reqEncodedSourceId = id;
  CHECK(allocation_failure_tracker.process_cycle(
            observations, 35, [](const MatrixStoreRequest &) {}, &failure) ==
        MatrixStoreTrackerStatus::AllocationFailure);
  CHECK(allocation_failure_tracker.empty());
  return true;
}

bool test_outstanding_context_and_clear() {
  MatrixStoreTracker tracker;
  std::array<MatrixStoreObservation, 2> observations{};
  observations[1].reqValid = true;
  observations[1].reqEncodedSourceId = 0x3803ULL;
  observations[1].request = make_request(6);
  CHECK(tracker.process_cycle(observations, 40, [](const MatrixStoreRequest &) {}) == MatrixStoreTrackerStatus::Ok);
  CHECK(tracker.entries().begin()->second.requestLane == 1);
  CHECK(tracker.entries().begin()->second.requestCycle == 40);
  tracker.clear();
  CHECK(tracker.empty());
  return true;
}

bool test_multilane_cross_lane_completion_and_core_isolation() {
  MatrixStoreTracker core0;
  MatrixStoreTracker core1;
  constexpr size_t lane_count = 4;
  std::array<uint64_t, lane_count> ids = {0x3803ULL, 0x400bULL, 0x4813ULL, 0x501bULL};
  std::array<MatrixStoreRequest, lane_count> requests{};
  std::array<MatrixStoreObservation, lane_count> observations{};
  for (size_t lane = 0; lane < lane_count; lane++) {
    requests[lane] = make_request(10 + lane);
    observations[lane].reqValid = true;
    observations[lane].reqEncodedSourceId = ids[lane];
    observations[lane].request = requests[lane];
  }
  CHECK(core0.process_cycle(observations, 50, [](const MatrixStoreRequest &) {}) == MatrixStoreTrackerStatus::Ok);

  observations = {};
  const std::array<size_t, lane_count> response_order = {2, 0, 3, 1};
  for (size_t lane = 0; lane < lane_count; lane++) {
    observations[lane].respValid = true;
    observations[lane].respEncodedSourceId = ids[response_order[lane]];
  }
  size_t completion_count = 0;
  bool payload_matches = true;
  CHECK(core0.process_cycle(observations, 51, [&](const MatrixStoreRequest &completed) {
    payload_matches = payload_matches && same_payload(completed, requests[response_order[completion_count]]);
    completion_count++;
  }) == MatrixStoreTrackerStatus::Ok);
  CHECK(payload_matches);
  CHECK(completion_count == lane_count);

  std::array<MatrixStoreObservation, 1> same_id{};
  same_id[0].reqValid = true;
  same_id[0].reqEncodedSourceId = ids[0];
  same_id[0].request = requests[0];
  CHECK(core0.process_cycle(same_id, 52, [](const MatrixStoreRequest &) {}) == MatrixStoreTrackerStatus::Ok);
  CHECK(core1.process_cycle(same_id, 52, [](const MatrixStoreRequest &) {}) == MatrixStoreTrackerStatus::Ok);
  CHECK(core0.size() == 1);
  CHECK(core1.size() == 1);
  return true;
}

bool run_same_cycle_reuse(size_t request_lane, size_t response_lane) {
  MatrixStoreTracker tracker;
  constexpr uint64_t id = 0x3803ULL;
  const MatrixStoreRequest old_request = make_request(20);
  const MatrixStoreRequest new_request = make_request(21);
  CHECK(tracker.insert(id, old_request) == MatrixStoreTrackerStatus::Ok);

  std::array<MatrixStoreObservation, 2> observations{};
  observations[request_lane].reqValid = true;
  observations[request_lane].reqEncodedSourceId = id;
  observations[request_lane].request = new_request;
  observations[response_lane].respValid = true;
  observations[response_lane].respEncodedSourceId = id;
  bool old_completed = false;
  CHECK(tracker.process_cycle(observations, 60, [&](const MatrixStoreRequest &completed) {
    old_completed = same_payload(completed, old_request);
  }) == MatrixStoreTrackerStatus::Ok);
  CHECK(old_completed);
  CHECK(tracker.size() == 1);

  observations = {};
  observations[0].respValid = true;
  observations[0].respEncodedSourceId = id;
  bool new_completed = false;
  CHECK(tracker.process_cycle(observations, 61, [&](const MatrixStoreRequest &completed) {
    new_completed = same_payload(completed, new_request);
  }) == MatrixStoreTrackerStatus::Ok);
  CHECK(new_completed);
  return true;
}

bool test_response_before_request_and_replay() {
  CHECK(run_same_cycle_reuse(0, 1));
  CHECK(run_same_cycle_reuse(1, 0));

  MatrixStoreTracker tracker;
  const MatrixStoreRequest snapshotted = make_request(30);
  const MatrixStoreRequest later = make_request(31);
  CHECK(tracker.insert(0x3803ULL, snapshotted) == MatrixStoreTrackerStatus::Ok);
  CHECK(tracker.snapshot() == MatrixStoreTrackerStatus::Ok);
  CHECK(tracker.insert(0x400bULL, later) == MatrixStoreTrackerStatus::Ok);
  CHECK(tracker.size() == 2);
  CHECK(tracker.restore() == MatrixStoreTrackerStatus::Ok);
  CHECK(tracker.size() == 1);
  CHECK(tracker.entries().count(0x3803ULL) == 1);
  CHECK(tracker.entries().count(0x400bULL) == 0);
  return true;
}

} // namespace

int main() {
  const std::array tests = {
    test_more_than_legacy_capacity_and_out_of_order_completion,
    test_completion_only_after_response_and_id_reuse,
    test_duplicate_request_preserves_original_payload,
    test_response_tag_and_allocation_failures,
    test_outstanding_context_and_clear,
    test_multilane_cross_lane_completion_and_core_isolation,
    test_response_before_request_and_replay,
  };

  for (const auto test: tests) {
    if (!test()) {
      return 1;
    }
  }
  std::printf("matrix-store tracker: %zu tests passed\n", tests.size());
  return 0;
}
