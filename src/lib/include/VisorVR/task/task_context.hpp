// Copyright 2026 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <VisorVR/dprint.hpp>

#include <combaseapi.h>
#include <ctxtcall.h>

#include <cinttypes>
#include <thread>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
#include <wil/com.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace VisorVR::detail {
struct TaskContext;
struct TaskContextAwaiter;
}// namespace VisorVR::detail

namespace VisorVR::inline task_ns {

struct task_context;
namespace this_thread {
task_context get_task_context();
}

struct task_context {
  friend struct detail::TaskContextAwaiter;

  friend task_context this_thread::get_task_context();

  [[nodiscard]]
  bool is_same_com_context() const noexcept {
    ULONG_PTR current {};
    [[maybe_unused]] const auto hr = CoGetContextToken(&current);
    VISORVR_ASSERT(SUCCEEDED(hr));
    return current == mCOMContextToken;
  }

 protected:
  ULONG_PTR mCOMContextToken {};
  wil::com_ptr<IContextCallback> mCOMCallback;

  task_context() {
    [[maybe_unused]] const auto hr =
      CoGetObjectContext(IID_PPV_ARGS(mCOMCallback.put()));
    VISORVR_ASSERT(
      SUCCEEDED(hr),
      "Attempted to create a task_context<> without a COM context: {:#010x}",
      static_cast<uint32_t>(hr));
    [[maybe_unused]] const auto tokenHr = CoGetContextToken(&mCOMContextToken);
    VISORVR_ASSERT(
      SUCCEEDED(tokenHr),
      "Failed to capture COM context token: {:#010x}",
      static_cast<uint32_t>(tokenHr));
  }
};

namespace this_thread {
inline task_context get_task_context() { return task_context {}; }
}// namespace this_thread

}// namespace VisorVR::inline task_ns
