// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/ConsoleLoopCondition.hpp>
#include <VisorVR/WindowCaptureControl.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/scope_exit.hpp>

#include <Windows.h>
#include <shellapi.h>

#include <cstdlib>

using namespace VisorVR;

namespace VisorVR {
/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("VisorVR.WindowCaptureHook.Helper")
 * e8aa8bc1-a583-5d15-5ef0-8592c150f2be
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "VisorVR.WindowCaptureHook.Helper",
  (0xe8aa8bc1, 0xa583, 0x5d15, 0x5e, 0xf0, 0x85, 0x92, 0xc1, 0x50, 0xf2, 0xbe));
}// namespace VisorVR

// We only need a standard `main()` function, but using wWinMain prevents
// a window/task bar entry from temporarily appearing
int WINAPI wWinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  PWSTR pCmdLine,
  int nCmdShow) {
  TraceLoggingRegister(gTraceProvider);
  const scope_exit unregisterTraceProvider(
    []() { TraceLoggingUnregister(gTraceProvider); });
  DPrintSettings::Set({
    .prefix = std::format("WindowCaptureHook-helper-{}", sizeof(void*) * 8),
    .consoleOutput = DPrintSettings::ConsoleOutputMode::ALWAYS,
  });
  int argc = 0;
  auto argv = CommandLineToArgvW(pCmdLine, &argc);

  if (argc != 2) {
    dprint("Usage: HWND PARENTPROCESS");
    return 1;
  }

  const auto hwnd = reinterpret_cast<HWND>(_wcstoui64(argv[0], nullptr, 10));
  if (!hwnd) {
    dprint("Unable to parse an hwnd");
    return 1;
  }
  const auto parentID = static_cast<DWORD>(_wcstoui64(argv[1], nullptr, 10));
  if (!parentID) {
    dprint("Unable to parse parent ID");
    return 1;
  }

  dprint(
    "Attaching to HWND {:016x} from parent {}",
    reinterpret_cast<uint64_t>(hwnd),
    parentID);

  winrt::handle parent {
    OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, parentID)};

  auto handles = WindowCaptureControl::InstallHooks(hwnd);
  if (!handles.IsValid()) {
    dprint("Failed to attach to hwnd");
    return 1;
  }

  ConsoleLoopCondition loop;
  while (loop.Sleep(std::chrono::seconds(1))) {
    DWORD exitCode {};
    GetExitCodeProcess(parent.get(), &exitCode);
    if (exitCode != STILL_ACTIVE) {
      dprint("Parent quit, closing");
      return 0;
    }
    // do nothing
  }
  return 0;
}
