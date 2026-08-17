// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Events.hpp>
#include <VisorVR/IToolbarFlyout.hpp>
#include <VisorVR/IToolbarItemWithVisibility.hpp>

#include <vector>

namespace VisorVR {

class KneeboardState;
class KneeboardView;

class SwitchProfileFlyout final : public EventReceiver,
                                  public virtual IToolbarFlyout,
                                  public virtual IToolbarItemWithVisibility {
 public:
  SwitchProfileFlyout(KneeboardState*);
  ~SwitchProfileFlyout();

  std::string_view GetGlyph() const override;
  std::string_view GetLabel() const override;
  virtual bool IsEnabled() const override;

  virtual bool IsVisible() const override;

  virtual std::vector<std::shared_ptr<IToolbarItem>> GetSubItems()
    const override;

  SwitchProfileFlyout() = delete;

 private:
  KneeboardState* mKneeboardState {nullptr};
};

}// namespace VisorVR
