// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once
// clang-format off
#include "pch.h"
#include "VRViewSettingsControl.g.h"
// clang-format on

#include "WithPropertyChangedEvent.h"

#include <VisorVR/ViewsSettings.hpp>

namespace VisorVR {
class KneeboardState;
}

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace VisorVR;

namespace winrt::VisorVRApp::implementation {
struct VRViewSettingsControl : VRViewSettingsControlT<VRViewSettingsControl>,
                               WithPropertyChangedEvent,
                               EventReceiver {
  VRViewSettingsControl();
  ~VRViewSettingsControl();

  winrt::guid ViewID();
  void ViewID(const winrt::guid&);

  bool IsEnabledInVR();
  VisorVR::fire_and_forget IsEnabledInVR(bool);

  IInspectable SelectedKind();
  VisorVR::fire_and_forget SelectedKind(
    Windows::Foundation::IInspectable);

  IInspectable SelectedDefaultTab();
  VisorVR::fire_and_forget SelectedDefaultTab(
    Windows::Foundation::IInspectable);

  winrt::Microsoft::UI::Xaml::Visibility TooManyViewsVisibility();

 private:
  audited_ptr<VisorVR::KneeboardState> mKneeboard;

  winrt::guid mViewID;

  void PopulateKind(const ViewVRSettings&);
  void PopulateSubcontrol(const ViewVRSettings&);
  void PopulateDefaultTab();

  Control mSubControl {nullptr};
};
}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct VRViewSettingsControl : VRViewSettingsControlT<
                                 VRViewSettingsControl,
                                 implementation::VRViewSettingsControl> {};
}// namespace winrt::VisorVRApp::factory_implementation
