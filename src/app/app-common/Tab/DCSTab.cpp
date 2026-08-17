// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/APIEvent.hpp>
#include <VisorVR/DCSEvents.hpp>
#include <VisorVR/DCSTab.hpp>
#include <VisorVR/KneeboardState.hpp>

#include <VisorVR/dprint.hpp>

namespace VisorVR {

DCSTab::DCSTab(KneeboardState* kbs) {
  mAPIEventToken = AddEventListener(
    kbs->evAPIEvent, [this](const APIEvent& ev) { this->OnAPIEvent(ev); });
}

DCSTab::~DCSTab() { this->RemoveEventListener(mAPIEventToken); }

void DCSTab::OnAPIEvent(const APIEvent& event) {
  if (event.name == DCSEvents::EVT_INSTALL_PATH) {
    mInstallPath = std::filesystem::canonical(event.value);
  }

  if (event.name == DCSEvents::EVT_SAVED_GAMES_PATH) {
    mSavedGamesPath = std::filesystem::canonical(event.value);
  }

  if (!(mInstallPath.empty() || mSavedGamesPath.empty())) {
    OnAPIEvent(event, mInstallPath, mSavedGamesPath);
  }
}

std::filesystem::path DCSTab::ToAbsolutePath(
  const std::filesystem::path& maybeRelative) {
  if (maybeRelative.empty()) {
    return {};
  }

  if (std::filesystem::exists(maybeRelative)) {
    return std::filesystem::canonical(maybeRelative);
  }

  for (const auto& prefix: {mInstallPath, mSavedGamesPath}) {
    if (prefix.empty()) {
      continue;
    }

    const auto path = prefix / maybeRelative;
    if (std::filesystem::exists(path)) {
      return std::filesystem::canonical(path);
    }
  }

  return maybeRelative;
}

}// namespace VisorVR
