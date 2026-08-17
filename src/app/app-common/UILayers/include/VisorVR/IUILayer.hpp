// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Events.hpp>
#include <VisorVR/KneeboardView.hpp>
#include <VisorVR/RenderTarget.hpp>
#include <VisorVR/TabView.hpp>

#include <memory>
#include <span>

#include <d2d1_1.h>

namespace VisorVR {

struct CursorEvent;

class IUILayer {
 public:
  IUILayer();
  virtual ~IUILayer();

  using NextList = std::span<IUILayer*>;

  struct Context {
    const std::shared_ptr<TabView>& mTabView {nullptr};
    const std::shared_ptr<KneeboardView>& mKneeboardView {nullptr};
    bool mIsActiveForInput {false};
  };

  virtual void PostCursorEvent(
    const NextList&,
    const Context&,
    KneeboardViewID,
    const CursorEvent&) = 0;

  [[nodiscard]] virtual task<void> Render(
    const RenderContext&,
    const NextList&,
    const Context&,
    const PixelRect&) = 0;

  struct Metrics {
    PreferredSize mPreferredSize {};
    PixelRect mNextArea {};
    PixelRect mContentArea {};

    Metrics() = delete;
    constexpr Metrics(
      PreferredSize preferredSize,
      PixelRect nextArea,
      PixelRect contentArea)
      : mPreferredSize(preferredSize),
        mNextArea(nextArea),
        mContentArea(contentArea) {}
  };

  virtual Metrics GetMetrics(const NextList&, const Context&) const = 0;

  Event<> evNeedsRepaintEvent;
};

}// namespace VisorVR
