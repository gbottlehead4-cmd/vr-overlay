// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once
// clang-format off
#include "pch.h"
#include "VRSettingsPage.g.h"
// clang-format on

#include "WithPropertyChangedEvent.h"

#include <VisorVR/Events.hpp>
#include <VisorVR/ViewsSettings.hpp>

#include <winrt/Microsoft.UI.Xaml.Controls.h>

using namespace winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

namespace VisorVR {
class KneeboardState;
}

using namespace VisorVR;

namespace winrt::VisorVRApp::implementation {
struct VRSettingsPage
  : VRSettingsPageT<VRSettingsPage>,
    WithPropertyChangedEventOnProfileChange<VRSettingsPage> {
  VRSettingsPage();
  ~VRSettingsPage();

  bool SteamVREnabled();
  VisorVR::fire_and_forget SteamVREnabled(bool);

  bool OpenXR64Enabled() noexcept;
  VisorVR::fire_and_forget OpenXR64Enabled(bool) noexcept;

  bool OpenXR32Enabled() noexcept;
  VisorVR::fire_and_forget OpenXR32Enabled(bool) noexcept;

  VisorVR::fire_and_forget RestoreDefaults(
    IInspectable,
    RoutedEventArgs) noexcept;

  VisorVR::fire_and_forget AddView(muxc::TabView, IInspectable) noexcept;

  VisorVR::fire_and_forget RemoveView(
    muxc::TabView,
    muxc::TabViewTabCloseRequestedEventArgs) noexcept;

 private:
  VisorVR::audited_ptr<KneeboardState> mKneeboard;
  void PopulateViews() noexcept;

  void AppendViewTab(const ViewSettings& view) noexcept;
};
}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct VRSettingsPage
  : VRSettingsPageT<VRSettingsPage, implementation::VRSettingsPage> {};
}// namespace winrt::VisorVRApp::factory_implementation
