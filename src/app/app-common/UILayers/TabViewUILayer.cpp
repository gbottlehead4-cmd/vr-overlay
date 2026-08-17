// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/CursorEvent.hpp>
#include <VisorVR/D2DErrorRenderer.hpp>
#include <VisorVR/DXResources.hpp>
#include <VisorVR/ITab.hpp>
#include <VisorVR/TabView.hpp>
#include <VisorVR/TabViewUILayer.hpp>

#include <VisorVR/config.hpp>
#include <VisorVR/utf8.hpp>

namespace VisorVR {

TabViewUILayer::TabViewUILayer(const audited_ptr<DXResources>& dxr) {
  mErrorRenderer = std::make_unique<D2DErrorRenderer>(dxr);
  dxr->mD2DDeviceContext->CreateSolidColorBrush(
    {1.0f, 1.0f, 1.0f, 1.0f},
    D2D1::BrushProperties(),
    mErrorBackgroundBrush.put());
}
TabViewUILayer::~TabViewUILayer() = default;

void TabViewUILayer::PostCursorEvent(
  const IUILayer::NextList&,
  const Context& context,
  KneeboardViewID,
  const CursorEvent& ev) {
  if (ev.mTouchState == CursorTouchState::NotNearSurface) {
    mCursorPoint.reset();
    context.mTabView->PostCursorEvent({});
    return;
  }
  mCursorPoint = {ev.mX, ev.mY};
  context.mTabView->PostCursorEvent(ev);
}

IUILayer::Metrics TabViewUILayer::GetMetrics(
  const IUILayer::NextList&,
  const Context& context) const {
  const constexpr Metrics errorMapping {
    ErrorPreferredSize,
    {{}, ErrorPixelSize},
    {{}, ErrorPixelSize},
  };
  auto tabView = context.mTabView;
  if (!tabView) {
    return errorMapping;
  }
  const auto nextSize = tabView->GetPreferredSize();
  if (!nextSize.has_value()) {
    return errorMapping;
  }

  const auto& ps = nextSize->mPixelSize;

  if (ps.mWidth == 0 || ps.mHeight == 0) {
    return errorMapping;
  }

  return {
    nextSize.value(),
    {{}, ps},
    {{}, ps},
  };
}

task<void> TabViewUILayer::Render(
  const RenderContext& rc,
  const IUILayer::NextList&,
  const Context& context,
  const PixelRect& rect) {
  const auto tabView = context.mTabView;
  std::shared_ptr<ITab> tab;
  if (tabView) {
    tab = tabView->GetTab().lock();
  }

  VISORVR_TraceLoggingScope(
    "TabViewUILayer::Render()",
    TraceLoggingHexUInt64(
      rc.GetRenderTarget()->GetID().GetTemporaryValue(), "RenderTargetID"),
    TraceLoggingGuid((tab ? tab->GetPersistentID() : winrt::guid {}), "TabID"),
    TraceLoggingHexUInt64(
      tabView ? tabView->GetPageID().GetTemporaryValue() : 0ui64, "PageID"));

  if (!tabView) {
    this->RenderError(rc.d2d(), _("No Tab View"), rect);
    co_return;
  }
  if (!tab) {
    this->RenderError(rc.d2d(), _("No Tab"), rect);
    co_return;
  }
  const auto pageCount = tab->GetPageCount();
  if (pageCount == 0) {
    this->RenderError(rc.d2d(), _("No Pages"), rect);
    co_return;
  }

  co_await tab->RenderPage(rc, tabView->GetPageID(), rect);
}

void TabViewUILayer::RenderError(
  ID2D1DeviceContext* d2d,
  std::string_view text,
  const PixelRect& rect) {
  d2d->FillRectangle(rect, mErrorBackgroundBrush.get());
  mErrorRenderer->Render(d2d, text, rect);
}

std::optional<D2D1_POINT_2F> TabViewUILayer::GetCursorPoint() const {
  return mCursorPoint;
}

}// namespace VisorVR
