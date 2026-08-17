// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/task.hpp>

#include <shims/winrt/base.h>

#include <winrt/Windows.Foundation.h>

#include <functional>
#include <string>

namespace VisorVR {

namespace SpecialURIs {
constexpr std::string_view Scheme {"visorvr"};

namespace Paths {
constexpr std::string_view SettingsInput {"Settings/Input"};
constexpr std::string_view SettingsTabs {"Settings/Tabs"};
}// namespace Paths

std::string_view SettingsInput();
/// The panel list: adding, deleting and reordering panels.
std::string_view SettingsTabs();

};// namespace SpecialURIs

void RegisterURIHandler(
  std::string_view schemeName,
  std::function<void(std::string_view)> handler);

task<void> LaunchURI(std::string_view uri);

}// namespace VisorVR
