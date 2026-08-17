// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

// clang-format off
#include "pch.h"
// clang-format on

#include <VisorVR/audited_ptr.hpp>
#include <VisorVR/task.hpp>

#include <shims/winrt/base.h>

#include <winrt/microsoft.ui.xaml.h>

#include <future>
#include <memory>

namespace VisorVR {

class KneeboardState;

task<void> InstallPlugin(
  audited_weak_ptr<KneeboardState>,
  winrt::Microsoft::UI::Xaml::XamlRoot,
  const wchar_t* const commandLine);

}// namespace VisorVR
