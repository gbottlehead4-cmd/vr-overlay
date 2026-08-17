// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
// clang-format off
#include "pch.h"
#include "IndependentVRViewUIKind.h"
#include "IndependentVRViewUIKind.g.cpp"
// clang-format on

#include <VisorVR/utf8.hpp>

using namespace VisorVR;

namespace winrt::VisorVRApp::implementation {
IndependentVRViewUIKind::IndependentVRViewUIKind() { Label(_(L"Independent")); }
}// namespace winrt::VisorVRApp::implementation
