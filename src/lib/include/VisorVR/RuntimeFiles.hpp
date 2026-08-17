// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <filesystem>
#include <string_view>

namespace VisorVR::RuntimeFiles {

#ifdef _WIN64
#define VISORVR_PRIVATE_ARCH_RUNTIME_FILES \
  IT(WINDOW_CAPTURE_HOOK_32BIT_HELPER)
#else
#define VISORVR_PRIVATE_ARCH_RUNTIME_FILES
#endif

#define VISORVR_PRIVATE_RUNTIME_FILES \
  IT(OPENXR_REGISTER_LAYER_HELPER) \
  VISORVR_PRIVATE_ARCH_RUNTIME_FILES

#define VISORVR_PUBLIC_RUNTIME_FILES \
  IT(CHROMIUM) \
  IT(DCSWORLD_HOOK_DLL) \
  IT(DCSWORLD_HOOK_LUA) \
  IT(WINDOW_CAPTURE_HOOK_DLL) \
  IT(OPENXR_64BIT_DLL) \
  IT(OPENXR_32BIT_DLL) \
  IT(OPENXR_64BIT_JSON) \
  IT(OPENXR_32BIT_JSON) \
  IT(WELCOME_HTML)

#define VISORVR_RUNTIME_FILES \
  VISORVR_PUBLIC_RUNTIME_FILES \
  VISORVR_PRIVATE_RUNTIME_FILES

#define IT(x) extern const std::string_view x;
VISORVR_RUNTIME_FILES
#undef IT

std::filesystem::path GetInstallationDirectory();

}// namespace VisorVR::RuntimeFiles
