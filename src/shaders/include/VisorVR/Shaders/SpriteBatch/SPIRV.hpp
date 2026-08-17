// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <string>

namespace VisorVR::Shaders::SpriteBatch::SPIRV::Detail {

#include <VisorVR/Shaders/gen/VisorVR-SpriteBatch-SPIRV-PS.hpp>
#include <VisorVR/Shaders/gen/VisorVR-SpriteBatch-SPIRV-VS.hpp>

}// namespace VisorVR::Shaders::SpriteBatch::SPIRV::Detail

namespace VisorVR::Shaders::SpriteBatch::SPIRV {

constexpr std::basic_string_view<unsigned char> PS {
  Detail::g_SpritePixelShader,
  std::size(Detail::g_SpritePixelShader)};

constexpr std::basic_string_view<unsigned char> VS {
  Detail::g_SpriteVertexShader,
  std::size(Detail::g_SpriteVertexShader)};

}// namespace VisorVR::Shaders::SpriteBatch::SPIRV
