// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/Filesystem.hpp>
#include <VisorVR/OpenXRLayerRegistry.hpp>
#include <VisorVR/RuntimeFiles.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/scope_exit.hpp>

#include <Windows.h>

#include <functional>
#include <string>
#include <vector>

namespace VisorVR::OpenXRLayers {

namespace {

constexpr auto SubKey = L"SOFTWARE\\Khronos\\OpenXR\\1\\ApiLayers\\Implicit";

HKEY RootKey(Scope scope) {
  return scope == Scope::AllUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
}

REGSAM ViewFlag(Bitness bitness) {
  return bitness == Bitness::x64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
}

HKEY OpenOrCreate(Scope scope, Bitness bitness, REGSAM access) {
  HKEY key {};
  const auto result = RegCreateKeyExW(
    RootKey(scope),
    SubKey,
    0,
    nullptr,
    0,
    access | ViewFlag(bitness),
    nullptr,
    &key,
    nullptr);
  if (result != ERROR_SUCCESS) {
    dprint("Failed to open OpenXR implicit layer key: {}", result);
    return {};
  }
  return key;
}

/// Every value name under the implicit-layer key; these are layer JSON paths.
std::vector<std::wstring> EnumerateLayers(HKEY key) {
  std::vector<std::wstring> layers;
  // https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-element-size-limits
  std::wstring buffer(16383, L'\0');
  DWORD index {0};
  while (true) {
    DWORD size = static_cast<DWORD>(buffer.size());
    if (
      RegEnumValueW(
        key, index++, buffer.data(), &size, nullptr, nullptr, nullptr, nullptr)
      != ERROR_SUCCESS) {
      break;
    }
    layers.emplace_back(buffer.data(), size);
  }
  return layers;
}

bool SetDisabledFlag(HKEY key, const std::wstring& path, DWORD disabled) {
  const auto result = RegSetValueExW(
    key,
    path.c_str(),
    0,
    REG_DWORD,
    reinterpret_cast<const BYTE*>(&disabled),
    sizeof(disabled));
  if (result != ERROR_SUCCESS) {
    dprint("Failed to set OpenXR layer value: {}", result);
    return false;
  }
  return true;
}

/// Canonical path, or the input unchanged if it does not exist yet.
std::wstring CanonicalOrRaw(const std::filesystem::path& path) {
  std::error_code ec;
  const auto canonical = std::filesystem::canonical(path, ec);
  return ec ? path.wstring() : canonical.wstring();
}

}// namespace

bool IsEnabled(
  const Scope scope,
  const Bitness bitness,
  const std::filesystem::path& jsonPath) {
  DWORD data {};
  DWORD dataSize = sizeof(data);
  const auto result = RegGetValueW(
    RootKey(scope),
    SubKey,
    CanonicalOrRaw(jsonPath).c_str(),
    RRF_RT_DWORD
      | (bitness == Bitness::x64 ? RRF_SUBKEY_WOW6464KEY
                                 : RRF_SUBKEY_WOW6432KEY),
    nullptr,
    &data,
    &dataSize);
  if (result != ERROR_SUCCESS) {
    return false;
  }
  // For implicit layers the value is a 'disabled' flag: 0 means active.
  return data == 0;
}

bool Enable(
  const Scope scope,
  const Bitness bitness,
  const std::filesystem::path& rawJsonPath) {
  auto key = OpenOrCreate(scope, bitness, KEY_ALL_ACCESS);
  if (!key) {
    return false;
  }
  const scope_exit closeKey([key]() { RegCloseKey(key); });

  const auto jsonPath = CanonicalOrRaw(rawJsonPath);
  const auto jsonFile = rawJsonPath.filename().wstring();

  // A second copy of VisorVR registered from a different folder would put two
  // identical overlays in the headset, so stand the others down first.
  for (const auto& layer: EnumerateLayers(key)) {
    if (layer != jsonPath && layer.ends_with(jsonFile)) {
      dprint(L"Disabling stale VisorVR OpenXR layer: {}", layer);
      SetDisabledFlag(key, layer, 1);
    }
  }

  return SetDisabledFlag(key, jsonPath, 0);
}

bool Disable(
  const Scope scope,
  const Bitness bitness,
  const std::filesystem::path& rawJsonPath) {
  auto key = OpenOrCreate(scope, bitness, KEY_ALL_ACCESS);
  if (!key) {
    return false;
  }
  const scope_exit closeKey([key]() { RegCloseKey(key); });
  return SetDisabledFlag(key, CanonicalOrRaw(rawJsonPath), 1);
}

void RemoveStaleEntries(
  const Scope scope,
  const Bitness bitness,
  const std::wstring_view jsonFileName) {
  auto key = OpenOrCreate(scope, bitness, KEY_ALL_ACCESS);
  if (!key) {
    return;
  }
  const scope_exit closeKey([key]() { RegCloseKey(key); });

  for (const auto& layer: EnumerateLayers(key)) {
    if (!layer.ends_with(jsonFileName)) {
      continue;
    }
    if (std::filesystem::exists(std::filesystem::path {layer})) {
      continue;
    }
    dprint(L"Removing OpenXR layer registration for missing file: {}", layer);
    RegDeleteValueW(key, layer.c_str());
  }
}

bool RegisterForCurrentUser() {
  std::error_code ec;
  const auto jsonPath = RuntimeFiles::GetInstallationDirectory()
    / RuntimeFiles::OPENXR_64BIT_JSON;
  if (!std::filesystem::exists(jsonPath, ec)) {
    dprint(L"No OpenXR layer JSON at {}; not self-registering", jsonPath.wstring());
    return false;
  }

  // Do this before anything else: folders this copy previously ran from are
  // gone either way, and leaving dead values behind would mean the OpenXR
  // loader complaining about missing layers on every launch.
  RemoveStaleEntries(
    Scope::CurrentUser,
    Bitness::x64,
    std::wstring {
      RuntimeFiles::OPENXR_64BIT_JSON.begin(),
      RuntimeFiles::OPENXR_64BIT_JSON.end()});

  // An installed copy already registered this same path for all users; leave
  // it be rather than shadowing it with a duplicate per-user entry.
  if (IsEnabled(Scope::AllUsers, Bitness::x64, jsonPath)) {
    dprint("OpenXR layer already registered for all users; leaving it alone");
    return true;
  }

  if (IsEnabled(Scope::CurrentUser, Bitness::x64, jsonPath)) {
    return true;
  }

  dprint(L"Registering OpenXR layer for this user: {}", jsonPath.wstring());
  return Enable(Scope::CurrentUser, Bitness::x64, jsonPath);
}

bool UnregisterForCurrentUser() {
  const auto jsonPath = RuntimeFiles::GetInstallationDirectory()
    / RuntimeFiles::OPENXR_64BIT_JSON;
  return Disable(Scope::CurrentUser, Bitness::x64, jsonPath);
}

}// namespace VisorVR::OpenXRLayers
