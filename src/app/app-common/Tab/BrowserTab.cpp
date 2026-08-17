// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/BrowserTab.hpp>

#include <VisorVR/json/Geometry2D.hpp>

#include <VisorVR/dprint.hpp>

#include <algorithm>
#include <cmath>

namespace VisorVR {

BrowserTab::BrowserTab(
  const audited_ptr<DXResources>& dxr,
  KneeboardState* kbs,
  const winrt::guid& persistentID,
  std::string_view title,
  const Settings& settings)
  : TabBase(
      persistentID,
      title.empty() ? std::string_view {_("Web Dashboard")} : title),
    PageSourceWithDelegates(dxr, kbs),
    mDXR(dxr),
    mKneeboard(kbs),
    mSettings(settings),
    mHaveTitle(!title.empty()) {}

task<std::shared_ptr<BrowserTab>> BrowserTab::Create(
  audited_ptr<DXResources> dxr,
  KneeboardState* kbs,
  winrt::guid persistentID,
  std::string_view title,
  Settings settings) {
  std::shared_ptr<BrowserTab> ret {
    new BrowserTab(dxr, kbs, persistentID, title, settings)};
  co_await ret->Reload();
  co_return ret;
}

BrowserTab::~BrowserTab() {
  VISORVR_TraceLoggingScope("BrowserTab::~BrowserTab()");
  this->RemoveAllEventListeners();
}

std::string BrowserTab::GetGlyph() const { return GetStaticGlyph(); }

std::string BrowserTab::GetStaticGlyph() {
  // Website
  return {"\ueb41"};
}

task<void> BrowserTab::Reload() {
  VISORVR_TraceLoggingCoro("BrowserTab::Reload()");
  auto keepAlive = shared_from_this();

  mDelegate = {};
  co_await this->SetDelegates({});
  mDelegate = co_await ChromiumPageSource::Create(
    mDXR, mKneeboard, WebPageSourceKind::WebDashboard, mSettings);
  this->RemoveAllEventListeners();
  AddEventListener(
    mDelegate->evDocumentTitleChangedEvent,
    {
      this,
      [](auto self, auto title) -> fire_and_forget {
        if (self->mHaveTitle) {
          co_return;
        }
        co_await self->mUIThread;
        self->SetTitle(title);
        self->mHaveTitle = true;
      },
    });
  co_await this->SetDelegates({mDelegate});
}

nlohmann::json BrowserTab::GetSettings() const { return mSettings; }

bool BrowserTab::IsSimHubIntegrationEnabled() const {
  return mSettings.mIntegrateWithSimHub;
}

task<void> BrowserTab::SetSimHubIntegrationEnabled(bool enabled) {
  VISORVR_TraceLoggingCoro("BrowserTab::SetSimHubIntegrationEnabled()");
  if (enabled == this->IsSimHubIntegrationEnabled()) {
    co_return;
  }
  mSettings.mIntegrateWithSimHub = enabled;
  co_await this->Reload();
  this->evSettingsChangedEvent.Emit();
}

bool BrowserTab::AreOpenKneeboardAPIsEnabled() const {
  return mSettings.mExposeOpenKneeboardAPIs;
}

task<void> BrowserTab::SetOpenKneeboardAPIsEnabled(const bool enabled) {
  VISORVR_TraceLoggingCoro("BrowserTab::SetOpenKneeboardAPIsEnabled()");
  if (enabled == this->AreOpenKneeboardAPIsEnabled()) {
    co_return;
  }
  mSettings.mExposeOpenKneeboardAPIs = enabled;
  co_await this->Reload();
  this->evSettingsChangedEvent.Emit();
}

bool BrowserTab::IsBackgroundTransparent() const {
  return mSettings.mTransparentBackground;
}

task<void> BrowserTab::SetBackgroundTransparent(bool transparent) {
  VISORVR_TraceLoggingCoro("BrowserTab::SetBackgroundTransparent()");
  if (transparent == this->IsBackgroundTransparent()) {
    co_return;
  }
  mSettings.mTransparentBackground = transparent;
  co_await this->Reload();
  this->evSettingsChangedEvent.Emit();
}

float BrowserTab::GetRenderScale() const {
  return mSettings.GetUsableRenderScale();
}

task<void> BrowserTab::SetRenderScale(const float scale) {
  VISORVR_TraceLoggingCoro("BrowserTab::SetRenderScale()");
  const auto clamped = std::clamp(scale, 1.0f, this->GetMaxRenderScale());
  if (clamped == this->GetRenderScale()) {
    co_return;
  }
  mSettings.mRenderScale = clamped;
  // Reload() rebuilds the page source from mSettings; that's what picks the
  // new device scale factor up.
  co_await this->Reload();
  this->evSettingsChangedEvent.Emit();
}

float BrowserTab::GetMaxRenderScale() const {
  return MaxUsefulRenderScale(mSettings.mInitialSize);
}

PixelSize BrowserTab::GetRenderPixelSize() const {
  const auto scale = this->GetRenderScale();
  return {
    static_cast<uint32_t>(std::lround(mSettings.mInitialSize.mWidth * scale)),
    static_cast<uint32_t>(std::lround(mSettings.mInitialSize.mHeight * scale)),
  };
}

VISORVR_DEFINE_SPARSE_JSON(
  BrowserTab::Settings,
  mURI,
  mInitialSize,
  mRenderScale,
  mIntegrateWithSimHub,
  mTransparentBackground,
  mExposeOpenKneeboardAPIs)

}// namespace VisorVR
