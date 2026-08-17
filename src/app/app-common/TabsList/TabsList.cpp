// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/BrowserTab.hpp>
#include <VisorVR/Filesystem.hpp>
#include <VisorVR/KneeboardState.hpp>
#include <VisorVR/PluginTab.hpp>
#include <VisorVR/RuntimeFiles.hpp>
#include <VisorVR/TabBase.hpp>
#include <VisorVR/TabTypes.hpp>
#include <VisorVR/TabView.hpp>
#include <VisorVR/TabsList.hpp>

#include <VisorVR/dprint.hpp>

#include <shims/nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <vector>

namespace VisorVR {

TabsList::TabsList(
  const audited_ptr<DXResources>& dxr,
  KneeboardState* kneeboard)
  : mDXR(dxr),
    mKneeboard(kneeboard) {}

task<std::shared_ptr<TabsList>> TabsList::Create(
  const audited_ptr<DXResources>& dxr,
  KneeboardState* kneeboard,
  const nlohmann::json& config) {
  std::unique_ptr<TabsList> ret {new TabsList(dxr, kneeboard)};
  co_await ret->LoadSettings(config);
  co_return ret;
}

TabsList::~TabsList() { this->RemoveAllEventListeners(); }

static std::tuple<std::string, nlohmann::json> MigrateTab(
  const std::string& type,
  const nlohmann::json& settings) {
  if (type == "PDF" || type == "TextFile") {
    return {"SingleFile", settings};
  }

  return {type, settings};
}

static void RestoreSavedBookmarks(ITab& tab, const nlohmann::json& bmArray) {
  if (!bmArray.is_array()) {
    return;
  }

  std::vector<ITab::PersistentBookmark> restored;
  for (const auto& entry: bmArray) {
    if (!entry.contains("PersistentID") || !entry.contains("Title")) {
      continue;
    }
    const auto title = entry.at("Title").get<std::string>();
    const auto persistentId = entry.at("PersistentID").get<std::string>();
    restored.emplace_back(persistentId, title);
  }

  tab.SetPersistentBookmarks(std::move(restored));
}

static nlohmann::json SerializeTabBookmarks(const ITab& tab) {
  nlohmann::json ret = nlohmann::json::array();

  auto bookmarks = tab.GetPersistentBookmarks();

  for (auto&& [persistentID, title]: bookmarks) {
    ret.push_back({
      {"PersistentID", std::move(persistentID)},
      {"Title", std::move(title)},
    });
  }
  return ret;
}

task<std::shared_ptr<ITab>> TabsList::LoadTabFromJSON(
  const nlohmann::json tab) {
  if (!(tab.contains("Type") && tab.contains("Title"))) {
    co_return nullptr;
  }

  const std::string title = tab.at("Title");
  const std::string rawType = tab.at("Type");

  nlohmann::json rawSettings;
  if (tab.contains("Settings")) {
    rawSettings = tab.at("Settings");
  }

  const auto [type, settings] = MigrateTab(rawType, rawSettings);

  winrt::guid persistentID {};
  if (tab.contains("ID")) {
    persistentID = winrt::guid {tab.at("ID").get<std::string>()};
    // else handled by TabBase
  }

  std::shared_ptr<ITab> result;
  try {
#define IT(_, it) \
  if (type == #it) { \
    auto instance = co_await load_tab<it##Tab>( \
      mDXR, mKneeboard, persistentID, title, settings); \
    if (instance) { \
      result = instance; \
    } \
  }
    VISORVR_TAB_TYPES
#undef IT
    if (!result && type == "Plugin") {
      result = co_await PluginTab::Create(
        mDXR, mKneeboard, persistentID, title, settings);
    }
  } catch (const std::exception& e) {
    dprint.Error(
      "Failed to load tab {} with std::exception: {}",
      tab.value<std::string>("ID", "<no GUID>"),
      e.what());
    throw;
  } catch (const winrt::hresult_error& e) {
    dprint.Error(
      "Failed to load tab {} with HRESULT: {}",
      tab.value<std::string>("ID", "<no GUID>"),
      winrt::to_string(e.message()));
    throw;
  }

  if (!result) {
    dprint("Couldn't load tab with type {}", rawType);
    VISORVR_BREAK;
    co_return nullptr;
  }

  if (tab.contains("Icon")) {
    result->SetIcon(tab.at("Icon").get<std::string>());
  }

  if (tab.contains("Bookmarks")) {
    RestoreSavedBookmarks(*result, tab.at("Bookmarks"));
  }

  co_return result;
}

task<void> TabsList::LoadSettings(nlohmann::json config) {
  if (config.is_null()) {
    co_await LoadDefaultSettings();
    co_return;
  }
  const std::vector<nlohmann::json> jsonTabs = config;

  std::vector<task<std::shared_ptr<ITab>>> awaitables;
  for (auto&& tab: jsonTabs) {
    awaitables.push_back(this->LoadTabFromJSON(tab));
  }

  decltype(mTabs) tabs;
  for (auto&& it: awaitables) {
    auto tab = co_await std::move(it);
    if (tab) {
      tabs.push_back(std::move(tab));
    }
  }

  co_await this->SetTabs(tabs);
}

task<void> TabsList::LoadDefaultSettings() {
  // A fresh install gets exactly one panel: a local page explaining how to add
  // the rest. The DCS panels used to be the defaults, so every new user -
  // including everyone who came for sim racing - started with five panels for
  // a game they may not own. They are still one click away in Settings ->
  // Panels.
  const auto welcome
    = Filesystem::GetRuntimeDirectory() / RuntimeFiles::WELCOME_HTML;

  BrowserTab::Settings settings {};
  settings.mURI = "file:///" + welcome.generic_string();
  // The page paints its own background; a transparent one would leave the
  // text floating over the game.
  settings.mTransparentBackground = false;

  co_await this->SetTabs({co_await BrowserTab::Create(
    mDXR, mKneeboard, random_guid(), "Welcome", settings)});
}

nlohmann::json TabsList::GetSettings() const {
  std::vector<nlohmann::json> ret;

  for (const auto& tab: mTabs) {
    std::string type;
#define IT(_, it) \
  if (type.empty() && std::dynamic_pointer_cast<it##Tab>(tab)) { \
    type = #it; \
  }
    VISORVR_TAB_TYPES
#undef IT
    if (type.empty() && std::dynamic_pointer_cast<PluginTab>(tab)) {
      type = "Plugin";
    }
    if (type.empty()) {
      dprint("Unknown type for tab {}", tab->GetTitle());
      VISORVR_BREAK;
      continue;
    }

    nlohmann::json savedTab {
      {"Type", type},
      {"Title", tab->GetTitle()},
      {"ID", winrt::to_string(winrt::to_hstring(tab->GetPersistentID()))},
    };

    // Only persist an icon the user actually picked; otherwise the tab
    // type's own glyph is used.
    if (const auto icon = tab->GetIcon(); icon != tab->GetGlyph()) {
      savedTab.emplace("Icon", icon);
    }

    auto withSettings = std::dynamic_pointer_cast<ITabWithSettings>(tab);
    if (withSettings) {
      auto settings = withSettings->GetSettings();
      if (!settings.is_null()) {
        savedTab.emplace("Settings", settings);
      }
    }

    auto bmArray = SerializeTabBookmarks(*tab);
    if (!bmArray.empty()) {
      savedTab.emplace("Bookmarks", std::move(bmArray));
    }

    ret.push_back(savedTab);
    continue;
  }

  return ret;
}

std::vector<std::shared_ptr<ITab>> TabsList::GetTabs() const { return mTabs; }

task<void> TabsList::SetTabs(std::vector<std::shared_ptr<ITab>> tabs) {
  if (std::ranges::equal(tabs, mTabs)) {
    co_return;
  }

  {
    const auto oldTabs = std::exchange(mTabs, {});
    std::vector<task<void>> disposers;
    for (auto tab: oldTabs) {
      if (!std::ranges::contains(tabs, tab->GetRuntimeID(), [](auto it) {
            return it->GetRuntimeID();
          })) {
        if (auto p = std::dynamic_pointer_cast<IHasDisposeAsync>(tab)) {
          disposers.push_back(p->DisposeAsync());
        }
      }
    }
    for (auto& it: disposers) {
      co_await std::move(it);
    }
  }

  mTabs = tabs;
  for (auto token: mTabEvents) {
    RemoveEventListener(token);
  }
  mTabEvents.clear();
  for (const auto& tab: mTabs) {
    mTabEvents.push_back(AddEventListener(
      tab->evSettingsChangedEvent, this->evSettingsChangedEvent));
  }

  evTabsChangedEvent.Emit();
  evSettingsChangedEvent.Emit();
}

task<void> TabsList::InsertTab(TabIndex index, std::shared_ptr<ITab> tab) {
  auto tabs = mTabs;
  tabs.insert(tabs.begin() + index, tab);
  co_await this->SetTabs(tabs);
}

task<void> TabsList::RemoveTab(TabIndex index) {
  if (index >= mTabs.size()) {
    co_return;
  }

  auto tabs = mTabs;
  tabs.erase(tabs.begin() + index);
  co_await this->SetTabs(tabs);
}

}// namespace VisorVR
