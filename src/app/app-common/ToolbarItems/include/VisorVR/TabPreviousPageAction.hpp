// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/ToolbarAction.hpp>
#include <VisorVR/UserActionHandler.hpp>

namespace VisorVR {

class TabView;

class TabPreviousPageAction final : public ToolbarAction,
                                    private EventReceiver,
                                    public UserActionHandler {
 public:
  TabPreviousPageAction() = delete;

  TabPreviousPageAction(KneeboardState*, const std::shared_ptr<TabView>& state);
  ~TabPreviousPageAction();

  virtual bool IsEnabled() const override;
  [[nodiscard]]
  virtual task<void> Execute() override;

 private:
  KneeboardState* mKneeboard;
  std::weak_ptr<TabView> mTabView;
};

}// namespace VisorVR
