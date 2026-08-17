// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Pixels.hpp>

#include <VisorVR/config.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace VisorVR {

/** Largest pixel density we can usefully rasterize `size` at.
 *
 * Anything sharper than `MaxViewRenderSize` is scaled back down before it
 * reaches the headset, so rendering it would just cost GPU time for nothing.
 */
constexpr float MaxUsefulRenderScale(const PixelSize& size) noexcept {
  if (size.mWidth == 0 || size.mHeight == 0) {
    return 1.0f;
  }
  return std::min(
    Config::MaxViewRenderSize.Width<float>() / size.Width<float>(),
    Config::MaxViewRenderSize.Height<float>() / size.Height<float>());
}

struct WebPageSourceSettings {
  PixelSize mInitialSize {Config::DefaultWebPagePixelSize};
  /** Pixel density multiplier, like a HiDPI display.
   *
   * The page still lays out at `mInitialSize` CSS pixels, but is rasterized at
   * `mInitialSize * mRenderScale` real pixels, so it stays sharp when the panel
   * is pulled closer in VR. Physical size in meters adds no detail; only this
   * does.
   *
   * Costs GPU time and VRAM quadratically, and is pointless above
   * `MaxUsefulRenderScale(mInitialSize)`.
   */
  float mRenderScale {1.0f};
  bool mExposeOpenKneeboardAPIs {true};
  bool mIntegrateWithSimHub {true};
  std::string mURI;
  bool mTransparentBackground {true};

  ///// NOT SAVED - JUST FOR INTERNAL USE (e.g. PluginTab) /////
  std::unordered_map<std::string, std::filesystem::path> mVirtualHosts;

  /// `mRenderScale`, clamped to something this panel can actually display.
  constexpr float GetUsableRenderScale() const noexcept {
    return std::clamp(mRenderScale, 1.0f, MaxUsefulRenderScale(mInitialSize));
  }

  constexpr bool operator==(const WebPageSourceSettings&) const noexcept =
    default;
};
}// namespace VisorVR
