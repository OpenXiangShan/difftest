#ifndef DIFFTEST_ARCH_EVENT_H
#define DIFFTEST_ARCH_EVENT_H

template <typename Event>
constexpr auto arch_event_commit_position(const Event &event, int) -> decltype(event.isLatter, int{}) {
  return event.isLatter ? 1 : 0;
}

// Releases without isLatter report the event before all ordinary commits.
template <typename Event>
constexpr int arch_event_commit_position(const Event &, ...) {
  return 0;
}

#endif
