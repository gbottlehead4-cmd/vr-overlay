// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

// clang-format off
#include "pch.h"
#include "TabsSettingsPage.g.h"

#include "TabUIData.g.h"
#include "BrowserTabUIData.g.h"
#include "DCSRadioLogTabUIData.g.h"
#include "WindowCaptureTabUIData.g.h"

#include "TabUIDataTemplateSelector.g.h"
// clang-format on

#include "WithPropertyChangedEvent.h"

#include <VisorVR/Events.hpp>

#include <VisorVR/audited_ptr.hpp>
#include <VisorVR/task.hpp>

#include <optional>
#include <string>

namespace VisorVR {
class ITab;
class TabView;
class BrowserTab;
class DCSRadioLogTab;
struct DXResources;
class KneeboardState;
class WindowCaptureTab;
enum class TabType;
}// namespace VisorVR

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Controls::Primitives;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::VisorVRApp::implementation {
struct TabsSettingsPage : TabsSettingsPageT<TabsSettingsPage>,
                          VisorVR::EventReceiver,
                          VisorVR::WithPropertyChangedEvent {
  TabsSettingsPage();
  ~TabsSettingsPage() noexcept override;

  IVector<IInspectable> Tabs() noexcept;

  VisorVR::fire_and_forget RestoreDefaults(
    IInspectable,
    RoutedEventArgs) noexcept;

  VisorVR::fire_and_forget CreateTab(
    IInspectable,
    RoutedEventArgs) noexcept;
  VisorVR::fire_and_forget CreatePluginTab(
    IInspectable,
    RoutedEventArgs) noexcept;
  VisorVR::fire_and_forget RemoveTab(IInspectable, RoutedEventArgs);
  VisorVR::fire_and_forget ShowTabSettings(IInspectable, RoutedEventArgs);
  VisorVR::fire_and_forget ToggleVREditMode(IInspectable, RoutedEventArgs);
  VisorVR::fire_and_forget RecenterVR(IInspectable, RoutedEventArgs);
  VisorVR::fire_and_forget GoToInputBindings(IInspectable, RoutedEventArgs);
  VisorVR::fire_and_forget ShowDebugInfo(IInspectable, RoutedEventArgs);
  void CopyDebugInfo(const IInspectable&, const RoutedEventArgs&);

  VisorVR::fire_and_forget OnTabsChanged(
    IInspectable,
    Windows::Foundation::Collections::IVectorChangedEventArgs) noexcept;
  void OnAddBrowserAddressTextChanged(
    const IInspectable&,
    const IInspectable&) noexcept;

 private:
  void CreateAddTabMenu(const Button& button, FlyoutPlacementMode);

  template <class T>
  VisorVR::fire_and_forget CreateFileTab(
    const std::string& pickerDialogTitle = {});
  VisorVR::fire_and_forget CreateFolderTab();
  VisorVR::fire_and_forget CreateWindowCaptureTab();
  VisorVR::fire_and_forget CreateBrowserTab();
  winrt::guid GetFilePickerPersistenceGuid();

  task<void> AddTabs(const std::vector<std::shared_ptr<VisorVR::ITab>>&);
  static VisorVRApp::TabUIData CreateTabUIData(
    const std::shared_ptr<VisorVR::ITab>&);

  bool mUIIsChangingTabs = false;

  VisorVR::audited_ptr<VisorVR::DXResources> mDXR;
  VisorVR::audited_ptr<VisorVR::KneeboardState> mKneeboard;
};

struct TabUIData : TabUIDataT<TabUIData>,
                   VisorVR::EventReceiver,
                   VisorVR::WithPropertyChangedEvent {
  TabUIData() = default;
  ~TabUIData();

  uint64_t InstanceID() const;
  void InstanceID(uint64_t);

  hstring Title() const;
  void Title(hstring);

  bool HasDebugInformation() const;
  hstring DebugInformation() const;

  bool HasVRPlacement() const;
  bool IsVREnabled() const;
  VisorVR::fire_and_forget IsVREnabled(bool);
  float VRWidth() const;
  VisorVR::fire_and_forget VRWidth(float);
  float VRHeight() const;
  VisorVR::fire_and_forget VRHeight(float);
  float VRDistance() const;
  VisorVR::fire_and_forget VRDistance(float);
  int32_t IconIndex() const;
  void IconIndex(int32_t);

  bool HasRenderScale() const;
  double RenderScalePercent() const;
  VisorVR::fire_and_forget RenderScalePercent(double);
  double MaxRenderScalePercent() const;
  hstring RenderResolutionDescription() const;

 protected:
  std::weak_ptr<VisorVR::ITab> mTab;

 private:
  std::optional<winrt::guid> GetVRViewID() const;
  /// Null unless this is a web panel; only those have a render scale.
  std::shared_ptr<VisorVR::BrowserTab> GetBrowserTab() const;
};

struct BrowserTabUIData : BrowserTabUIDataT<
                            BrowserTabUIData,
                            VisorVRApp::implementation::TabUIData> {
  BrowserTabUIData() = default;

  bool IsSimHubIntegrationEnabled() const;
  VisorVR::fire_and_forget IsSimHubIntegrationEnabled(bool);

  bool AreOpenKneeboardAPIsEnabled() const;
  VisorVR::fire_and_forget AreOpenKneeboardAPIsEnabled(bool);

  bool IsBackgroundTransparent() const;
  VisorVR::fire_and_forget IsBackgroundTransparent(bool);

 private:
  std::shared_ptr<VisorVR::BrowserTab> GetTab() const;
};

struct DCSRadioLogTabUIData : DCSRadioLogTabUIDataT<
                                DCSRadioLogTabUIData,
                                VisorVRApp::implementation::TabUIData> {
  DCSRadioLogTabUIData() = default;
  uint8_t MissionStartBehavior() const;
  void MissionStartBehavior(uint8_t value);
  bool TimestampsEnabled() const;
  void TimestampsEnabled(bool value);

 private:
  std::shared_ptr<VisorVR::DCSRadioLogTab> GetTab() const;
};

struct WindowCaptureTabUIData : WindowCaptureTabUIDataT<
                                  WindowCaptureTabUIData,
                                  VisorVRApp::implementation::TabUIData> {
  WindowCaptureTabUIData() = default;

  using fire_and_forget = VisorVR::fire_and_forget;

  hstring WindowTitle();
  fire_and_forget WindowTitle(hstring const& value);
  bool MatchWindowClass();
  fire_and_forget MatchWindowClass(bool value);
  uint8_t MatchWindowTitle();
  fire_and_forget MatchWindowTitle(uint8_t value);
  bool IsInputEnabled() const;
  void IsInputEnabled(bool value);
  bool IsCursorCaptureEnabled() const;
  fire_and_forget IsCursorCaptureEnabled(bool value);
  bool CaptureClientArea() const;
  fire_and_forget CaptureClientArea(bool value);
  hstring ExecutablePathPattern() const;
  fire_and_forget ExecutablePathPattern(hstring);
  hstring WindowClass() const;
  fire_and_forget WindowClass(hstring);

 private:
  std::shared_ptr<VisorVR::WindowCaptureTab> GetTab() const;
};

struct TabUIDataTemplateSelector
  : TabUIDataTemplateSelectorT<TabUIDataTemplateSelector> {
  TabUIDataTemplateSelector() = default;

  winrt::Microsoft::UI::Xaml::DataTemplate Generic();
  void Generic(winrt::Microsoft::UI::Xaml::DataTemplate const& value);
  winrt::Microsoft::UI::Xaml::DataTemplate Browser();
  void Browser(winrt::Microsoft::UI::Xaml::DataTemplate const& value);
  winrt::Microsoft::UI::Xaml::DataTemplate DCSRadioLog();
  void DCSRadioLog(winrt::Microsoft::UI::Xaml::DataTemplate const& value);
  winrt::Microsoft::UI::Xaml::DataTemplate WindowCapture();
  void WindowCapture(winrt::Microsoft::UI::Xaml::DataTemplate const& value);

  DataTemplate SelectTemplateCore(const IInspectable&);
  DataTemplate SelectTemplateCore(const IInspectable&, const DependencyObject&);

 private:
  DataTemplate mGeneric;
  DataTemplate mBrowser;
  DataTemplate mDCSRadioLog;
  DataTemplate mWindowCapture;
};

}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct TabsSettingsPage
  : TabsSettingsPageT<TabsSettingsPage, implementation::TabsSettingsPage> {};

struct TabUIData : TabUIDataT<TabUIData, implementation::TabUIData> {};
struct BrowserTabUIData
  : BrowserTabUIDataT<BrowserTabUIData, implementation::BrowserTabUIData> {};
struct DCSRadioLogTabUIData : DCSRadioLogTabUIDataT<
                                DCSRadioLogTabUIData,
                                implementation::DCSRadioLogTabUIData> {};
struct WindowCaptureTabUIData : WindowCaptureTabUIDataT<
                                  WindowCaptureTabUIData,
                                  implementation::WindowCaptureTabUIData> {};

struct TabUIDataTemplateSelector
  : TabUIDataTemplateSelectorT<
      TabUIDataTemplateSelector,
      implementation::TabUIDataTemplateSelector> {};

}// namespace winrt::VisorVRApp::factory_implementation
