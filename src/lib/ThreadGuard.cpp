// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.

#include <VisorVR/ThreadGuard.hpp>

#include <VisorVR/dprint.hpp>

namespace VisorVR {

ThreadGuard::ThreadGuard(const std::source_location& loc) : mLocation(loc) {
#ifdef DEBUG
  mThreadID = GetCurrentThreadId();
#endif
}

void ThreadGuard::CheckThread(const std::source_location& loc) const {
  if constexpr (Config::IsDebugBuild) {
    const auto thisThread = GetCurrentThreadId();
    if (thisThread == mThreadID) {
      return;
    }
    dprint(
      "ThreadGuard mismatch: was {} ({:#x}), now {} ({:#x})",
      mThreadID,
      mThreadID,
      thisThread,
      thisThread);
    dprint("Created at {}", mLocation);
    dprint("Checking at {}", loc);
    VISORVR_BREAK;
  }
}

ThreadGuard::~ThreadGuard() { this->CheckThread(); }

}// namespace VisorVR
