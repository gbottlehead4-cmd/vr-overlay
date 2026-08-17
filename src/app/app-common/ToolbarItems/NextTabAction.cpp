// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/AppSettings.hpp>
#include <VisorVR/KneeboardState.hpp>
#include <VisorVR/KneeboardView.hpp>
#include <VisorVR/NextTabAction.hpp>

namespace VisorVR {

NextTabAction::NextTabAction(
  KneeboardState* kneeboardState,
  const std::shared_ptr<KneeboardView>& kneeboardView)
  : ToolbarAction("\uE74B", _("Next Tab")),
    mKneeboardState(kneeboardState),
    mKneeboardView(kneeboardView) {
  AddEventListener(
    kneeboardView->evCurrentTabChangedEvent, this->evStateChangedEvent);
  AddEventListener(
    kneeboardState->evSettingsChangedEvent, this->evStateChangedEvent);
}

NextTabAction::~NextTabAction() { this->RemoveAllEventListeners(); }

bool NextTabAction::IsEnabled() const {
  const auto count = mKneeboardState->GetTabsList()->GetTabs().size();

  if (count < 2) {
    return false;
  }

  if (mKneeboardState->GetUISettings().mLoopTabs) {
    return true;
  }

  auto kbv = mKneeboardView.lock();
  return kbv && ((kbv->GetTabIndex() + 1) < count);
}

task<void> NextTabAction::Execute() {
  auto kbv = mKneeboardView.lock();
  if (kbv) {
    kbv->NextTab();
  }
  co_return;
}

}// namespace VisorVR
