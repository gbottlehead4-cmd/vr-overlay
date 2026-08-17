// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <string>

namespace VisorVR::Shaders::Viewer::DXBC::Detail {

#include <VisorVR/Shaders/gen/VisorVR-Viewer-DXBC-PS.hpp>
#include <VisorVR/Shaders/gen/VisorVR-Viewer-DXBC-VS.hpp>

}// namespace VisorVR::Shaders::Viewer::DXBC::Detail

namespace VisorVR::Shaders::Viewer::DXBC {

constexpr std::basic_string_view<unsigned char> PS {
  Detail::g_ViewerPixelShader,
  std::size(Detail::g_ViewerPixelShader)};

constexpr std::basic_string_view<unsigned char> VS {
  Detail::g_ViewerVertexShader,
  std::size(Detail::g_ViewerVertexShader)};

}// namespace VisorVR::Shaders::Viewer::DXBC
