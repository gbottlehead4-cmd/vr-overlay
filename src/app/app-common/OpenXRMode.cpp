// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/Filesystem.hpp>
#include <VisorVR/OpenXRMode.hpp>
#include <VisorVR/RunSubprocessAsync.hpp>
#include <VisorVR/RuntimeFiles.hpp>

#include <VisorVR/dprint.hpp>

#include <format>

namespace VisorVR {

static auto LaunchAndWaitForOpenXRHelperSubprocess(
  RunAs runas,
  std::wstring_view command) {
  const auto layerPath = RuntimeFiles::GetInstallationDirectory().wstring();
  const auto exePath = (Filesystem::GetRuntimeDirectory()
                        / RuntimeFiles::OPENXR_REGISTER_LAYER_HELPER)
                         .wstring();

  const auto commandLine = std::format(L"{} \"{}\"", command, layerPath);
  return RunSubprocessAsync(exePath, commandLine, runas);
}

task<void> SetOpenXR64ModeWithHelperProcess(OpenXRMode mode) {
  switch (mode) {
    case OpenXRMode::Disabled:
      co_await LaunchAndWaitForOpenXRHelperSubprocess(
        RunAs::Administrator, L"disable-HKLM-64");
      break;
    case OpenXRMode::AllUsers:
      co_await LaunchAndWaitForOpenXRHelperSubprocess(
        RunAs::Administrator, L"enable-HKLM-64");
      break;
  }
}

task<void> SetOpenXR32ModeWithHelperProcess(OpenXRMode mode) {
  switch (mode) {
    case OpenXRMode::Disabled:
      co_await LaunchAndWaitForOpenXRHelperSubprocess(
        RunAs::Administrator, L"disable-HKLM-32");
      break;
    case OpenXRMode::AllUsers:
      co_await LaunchAndWaitForOpenXRHelperSubprocess(
        RunAs::Administrator, L"enable-HKLM-32");
      break;
  }
}

}// namespace VisorVR
