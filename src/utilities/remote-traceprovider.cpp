// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.

#include <VisorVR/tracing.hpp>

#include <cstdlib>

namespace VisorVR {

/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("VisorVR.RemoteControl")
 * 6dafad04-3f57-55d2-e92e-7e49710d7e46
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "VisorVR.RemoteControl",
  (0x6dafad04, 0x3f57, 0x55d2, 0xe9, 0x2e, 0x7e, 0x49, 0x71, 0x0d, 0x7e, 0x46));

}// namespace VisorVR

namespace {

class TraceLoggingRegistration {
 public:
  TraceLoggingRegistration() {
    TraceLoggingRegister(VisorVR::gTraceProvider);
    TraceLoggingWrite(
      VisorVR::gTraceProvider,
      "Invocation/Start",
      TraceLoggingThisExecutable(),
      TraceLoggingValue(GetCommandLineW(), "Command Line"));
  }
  ~TraceLoggingRegistration() {
    TraceLoggingWrite(VisorVR::gTraceProvider, "Invocation/Stop");
    TraceLoggingUnregister(VisorVR::gTraceProvider);
  }

  TraceLoggingRegistration(const TraceLoggingRegistration&) = delete;
  TraceLoggingRegistration(TraceLoggingRegistration&&) = delete;
  TraceLoggingRegistration& operator=(const TraceLoggingRegistration&) = delete;
  TraceLoggingRegistration& operator=(TraceLoggingRegistration&&) = delete;
};
}// namespace

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
static TraceLoggingRegistration gTraceLoggingRegistration;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
