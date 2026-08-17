// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
// clang-format off
#include "pch.h"
#include "SettingsPage.xaml.h"
#include "SettingsPage.g.cpp"
#include "SettingsSubpageData.g.cpp"
// clang-format on

#include "Globals.h"

#include <VisorVR/KneeboardState.hpp>

#include <VisorVR/config.hpp>
#include <VisorVR/version.hpp>

namespace winrt::VisorVRApp::implementation {

using namespace VisorVR;

SettingsPage::SettingsPage() { InitializeComponent(); }

void SettingsPage::OnItemClick(
  const IInspectable&,
  const ItemClickEventArgs& args) noexcept {
  auto item = args.ClickedItem();
  if (!item) {
    return;
  }

#define IT(X) \
  if (item == X##Item()) { \
    Frame().Navigate(xaml_typename<X##SettingsPage>()); \
    return; \
  }
  IT(VR)
  IT(Tabs)
  IT(Input)
  IT(Advanced)
#undef IT

  VISORVR_BREAK;
}// namespace winrt::VisorVRApp::implementation

hstring SettingsSubpageData::Title() { return mTitle; }

void SettingsSubpageData::Title(hstring const& value) { mTitle = value; }

hstring SettingsSubpageData::Glyph() { return mGlyph; }

void SettingsSubpageData::Glyph(hstring const& value) { mGlyph = value; }

hstring SettingsSubpageData::Description() { return mDescription; }

void SettingsSubpageData::Description(hstring const& value) {
  mDescription = value;
}

}// namespace winrt::VisorVRApp::implementation
