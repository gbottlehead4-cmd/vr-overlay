// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/AppSettings.hpp>
#include <VisorVR/DirectInputSettings.hpp>
#include <VisorVR/DoodleSettings.hpp>
#include <VisorVR/TabletSettings.hpp>
#include <VisorVR/TextSettings.hpp>
#include <VisorVR/UISettings.hpp>
#include <VisorVR/VRSettings.hpp>
#include <VisorVR/ViewsSettings.hpp>

#include <filesystem>

namespace VisorVR {

#define VISORVR_GLOBAL_SETTINGS_SECTIONS IT(AppSettings, App)

#define VISORVR_PER_PROFILE_SETTINGS_SECTIONS \
  IT(DirectInputSettings, DirectInput) \
  IT(DoodleSettings, Doodles) \
  IT(TextSettings, Text) \
  IT(TabletSettings, TabletInput) \
  IT(nlohmann::json, Tabs) \
  IT(UISettings, UI) \
  IT(ViewsSettings, Views) \
  IT(VRSettings, VR)

#define VISORVR_SETTINGS_SECTIONS \
  VISORVR_GLOBAL_SETTINGS_SECTIONS \
  VISORVR_PER_PROFILE_SETTINGS_SECTIONS

struct Settings final {
#define IT(cpptype, name) cpptype m##name {};
  VISORVR_SETTINGS_SECTIONS
#undef IT

  static Settings Load(winrt::guid defaultProfile, winrt::guid activeProfile);
  void Save(winrt::guid defaultProfile, winrt::guid activeProfile) const;
#define IT(cpptype, name) \
  void Reset##name##Section( \
    winrt::guid defaultProfile, winrt::guid activeProfile);
  VISORVR_SETTINGS_SECTIONS
#undef IT

  bool operator==(const Settings&) const noexcept = default;
};

VISORVR_DECLARE_SPARSE_JSON(Settings);

}// namespace VisorVR
