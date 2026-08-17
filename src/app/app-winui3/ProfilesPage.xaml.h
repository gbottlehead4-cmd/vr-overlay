// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

// clang-format off
#include "pch.h"
#include "ProfileUIData.g.h"
#include "ProfilesPage.g.h"
// clang-format on

#include <VisorVR/Events.hpp>

namespace VisorVR {
class KneeboardState;
}

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::VisorVRApp::implementation {
struct ProfilesPage : ProfilesPageT<ProfilesPage>,
                      private VisorVR::EventReceiver {
  ProfilesPage();

  static void final_release(std::unique_ptr<ProfilesPage>);

  VisorVR::fire_and_forget CreateProfile(
    Windows::Foundation::IInspectable,
    RoutedEventArgs) noexcept;
  VisorVR::fire_and_forget RemoveProfile(
    Windows::Foundation::IInspectable,
    RoutedEventArgs);

  VisorVR::fire_and_forget OnList_SelectionChanged(
    Windows::Foundation::IInspectable,
    SelectionChangedEventArgs);

 private:
  void UpdateList();
  Windows::Foundation::Collections::IObservableVector<IInspectable>
    mUIProfiles {single_threaded_observable_vector<IInspectable>()};
  VisorVR::audited_ptr<VisorVR::KneeboardState> mKneeboard;
};

struct ProfileUIData : ProfileUIDataT<ProfileUIData> {
  ProfileUIData() = default;

  winrt::guid ID();
  void ID(winrt::guid);

  hstring Name();
  void Name(hstring);

  bool CanDelete();
  void CanDelete(bool);

 private:
  winrt::guid mGuid;
  hstring mName;
  bool mCanDelete {true};
};

}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct ProfilesPage
  : ProfilesPageT<ProfilesPage, implementation::ProfilesPage> {};

struct ProfileUIData
  : ProfileUIDataT<ProfileUIData, implementation::ProfileUIData> {};
}// namespace winrt::VisorVRApp::factory_implementation
