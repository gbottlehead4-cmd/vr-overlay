// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/DXResources.hpp>
#include <VisorVR/RenderTargetID.hpp>

#include <VisorVR/audited_ptr.hpp>

#include <winrt/VisorVRApp.h>

#include <memory>
#include <vector>

namespace VisorVR {
class KneeboardState;
class TroubleshootingStore;

extern HWND gMainWindow;
extern audited_weak_ptr<KneeboardState> gKneeboard;
extern audited_ptr<DXResources> gDXResources;
extern winrt::handle gMutex;
extern std::weak_ptr<TroubleshootingStore> gTroubleshootingStore;
extern std::vector<winrt::weak_ref<winrt::VisorVRApp::TabPage>> gTabs;
extern RenderTargetID gGUIRenderTargetID;
extern bool gShuttingDown;

}// namespace VisorVR
