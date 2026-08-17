// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/ITabWithSettings.hpp>
#include <VisorVR/KneeboardViewID.hpp>
#include <VisorVR/PageSourceWithDelegates.hpp>
#include <VisorVR/Plugin.hpp>
#include <VisorVR/TabBase.hpp>

#include <VisorVR/json.hpp>

#include <optional>

namespace VisorVR {
class D2DErrorRenderer;

class PluginTab final : public TabBase,
                        public ITabWithSettings,
                        public PageSourceWithDelegates {
 public:
  struct Settings {
    std::string mPluginTabTypeID;

    bool operator==(const Settings&) const noexcept = default;
  };

  PluginTab() = delete;
  static task<std::shared_ptr<PluginTab>> Create(
    const audited_ptr<DXResources>&,
    KneeboardState*,
    const winrt::guid& persistentID,
    std::string_view title,
    const Settings&);

  ~PluginTab();

  std::string GetGlyph() const override;
  task<void> Reload() final override;
  nlohmann::json GetSettings() const override;
  PageIndex GetPageCount() const override;
  task<void> RenderPage(RenderContext, PageID, PixelRect rect) override;

  std::string GetPluginTabTypeID() const noexcept;

  void PostCustomAction(
    KneeboardViewID,
    std::string_view id,
    const nlohmann::json& arg);

 private:
  enum class State {
    OK,
    Uninit,
    PluginNotFound,
    VisorVRTooOld,
  };

  PluginTab(
    const audited_ptr<DXResources>&,
    KneeboardState*,
    const winrt::guid& persistentID,
    std::string_view title,
    const Settings&);
  audited_ptr<DXResources> mDXResources;
  KneeboardState* mKneeboard {nullptr};
  Settings mSettings;

  State mState {State::Uninit};
  std::unique_ptr<D2DErrorRenderer> mErrorRenderer;

  std::shared_ptr<IPageSource> mDelegate;

  std::optional<Plugin::TabType> mTabType;
};

VISORVR_DECLARE_SPARSE_JSON(PluginTab::Settings);

}// namespace VisorVR
