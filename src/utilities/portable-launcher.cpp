// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.

/** Start button for a portable copy.
 *
 * The app itself has to live in `bin`, because it finds Chromium and its data
 * files by looking beside its own folder. That leaves a new user unzipping a
 * folder of subdirectories with nothing obvious to double-click, so this sits
 * at the top level and starts the real thing.
 *
 * Deliberately tiny and dependency-free: it is the first thing a new user
 * runs, so it should not be able to fail in interesting ways.
 */

#include <Windows.h>
#include <shellapi.h>

#include <filesystem>

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int showCommand) {
  wchar_t rawPath[MAX_PATH] {};
  const auto length = GetModuleFileNameW(nullptr, rawPath, MAX_PATH);
  if (length == 0 || length == MAX_PATH) {
    MessageBoxW(
      nullptr,
      L"Couldn't work out where VisorVR is installed.",
      L"VisorVR",
      MB_OK | MB_ICONERROR);
    return 1;
  }

  const std::filesystem::path here {rawPath};
  const auto app = here.parent_path() / L"bin" / L"VisorVRApp.exe";

  std::error_code ec;
  if (!std::filesystem::exists(app, ec)) {
    MessageBoxW(
      nullptr,
      L"VisorVR is incomplete: bin\\VisorVRApp.exe is missing.\n\n"
      L"If you copied VisorVR here, copy the whole folder - the bin, libexec "
      L"and share folders all need to stay together.",
      L"VisorVR",
      MB_OK | MB_ICONERROR);
    return 1;
  }

  const auto workingDirectory = app.parent_path().wstring();
  const auto appPath = app.wstring();

  SHELLEXECUTEINFOW info {
    .cbSize = sizeof(SHELLEXECUTEINFOW),
    .fMask = SEE_MASK_NOASYNC,
    .lpVerb = L"open",
    .lpFile = appPath.c_str(),
    .lpDirectory = workingDirectory.c_str(),
    .nShow = showCommand,
  };
  if (!ShellExecuteExW(&info)) {
    MessageBoxW(
      nullptr, L"Couldn't start VisorVR.", L"VisorVR", MB_OK | MB_ICONERROR);
    return 1;
  }
  return 0;
}
