// A checkpoint writer serializes REF restore state in a process that exits
// afterwards, so serialization never mutates a REF that continues running.
// A single-threaded owner registers each writer; the writer manager reaps it,
// publishes the completed snapshot and retains the newest endpoints.
#ifndef CHECKPOINT_WRITER_H
#define CHECKPOINT_WRITER_H

#include <cstdint>
#include <functional>
#include <string>
#include <sys/types.h>

// Serialize in the calling process and exit; this REF must never resume execution
// Its parent registers the PID with adopt; a reused checker is registered before SAVE
[[noreturn]] void checkpoint_writer_run(const std::string &base, const std::function<void(const char *)> &serialize);
// Run the serializer in a COW process so its PMEM writes do not change the live REF
// sibling=true makes the calling checker's supervisor the writer's parent
pid_t checkpoint_writer_fork(const std::string &base, const std::function<void(const char *)> &serialize,
                             bool sibling = false);
// Transfer exclusive waitpid/publication ownership to the writer's parent
// The parent checks capacity first; this call does not create or wait for a process
void checkpoint_writer_adopt(pid_t pid, const std::string &base, uint64_t instr);
// Fork and register a writer from the single-threaded SLOW/FAST parser
bool checkpoint_writer_submit(const std::string &base, uint64_t instr,
                              const std::function<void(const char *)> &serialize);
// Reap exited writers without waiting and return whether all have succeeded so far
bool checkpoint_writer_poll();
// Wait for all registered writers and return their combined success status
bool checkpoint_writer_drain();

#endif // CHECKPOINT_WRITER_H
