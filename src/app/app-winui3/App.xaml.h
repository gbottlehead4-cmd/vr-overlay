// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include "App.xaml.g.h"

#include <VisorVR/task.hpp>

namespace winrt::VisorVRApp::implementation {
struct App : AppT<App> {
  App();

  VisorVR::fire_and_forget OnLaunched(
    Microsoft::UI::Xaml::LaunchActivatedEventArgs) noexcept;

  ::VisorVR::task<void> CleanupAndExitAsync();

 private:
  winrt::Microsoft::UI::Xaml::Window mWindow {nullptr};
};
}// namespace winrt::VisorVRApp::implementation
