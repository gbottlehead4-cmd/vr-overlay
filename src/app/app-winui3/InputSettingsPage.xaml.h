// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

// clang-format off
#include "pch.h"
#include "InputSettingsPage.g.h"
// clang-format on

#include "WithPropertyChangedEvent.h"

#include <VisorVR/Events.hpp>

#include <string>

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Windows::Foundation::Collections;

namespace VisorVR {
class KneeboardState;
}

namespace winrt::VisorVRApp::implementation {
struct InputSettingsPage
  : InputSettingsPageT<InputSettingsPage>,
    VisorVR::WithPropertyChangedEventOnProfileChange<InputSettingsPage> {
  InputSettingsPage();
  ~InputSettingsPage();

  VisorVR::fire_and_forget RestoreDefaults(
    IInspectable,
    RoutedEventArgs) noexcept;

  IVector<IInspectable> Devices() noexcept;
  void OnOrientationChanged(
    const IInspectable&,
    const SelectionChangedEventArgs&);

 private:
  winrt::apartment_context mUIThread;
  VisorVR::audited_ptr<VisorVR::KneeboardState> mKneeboard;
};
}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct InputSettingsPage
  : InputSettingsPageT<InputSettingsPage, implementation::InputSettingsPage> {};
}// namespace winrt::VisorVRApp::factory_implementation
