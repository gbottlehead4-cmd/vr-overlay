// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Pixels.hpp>
#include <VisorVR/PreferredSize.hpp>

#include <VisorVR/detail/config.hpp>

#include <string>

#include <intrin.h>

namespace VisorVR::inline Config {

constexpr auto BuildBitness = sizeof(void*) * 8;
constexpr bool Is32BitBuild = (BuildBitness == 32);
constexpr bool Is64BitBuild = (BuildBitness == 64);

// As we lock the entire SHM segment before touching the texture, buffering
// isn't needed; that said, keep a buffer anyway, as seeing frame counters
// go backwards is a very easy way to diagnose issues :)
constexpr unsigned int SHMSwapchainLength = 2;
constexpr PixelSize MaxViewRenderSize {2048, 2048};
constexpr unsigned char MaxViewCount = 16;
constexpr unsigned int FramesPerSecond = 90;

// 5:8, matching entry-level Wacom and Huion tablets
constexpr PixelSize DefaultPixelSize {700, 1120};
constexpr PixelSize ErrorPixelSize = DefaultPixelSize;
constexpr PreferredSize ErrorPreferredSize {
  ErrorPixelSize,
  ScalingKind::Vector,
};
// We don't use the DefaultPixelSize as:
// - `DefaultPixelSize` is primarily for actual kneeboards
// - 5:8 doesn't really work for other common use cases,
//   e.g. youtube, discord, overlays
// - web pages intended to be kneeboards are the most likely
//   to call a resize API
constexpr PixelSize DefaultWebPagePixelSize {1024, 768};

constexpr float CursorRadiusDivisor = 400.0f;
constexpr float CursorStrokeDivisor = CursorRadiusDivisor;

constexpr unsigned int HeaderPercent = 5;
constexpr unsigned int FooterPercent = HeaderPercent;
constexpr unsigned int BookmarksBarPercent = HeaderPercent;

constexpr const std::string_view BuildType {detail::Config::BuildType};
constexpr bool IsDebugBuild {BuildType == "Debug"};

constexpr const char ProjectReverseDomainA[] {"@PROJECT_REVERSE_DOMAIN@"};
constexpr const wchar_t ProjectReverseDomainW[] {L"@PROJECT_REVERSE_DOMAIN@"};
constexpr const std::string_view OpenXRApiLayerName {
  "@PROJECT_OPENXR_API_LAYER_NAME@"};
constexpr const std::string_view OpenXRApiLayerDescription {
  "@PROJECT_OPENXR_API_LAYER_DESCRIPTION@"};

constexpr const wchar_t RegistrySubKey[] {L"SOFTWARE\\VisorVR"};

constexpr const wchar_t VariableWidthUIFont[] {L"Segoe UI"};
constexpr const wchar_t GlyphFont[] {L"Segoe MDL2 Assets"};
constexpr const wchar_t FixedWidthUIFont[] {L"Consolas"};
constexpr const wchar_t FixedWidthContentFont[] {L"Consolas"};

}// namespace VisorVR::inline Config

#ifdef DEBUG
#define VISORVR_BREAK __debugbreak()
#else
#define VISORVR_BREAK
#endif

// clang-format off
// @formatter:off
#define VISORVR_BUILD_BITNESS @BUILD_BITNESS@
// @formatter:on
// clang-format on
#if (VISORVR_BUILD_BITNESS == 32)
#define VISORVR_32BIT_BUILD 1
#endif
#if (VISORVR_BUILD_BITNESS == 64)
#define VISORVR_64BIT_BUILD 1
#endif

#if defined(_MSVC_LANG) && (!defined(__clang__)) && !defined(CLANG_CL)
#define VISORVR_FORCEINLINE [[msvc::forceinline]]
#define VISORVR_NOINLINE [[msvc::noinline]]
#elif defined(__clang__)
#define VISORVR_FORCEINLINE [[clang::always_inline]]
#define VISORVR_NOINLINE [[clang::noinline]]
#else
#define VISORVR_FORCEINLINE [[]]
#define VISORVR_NOINLINE [[]]
#endif