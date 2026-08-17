// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Events.hpp>
#include <VisorVR/ITab.hpp>
#include <VisorVR/KneeboardState.hpp>

#include <VisorVR/utf8.hpp>

#include <filesystem>

namespace VisorVR {

struct APIEvent;

class DCSTab : public virtual ITab, public virtual EventReceiver {
 public:
  DCSTab(KneeboardState*);
  virtual ~DCSTab();

  DCSTab() = delete;

 protected:
  static constexpr std::string_view DebugInformationHeader = _(
    "A tick or a cross indicates whether or not the folder exists, not whether "
    "or not it is meant to exist. Some crosses are expected, and not "
    "necessarily an error.\n");

  virtual VisorVR::fire_and_forget OnAPIEvent(
    APIEvent,
    std::filesystem::path installPath,
    std::filesystem::path savedGamesPath) = 0;

  std::filesystem::path ToAbsolutePath(const std::filesystem::path&);

 private:
  std::filesystem::path mInstallPath;
  std::filesystem::path mSavedGamesPath;
  EventHandlerToken mAPIEventToken;

  void OnAPIEvent(const APIEvent&);
};

}// namespace VisorVR
