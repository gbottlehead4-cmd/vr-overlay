// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
// clang-format off
#include "pch.h"
#include "MetersValueConverter.h"
#include "MetersValueConverter.g.cpp"
// clang-format on

#include <cmath>
#include <format>

namespace winrt::OpenKneeboardApp::implementation {
winrt::Windows::Foundation::IInspectable MetersValueConverter::Convert(
  winrt::Windows::Foundation::IInspectable const& value,
  winrt::Windows::UI::Xaml::Interop::TypeName const& /*targetType*/,
  winrt::Windows::Foundation::IInspectable const& /*parameter*/,
  hstring const& /*language*/) {
  const auto meters = unbox_value_or<double>(value, 0.0);
  return box_value(to_hstring(std::format("{:.2f} m", meters)));
}

winrt::Windows::Foundation::IInspectable MetersValueConverter::ConvertBack(
  winrt::Windows::Foundation::IInspectable const& /*value*/,
  winrt::Windows::UI::Xaml::Interop::TypeName const& /*targetType*/,
  winrt::Windows::Foundation::IInspectable const& /*parameter*/,
  hstring const& /*language*/) {
  throw hresult_not_implemented();
}
}// namespace winrt::OpenKneeboardApp::implementation
