// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once
// clang-format off
#include "pch.h"
#include "MetersValueConverter.g.h"
// clang-format on

namespace winrt::VisorVRApp::implementation {
struct MetersValueConverter : MetersValueConverterT<MetersValueConverter> {
  MetersValueConverter() = default;

  winrt::Windows::Foundation::IInspectable Convert(
    winrt::Windows::Foundation::IInspectable const& value,
    winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
    winrt::Windows::Foundation::IInspectable const& parameter,
    hstring const& language);
  winrt::Windows::Foundation::IInspectable ConvertBack(
    winrt::Windows::Foundation::IInspectable const& value,
    winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
    winrt::Windows::Foundation::IInspectable const& parameter,
    hstring const& language);
};
}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct MetersValueConverter : MetersValueConverterT<
                                MetersValueConverter,
                                implementation::MetersValueConverter> {};
}// namespace winrt::VisorVRApp::factory_implementation
