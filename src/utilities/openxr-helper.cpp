// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.

/** A separate process to register the OpenXR layer, outside of the
 * MSIX sandbox.
 *
 * If done from the main process, the registry write will be app-specific.
 */

#include <VisorVR/OpenXRLayerRegistry.hpp>
#include <VisorVR/RuntimeFiles.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/scope_exit.hpp>

#include <Windows.h>
#include <shellapi.h>

#include <filesystem>
#include <string>

using namespace VisorVR;
using namespace VisorVR::OpenXRLayers;

namespace VisorVR {

/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("VisorVR.OpenXR.Helper")
 * 2489967e-a7f2-5db8-ba74-27c35b944d56
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "VisorVR.OpenXR.Helper",
  (0x2489967e, 0xa7f2, 0x5db8, 0xba, 0x74, 0x27, 0xc3, 0x5b, 0x94, 0x4d, 0x56));
}// namespace VisorVR

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int) {
  TraceLoggingRegister(gTraceProvider);
  const scope_exit unregisterTraceProvider(
    []() { TraceLoggingUnregister(gTraceProvider); });
  DPrintSettings::Set({
    .prefix = "OpenXR-Helper",
    .consoleOutput = DPrintSettings::ConsoleOutputMode::ALWAYS,
  });
  int argc = 0;
  auto argv = CommandLineToArgvW(commandLine, &argc);
  if (argc != 2) {
    dprint("Invalid arguments ({}):", argc);
    for (int i = 0; i < argc; ++i) {
      dprint(L"argv[{}]: {}", i, argv[i]);
    }
    return 1;
  }
  dprint(L"OpenXR: {} -> {}", argv[0], argv[1]);
  const auto command = std::wstring_view(argv[0]);
  const std::filesystem::path directory {std::wstring_view(argv[1])};

  const auto layer64 = directory / RuntimeFiles::OPENXR_64BIT_JSON;
  if (command == L"disable-HKLM-64") {
    return Disable(Scope::AllUsers, Bitness::x64, layer64) ? 0 : 1;
  }
  if (command == L"enable-HKLM-64") {
    return Enable(Scope::AllUsers, Bitness::x64, layer64) ? 0 : 1;
  }
  // Per-user equivalents; these need no elevation, and are what a portable
  // copy registers itself with. Exposed here too so the same commands work
  // for scripting and troubleshooting.
  if (command == L"disable-HKCU-64") {
    return Disable(Scope::CurrentUser, Bitness::x64, layer64) ? 0 : 1;
  }
  if (command == L"enable-HKCU-64") {
    return Enable(Scope::CurrentUser, Bitness::x64, layer64) ? 0 : 1;
  }

  const auto layer32 = directory / RuntimeFiles::OPENXR_32BIT_JSON;
  if (command == L"disable-HKLM-32") {
    return Disable(Scope::AllUsers, Bitness::x86, layer32) ? 0 : 1;
  }
  if (command == L"enable-HKLM-32") {
    return Enable(Scope::AllUsers, Bitness::x86, layer32) ? 0 : 1;
  }

  dprint(L"Invalid command: {}", command);
  return 1;
}
