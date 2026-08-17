// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
// clang-format off
#include "pch.h"
#include "TabsSettingsPage.xaml.h"
#include "TabsSettingsPage.g.cpp"

#include "TabUIData.g.cpp"
#include "BrowserTabUIData.g.cpp"
#include "DCSRadioLogTabUIData.g.cpp"
#include "WindowCaptureTabUIData.g.cpp"

#include "TabUIDataTemplateSelector.g.cpp"
// clang-format on

#include "FilePicker.h"
#include "Globals.h"

#include <VisorVR/BrowserTab.hpp>
#include <VisorVR/DCSRadioLogTab.hpp>
#include <VisorVR/FilePageSource.hpp>
#include <VisorVR/IHasDebugInformation.hpp>
#include <VisorVR/ITab.hpp>
#include <VisorVR/KneeboardState.hpp>
#include <VisorVR/LaunchURI.hpp>
#include <VisorVR/PluginStore.hpp>
#include <VisorVR/PluginTab.hpp>
#include <VisorVR/TabTypes.hpp>
#include <VisorVR/TabView.hpp>
#include <VisorVR/TabsList.hpp>
#include <VisorVR/UserAction.hpp>
#include <VisorVR/ViewsSettings.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/inttypes.hpp>
#include <VisorVR/scope_exit.hpp>

#include <winrt/Windows.ApplicationModel.DataTransfer.h>

#include <microsoft.ui.xaml.window.h>

#include <cmath>
#include <ranges>
#include <regex>
#include <utility>

#include <shobjidl.h>

using namespace VisorVR;
using namespace winrt::Microsoft::UI::Xaml::Controls;

using WindowSpec = WindowCaptureTab::WindowSpecification;

namespace winrt::VisorVRApp::implementation {

VisorVRApp::TabUIData TabsSettingsPage::CreateTabUIData(
  const std::shared_ptr<ITab>& tab) {
  VisorVRApp::TabUIData tabData {nullptr};
  if (std::dynamic_pointer_cast<BrowserTab>(tab)) {
    tabData = winrt::make<BrowserTabUIData>();
  } else if (std::dynamic_pointer_cast<DCSRadioLogTab>(tab)) {
    tabData = winrt::make<DCSRadioLogTabUIData>();
  } else if (std::dynamic_pointer_cast<WindowCaptureTab>(tab)) {
    tabData = winrt::make<WindowCaptureTabUIData>();
  } else {
    tabData = winrt::make<TabUIData>();
  }
  tabData.InstanceID(tab->GetRuntimeID().GetTemporaryValue());
  return tabData;
}

TabsSettingsPage::TabsSettingsPage() {
  InitializeComponent();
  mDXR.copy_from(gDXResources);
  mKneeboard = gKneeboard.lock();

  AddEventListener(
    mKneeboard->GetTabsList()->evTabsChangedEvent,
    {
      get_weak(),
      [](auto self) {
        if (self->mUIIsChangingTabs) {
          return;
        }
        if (self->mPropertyChangedEvent) {
          self->mPropertyChangedEvent(*self, PropertyChangedEventArgs(L"Tabs"));
        }
      },
    });

  CreateAddTabMenu(AddTabTopButton(), FlyoutPlacementMode::Bottom);
  CreateAddTabMenu(AddTabBottomButton(), FlyoutPlacementMode::Top);
}

IVector<IInspectable> TabsSettingsPage::Tabs() noexcept {
  const std::shared_lock kbLock(*mKneeboard);

  auto tabs = winrt::single_threaded_observable_vector<IInspectable>();
  for (const auto& tab: mKneeboard->GetTabsList()->GetTabs()) {
    tabs.Append(CreateTabUIData(tab));
  }
  tabs.VectorChanged({this, &TabsSettingsPage::OnTabsChanged});
  return tabs;
}

TabsSettingsPage::~TabsSettingsPage() { this->RemoveAllEventListeners(); }

void TabsSettingsPage::CreateAddTabMenu(
  const Button& button,
  FlyoutPlacementMode placement) {
  MenuFlyout flyout;
  auto tabTypes = flyout.Items();
#define IT(label, name) \
  { \
    MenuFlyoutItem item; \
    item.Text(to_hstring(label)); \
    item.Tag(box_value<uint64_t>(TABTYPE_IDX_##name)); \
    item.Click({this, &TabsSettingsPage::CreateTab}); \
    auto glyph = name##Tab::GetStaticGlyph(); \
    if (!glyph.empty()) { \
      FontIcon fontIcon; \
      fontIcon.Glyph(winrt::to_hstring(glyph)); \
      item.Icon(fontIcon); \
    } \
    tabTypes.Append(item); \
  }
  VISORVR_TAB_TYPES
#undef IT
  const auto pluginTabTypes = mKneeboard->GetPluginStore()->GetTabTypes();
  if (!pluginTabTypes.empty()) {
    tabTypes.Append(MenuFlyoutSeparator {});
    for (const auto& it: pluginTabTypes) {
      MenuFlyoutItem item;
      item.Text(to_hstring(it.mName));
      item.Tag(box_value(to_hstring(it.mID)));
      item.Click({this, &TabsSettingsPage::CreatePluginTab});

      FontIcon fontIcon;
      fontIcon.Glyph(L"\uea86");// puzzle
      item.Icon(fontIcon);

      tabTypes.Append(item);
    }
  }

  flyout.Placement(placement);
  button.Flyout(flyout);
}

VisorVR::fire_and_forget TabsSettingsPage::RestoreDefaults(
  IInspectable,
  RoutedEventArgs) noexcept {
  ContentDialog dialog;
  dialog.XamlRoot(this->XamlRoot());
  dialog.Title(box_value(to_hstring(_("Restore defaults?"))));
  dialog.Content(box_value(to_hstring(
    _("Do you want to restore the default tabs list, "
      "removing your preferences?"))));
  dialog.PrimaryButtonText(to_hstring(_("Restore Defaults")));
  dialog.CloseButtonText(to_hstring(_("Cancel")));
  dialog.DefaultButton(ContentDialogButton::Close);

  if (co_await dialog.ShowAsync() != ContentDialogResult::Primary) {
    co_return;
  }

  co_await mKneeboard->ResetTabsSettings();
}

static std::shared_ptr<ITab> find_tab(const IInspectable& sender) {
  auto kneeboard = gKneeboard.lock();

  const auto tabID = ITab::RuntimeID::FromTemporaryValue(
    unbox_value<uint64_t>(sender.as<Button>().Tag()));
  auto tabsList = kneeboard->GetTabsList();
  const auto tabs = tabsList->GetTabs();
  auto it = std::ranges::find(tabs, tabID, &ITab::GetRuntimeID);
  if (it == tabs.end()) {
    return {nullptr};
  }
  return *it;
}

void TabsSettingsPage::CopyDebugInfo(
  const IInspectable& sender,
  const RoutedEventArgs&) {
  const auto text = unbox_value<hstring>(sender.as<FrameworkElement>().Tag());
  Windows::ApplicationModel::DataTransfer::DataPackage package;
  package.SetText(text);
  Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(package);
}

VisorVR::fire_and_forget TabsSettingsPage::ShowDebugInfo(
  IInspectable sender,
  RoutedEventArgs) {
  const std::shared_lock lock(*mKneeboard);
  const auto tab = find_tab(sender);
  if (!tab) {
    co_return;
  }

  const auto debug = std::dynamic_pointer_cast<IHasDebugInformation>(tab);
  if (!debug) {
    co_return;
  }

  const auto info = debug->GetDebugInformation();
  if (info.empty()) {
    co_return;
  }

  DebugInfoText().Text(to_hstring(info));

  DebugInfoDialog().Title(
    winrt::box_value(
      to_hstring(std::format("'{}' - Debug Information", tab->GetTitle()))));
  CopyDebugInfoButton().Tag(winrt::box_value(to_hstring(info)));
  co_await DebugInfoDialog().ShowAsync();
}

VisorVR::fire_and_forget TabsSettingsPage::ShowTabSettings(
  IInspectable sender,
  RoutedEventArgs) {
  // Only hold the lock long enough to resolve the tab. It must NOT be held
  // across `ShowAsync()` below: while the dialog is open, its controls (e.g.
  // the "Placement in VR" section) write settings via SetViewsSettings, which
  // needs an exclusive lock. Holding a shared lock here would deadlock that.
  std::shared_ptr<ITab> tab;
  {
    const std::shared_lock lock(*mKneeboard);
    tab = find_tab(sender);
  }
  if (!tab) {
    co_return;
  }

  std::string title;

#define IT(label, kind) \
  if (std::dynamic_pointer_cast<kind##Tab>(tab)) { \
    title = label; \
  }
  VISORVR_TAB_TYPES
#undef IT
  if (title.empty()) {
    // We reach this for plugin tabs
    title = _("Tab Settings");
  } else {
    auto idx = title.find(" (");
    if (idx != std::string::npos) {
      title.erase(idx, std::string::npos);
    }
    title = std::format(_("{} Tab Settings"), title);
  }
  TabSettingsDialog().Title(box_value(to_hstring(title)));

  auto uiData = CreateTabUIData(tab);
  TabSettingsDialogContent().Content(uiData);
  TabSettingsDialogContent().ContentTemplateSelector(
    TabSettingsTemplateSelector());
  co_await TabSettingsDialog().ShowAsync();
  // Without this, crash if you:
  //
  // 1. Open tab settings
  // 2. Change a setting
  // 3. Switch profile
  // 4. Open tab settings for the same kind of tab
  //
  // Given that nulling out the content isn't enough, it seems like
  // there's unsafe caching in `ContentPresenter`: the template is fine
  // to re-use, but it shouldn't be re-using the old data
  TabSettingsDialogContent().Content({nullptr});
  TabSettingsDialogContent().ContentTemplateSelector({nullptr});
}

VisorVR::fire_and_forget TabsSettingsPage::ToggleVREditMode(
  IInspectable,
  RoutedEventArgs) {
  co_await mKneeboard->PostUserAction(UserAction::TOGGLE_VR_EDIT_MODE);
}

VisorVR::fire_and_forget TabsSettingsPage::RecenterVR(
  IInspectable,
  RoutedEventArgs) {
  co_await mKneeboard->PostUserAction(UserAction::RECENTER_VR);
}

VisorVR::fire_and_forget TabsSettingsPage::GoToInputBindings(
  IInspectable,
  RoutedEventArgs) {
  co_await LaunchURI(SpecialURIs::SettingsInput());
}

VisorVR::fire_and_forget TabsSettingsPage::RemoveTab(
  IInspectable sender,
  RoutedEventArgs) {
  const std::shared_lock lock(*mKneeboard);

  const auto tab = find_tab(sender);
  if (!tab) {
    co_return;
  }

  ContentDialog dialog;
  dialog.XamlRoot(this->XamlRoot());
  dialog.Title(
    box_value(to_hstring(std::format(_("Remove {}?"), tab->GetTitle()))));
  dialog.Content(box_value(to_hstring(
    std::format(_("Do you want to remove the '{}' tab?"), tab->GetTitle()))));

  dialog.PrimaryButtonText(to_hstring(_("Yes")));
  dialog.CloseButtonText(to_hstring(_("No")));
  dialog.DefaultButton(ContentDialogButton::Primary);

  auto result = co_await dialog.ShowAsync();
  if (result != ContentDialogResult::Primary) {
    co_return;
  }

  mUIIsChangingTabs = true;
  const scope_exit changeComplete {[&]() { mUIIsChangingTabs = false; }};

  auto tabsList = mKneeboard->GetTabsList();
  const auto tabs = tabsList->GetTabs();
  auto it = std::ranges::find(tabs, tab);
  auto idx = static_cast<VisorVR::TabIndex>(it - tabs.begin());
  co_await tabsList->RemoveTab(idx);
  List()
    .ItemsSource()
    .as<Windows::Foundation::Collections::IVector<IInspectable>>()
    .RemoveAt(idx);
}

VisorVR::fire_and_forget TabsSettingsPage::CreatePluginTab(
  IInspectable sender,
  RoutedEventArgs) noexcept {
  const auto id =
    to_string(unbox_value<winrt::hstring>(sender.as<MenuFlyoutItem>().Tag()));
  const auto tabTypes = mKneeboard->GetPluginStore()->GetTabTypes();
  const auto it = std::ranges::find(tabTypes, id, &Plugin::TabType::mID);
  if (it == tabTypes.end()) {
    VISORVR_BREAK;
    co_return;
  }
  const auto tab = co_await PluginTab::Create(
    mDXR,
    mKneeboard.get(),
    random_guid(),
    it->mName,
    PluginTab::Settings {.mPluginTabTypeID = id});
  co_await this->AddTabs({tab});
  co_return;
}

// If inline, MSVC errors that the constraints are not met, even though it's
// in an `if constexpr(constraints)` block.
//
// MSVC is generally smarter, seems to be confused by coroutines.
template <class T>
task<std::shared_ptr<T>> make_tab_without_making_msvc_sad(
  const audited_ptr<DXResources>& dxr,
  KneeboardState* kbs) {
  static_assert(
    ::VisorVR::detail::
      shared_constructible_from<T, audited_ptr<DXResources>, KneeboardState*>);
  return ::VisorVR::detail::make_shared<T>(dxr, kbs);
}

VisorVR::fire_and_forget TabsSettingsPage::CreateTab(
  IInspectable sender,
  RoutedEventArgs) noexcept {
  auto tabType = static_cast<TabType>(
    unbox_value<uint64_t>(sender.as<MenuFlyoutItem>().Tag()));

  switch (tabType) {
#define IT(_label, prefix) \
  case TabType::##prefix: \
    dprint("Adding " #prefix " tab"); \
    break;
    VISORVR_TAB_TYPES
#undef IT
    default:
      dprint("Adding tab with kind {}", std::to_underlying(tabType));
  }

  switch (tabType) {
    case TabType::Folder:
      CreateFolderTab();
      co_return;
    case TabType::SingleFile:
      CreateFileTab<SingleFileTab>();
      co_return;
    case TabType::EndlessNotebook:
      CreateFileTab<EndlessNotebookTab>(_("Open Template"));
      co_return;
    case TabType::WindowCapture:
      CreateWindowCaptureTab();
      co_return;
    case TabType::Browser:
      CreateBrowserTab();
      co_return;
  }
#define IT(_, type) \
  if (tabType == TabType::type) { \
    if constexpr (::VisorVR::detail::shared_constructible_from< \
                    type##Tab, \
                    audited_ptr<DXResources>, \
                    KneeboardState*>) { \
      auto tab = co_await make_tab_without_making_msvc_sad<type##Tab>( \
        mDXR, mKneeboard.get()); \
      co_await AddTabs({std::static_pointer_cast<ITab>(tab)}); \
      co_return; \
    } else { \
      throw std::logic_error( \
        std::format("Don't know how to construct {}Tab", #type)); \
    } \
  }
  VISORVR_TAB_TYPES
#undef IT
  throw std::logic_error(
    std::format("Unhandled tab type: {}", static_cast<uint8_t>(tabType)));
}

VisorVR::fire_and_forget TabsSettingsPage::CreateBrowserTab() {
  AddBrowserAddress().Text({});

  if (co_await AddBrowserDialog().ShowAsync() != ContentDialogResult::Primary) {
    co_return;
  }

  BrowserTab::Settings settings;
  auto uri = to_string(AddBrowserAddress().Text());
  if (uri.find("://") == std::string::npos) {
    uri = "https://" + uri;
  }
  settings.mURI = uri;

  co_await this->AddTabs({co_await BrowserTab::Create(
    mDXR, mKneeboard.get(), random_guid(), {}, settings)});
}

void TabsSettingsPage::OnAddBrowserAddressTextChanged(
  const IInspectable&,
  const IInspectable&) noexcept {
  auto str = to_string(AddBrowserAddress().Text());
  auto valid = !str.empty();
  if (valid && str.find("://") == std::string::npos) {
    str = "https://" + str;
  }
  if (valid) {
    try {
      winrt::Windows::Foundation::Uri uri {to_hstring(str)};
      const auto foldedScheme = fold_utf8(to_string(uri.SchemeName()));

      valid = (foldedScheme == "https") || (foldedScheme == "http")
        || (foldedScheme == "file") || (foldedScheme == "chrome");

    } catch (...) {
      valid = false;
    }
  }
  AddBrowserDialog().IsPrimaryButtonEnabled(valid);
}

VisorVR::fire_and_forget TabsSettingsPage::CreateWindowCaptureTab() {
  VisorVRApp::WindowPickerDialog picker;
  picker.XamlRoot(this->XamlRoot());

  if (co_await picker.ShowAsync() != ContentDialogResult::Primary) {
    co_return;
  }

  const auto hwnd = reinterpret_cast<HWND>(picker.Hwnd());
  if (!hwnd) {
    co_return;
  }

  const auto windowSpec = WindowCaptureTab::GetWindowSpecification(hwnd);
  if (!windowSpec) {
    co_return;
  }

  WindowCaptureTab::MatchSpecification matchSpec {*windowSpec};

  // WPF and WindowsForms apps do not use window classes correctly
  if (
    windowSpec->mWindowClass.starts_with("HwndWrapper[")
    || windowSpec->mWindowClass.starts_with("WindowsForms")) {
    matchSpec.mMatchWindowClass = false;
    matchSpec.mMatchTitle =
      WindowCaptureTab::MatchSpecification::TitleMatchKind::Exact;
  }

  const std::unordered_set<std::string_view> alwaysMatchWindowTitle {
    // These are all `Chrome_WidgetWin_1` window classes - but matching title
    // isn't correct for *all* electron apps, e.g. title should not be matched
    // for Discord
    "RacelabApps.exe",
    // All "MainWindow"
    "iOverlay.exe",
  };

  // These are all `Chrome_WidgetWin_1` window classes - but matching title
  // isn't correct for *all* electron apps, e.g. title should not be matched
  // for Discord
  if (alwaysMatchWindowTitle.contains(
        windowSpec->mExecutableLastSeenPath.filename().string())) {
    matchSpec.mMatchTitle =
      WindowCaptureTab::MatchSpecification::TitleMatchKind::Exact;
  }
  // Electron apps tend to have versioned installation directories; detect,
  // and use a wildcard for the version
  if (windowSpec->mWindowClass == "Chrome_WidgetWin_1") {
    // \\\\sigh.
    matchSpec.mExecutablePathPattern = std::regex_replace(
      matchSpec.mExecutablePathPattern,
      std::regex {"\\\\app-\\d+\\.\\d+\\.\\d+\\\\([^/]+\\.exe)$"},
      "\\\\app-*\\\\\\1",
      std::regex_constants::format_sed);
  }

  co_await this->AddTabs(
    {WindowCaptureTab::Create(mDXR, mKneeboard.get(), matchSpec)});
}

winrt::guid TabsSettingsPage::GetFilePickerPersistenceGuid() {
  return mKneeboard->GetProfileSettings().GetActiveProfile().mGuid;
}

template <class T>
VisorVR::fire_and_forget TabsSettingsPage::CreateFileTab(
  const std::string& pickerDialogTitle) {
  FilePicker picker(gMainWindow);
  picker.SettingsIdentifier(GetFilePickerPersistenceGuid());
  picker.SuggestedStartLocation(FOLDERID_Documents);
  std::vector<std::wstring> extensions;
  for (const auto& utf8: FilePageSource::GetSupportedExtensions(mDXR)) {
    extensions.push_back(std::wstring {to_hstring(utf8)});
  }
  picker.AppendFileType(L"Supported files", extensions);
  for (const auto& extension: extensions) {
    picker.AppendFileType(std::format(L"{} files", extension), {extension});
  }

  if (!pickerDialogTitle.empty()) {
    std::wstring utf16(to_hstring(pickerDialogTitle));
    picker.SetTitle(utf16);
  }

  auto files = picker.PickMultipleFiles();
  if (files.empty()) {
    co_return;
  }

  std::vector<std::shared_ptr<VisorVR::ITab>> newTabs;
  for (const auto& path: files) {
    // TODO (after v1.4): figure out MSVC compile errors if I move
    // `detail::make_shared()` in TabTypes.h out of the `detail`
    // sub-namespace
    newTabs.push_back(
      co_await detail::make_shared<T>(mDXR, mKneeboard.get(), path));
  }

  co_await this->AddTabs(newTabs);
}

VisorVR::fire_and_forget TabsSettingsPage::CreateFolderTab() {
  FilePicker picker(gMainWindow);
  picker.SettingsIdentifier(GetFilePickerPersistenceGuid());
  picker.SuggestedStartLocation(FOLDERID_Documents);

  auto folder = picker.PickSingleFolder();
  if (!folder) {
    co_return;
  }

  co_await this->AddTabs(
    {co_await FolderTab::Create(mDXR, mKneeboard.get(), *folder)});
}

task<void> TabsSettingsPage::AddTabs(
  const std::vector<std::shared_ptr<ITab>>& tabs) {
  const std::shared_lock lock(*mKneeboard);

  mUIIsChangingTabs = true;
  const scope_exit changeComplete {[&]() { mUIIsChangingTabs = false; }};

  auto initialIndex = List().SelectedIndex();
  if (initialIndex == -1) {
    initialIndex = 0;
  }

  auto tabsList = mKneeboard->GetTabsList();

  auto idx = initialIndex;
  auto allTabs = tabsList->GetTabs();
  for (const auto& tab: tabs) {
    allTabs.insert(allTabs.begin() + idx++, tab);
  }
  co_await tabsList->SetTabs(allTabs);

  idx = initialIndex;
  auto items = List()
                 .ItemsSource()
                 .as<Windows::Foundation::Collections::IVector<IInspectable>>();
  for (const auto& tab: tabs) {
    items.InsertAt(idx++, CreateTabUIData(tab));
  }
}

VisorVR::fire_and_forget TabsSettingsPage::OnTabsChanged(
  IInspectable,
  Windows::Foundation::Collections::IVectorChangedEventArgs) noexcept {
  const std::shared_lock lock(*mKneeboard);
  // For add/remove, the kneeboard state is updated first, but for reorder,
  // the ListView is the source of truth.
  //
  // Reorders are two-step: a remove and an insert
  auto items = List()
                 .ItemsSource()
                 .as<Windows::Foundation::Collections::IVector<IInspectable>>();
  auto tabsList = mKneeboard->GetTabsList();
  auto tabs = tabsList->GetTabs();
  if (items.Size() != tabs.size()) {
    // ignore the deletion ...
    co_return;
  }
  // ... but act on the insert :)
  mUIIsChangingTabs = true;
  const scope_exit changeComplete {[&]() { mUIIsChangingTabs = false; }};

  std::vector<std::shared_ptr<ITab>> reorderedTabs;
  for (auto item: items) {
    auto id =
      ITab::RuntimeID::FromTemporaryValue(item.as<TabUIData>()->InstanceID());
    auto it = std::ranges::find(tabs, id, &ITab::GetRuntimeID);
    if (it == tabs.end()) {
      continue;
    }
    reorderedTabs.push_back(*it);
  }
  co_await tabsList->SetTabs(reorderedTabs);
}

TabUIData::~TabUIData() { this->RemoveAllEventListeners(); }

hstring TabUIData::Title() const {
  const auto tab = mTab.lock();
  if (!tab) {
    return {};
  }
  return to_hstring(tab->GetTitle());
}

void TabUIData::Title(hstring title) {
  auto tab = mTab.lock();
  if (!tab) {
    return;
  }
  tab->SetTitle(to_string(title));
}

bool TabUIData::HasDebugInformation() const {
  const auto tab = mTab.lock();
  return tab && std::dynamic_pointer_cast<IHasDebugInformation>(tab);
}

hstring TabUIData::DebugInformation() const {
  const auto tab = mTab.lock();
  if (!tab) {
    return {};
  }
  const auto hdi = std::dynamic_pointer_cast<IHasDebugInformation>(tab);
  if (!hdi) {
    return {};
  }
  return winrt::to_hstring(hdi->GetDebugInformation());
}

std::optional<winrt::guid> TabUIData::GetVRViewID() const {
  const auto tab = mTab.lock();
  if (!tab) {
    return {};
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    return {};
  }
  const auto tabID = tab->GetPersistentID();
  const auto views = kneeboard->GetViewsSettings().mViews;
  for (const auto& view: views) {
    if (
      view.mDefaultTabID == tabID
      && view.mVR.GetType() == ViewVRSettings::Type::Independent) {
      return view.mGuid;
    }
  }
  return {};
}

bool TabUIData::HasVRPlacement() const {
  return GetVRViewID().has_value();
}

bool TabUIData::IsVREnabled() const {
  const auto viewID = GetVRViewID();
  if (!viewID) {
    return false;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    return false;
  }
  const auto views = kneeboard->GetViewsSettings().mViews;
  const auto it = std::ranges::find(views, *viewID, &ViewSettings::mGuid);
  return it != views.end() && it->mVR.mEnabled;
}

VisorVR::fire_and_forget TabUIData::IsVREnabled(bool value) {
  const auto viewID = GetVRViewID();
  if (!viewID) {
    co_return;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    co_return;
  }
  auto settings = kneeboard->GetViewsSettings();
  auto it = std::ranges::find(settings.mViews, *viewID, &ViewSettings::mGuid);
  if (it == settings.mViews.end() || it->mVR.mEnabled == value) {
    co_return;
  }
  it->mVR.mEnabled = value;
  // Do not touch `this` after the co_await: the save can outlive this object.
  co_await kneeboard->SetViewsSettings(settings);
}

float TabUIData::VRWidth() const {
  const auto viewID = GetVRViewID();
  if (!viewID) {
    return 0.0f;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    return 0.0f;
  }
  const auto views = kneeboard->GetViewsSettings().mViews;
  const auto it = std::ranges::find(views, *viewID, &ViewSettings::mGuid);
  if (it == views.end()) {
    return 0.0f;
  }
  return it->mVR.GetIndependentSettings().mMaximumPhysicalSize.mWidth;
}

VisorVR::fire_and_forget TabUIData::VRWidth(float value) {
  if (std::isnan(value)) {
    co_return;
  }
  const auto viewID = GetVRViewID();
  if (!viewID) {
    co_return;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    co_return;
  }
  auto settings = kneeboard->GetViewsSettings();
  auto it = std::ranges::find(settings.mViews, *viewID, &ViewSettings::mGuid);
  if (it == settings.mViews.end()) {
    co_return;
  }
  auto vr = it->mVR.GetIndependentSettings();
  if (vr.mMaximumPhysicalSize.mWidth == value) {
    co_return;
  }
  vr.mMaximumPhysicalSize.mWidth = value;
  it->mVR.SetIndependentSettings(vr);
  // Do not touch `this` after the co_await: the save can outlive this object.
  co_await kneeboard->SetViewsSettings(settings);
}

float TabUIData::VRHeight() const {
  const auto viewID = GetVRViewID();
  if (!viewID) {
    return 0.0f;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    return 0.0f;
  }
  const auto views = kneeboard->GetViewsSettings().mViews;
  const auto it = std::ranges::find(views, *viewID, &ViewSettings::mGuid);
  if (it == views.end()) {
    return 0.0f;
  }
  return it->mVR.GetIndependentSettings().mMaximumPhysicalSize.mHeight;
}

VisorVR::fire_and_forget TabUIData::VRHeight(float value) {
  if (std::isnan(value)) {
    co_return;
  }
  const auto viewID = GetVRViewID();
  if (!viewID) {
    co_return;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    co_return;
  }
  auto settings = kneeboard->GetViewsSettings();
  auto it = std::ranges::find(settings.mViews, *viewID, &ViewSettings::mGuid);
  if (it == settings.mViews.end()) {
    co_return;
  }
  auto vr = it->mVR.GetIndependentSettings();
  if (vr.mMaximumPhysicalSize.mHeight == value) {
    co_return;
  }
  vr.mMaximumPhysicalSize.mHeight = value;
  it->mVR.SetIndependentSettings(vr);
  // Do not touch `this` after the co_await: the save can outlive this object.
  co_await kneeboard->SetViewsSettings(settings);
}

float TabUIData::VRDistance() const {
  const auto viewID = GetVRViewID();
  if (!viewID) {
    return 0.0f;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    return 0.0f;
  }
  const auto views = kneeboard->GetViewsSettings().mViews;
  const auto it = std::ranges::find(views, *viewID, &ViewSettings::mGuid);
  if (it == views.end()) {
    return 0.0f;
  }
  // -Z is forwards in the world; users expect a positive "distance".
  return -it->mVR.GetIndependentSettings().mPose.mZ;
}

VisorVR::fire_and_forget TabUIData::VRDistance(float value) {
  if (std::isnan(value)) {
    co_return;
  }
  const auto viewID = GetVRViewID();
  if (!viewID) {
    co_return;
  }
  const auto kneeboard = gKneeboard.lock();
  if (!kneeboard) {
    co_return;
  }
  auto settings = kneeboard->GetViewsSettings();
  auto it = std::ranges::find(settings.mViews, *viewID, &ViewSettings::mGuid);
  if (it == settings.mViews.end()) {
    co_return;
  }
  auto vr = it->mVR.GetIndependentSettings();
  if (vr.mPose.mZ == -value) {
    co_return;
  }
  vr.mPose.mZ = -value;
  it->mVR.SetIndependentSettings(vr);
  // Do not touch `this` after the co_await: the save can outlive this object.
  co_await kneeboard->SetViewsSettings(settings);
}

// Icon choices offered by the picker. Index 0 in the UI means "use the
// tab type's own glyph", so these are offset by one.
static const std::vector<std::string>& IconPalette() {
  static const std::vector<std::string> sPalette {
    "\ueb41",// web dashboard
    "\ue7f4",// window
    "\ue909",// map
    "\uf12e",// list
    "\ue95d",// briefing
    "\ue709",// aircraft
    "\uf0e3",// mission
    "\ue838",// folder
    "\uea90",// file
    "\ue8ed",// repeat
    "\ue713",// settings
  };
  return sPalette;
}

int32_t TabUIData::IconIndex() const {
  const auto tab = mTab.lock();
  if (!tab) {
    return 0;
  }
  const auto icon = tab->GetIcon();
  if (icon == tab->GetGlyph()) {
    return 0;
  }
  const auto& palette = IconPalette();
  const auto it = std::ranges::find(palette, icon);
  if (it == palette.end()) {
    return 0;
  }
  return static_cast<int32_t>(it - palette.begin()) + 1;
}

void TabUIData::IconIndex(int32_t value) {
  const auto tab = mTab.lock();
  if (!tab) {
    return;
  }
  const auto& palette = IconPalette();
  if (value <= 0 || value > static_cast<int32_t>(palette.size())) {
    tab->SetIcon({});
  } else {
    tab->SetIcon(palette.at(value - 1));
  }
  mPropertyChangedEvent(*this, PropertyChangedEventArgs(L"IconIndex"));
}

uint64_t TabUIData::InstanceID() const {
  const auto tab = mTab.lock();
  if (!tab) {
    return {};
  }
  return tab->GetRuntimeID().GetTemporaryValue();
}

void TabUIData::InstanceID(uint64_t value) {
  auto kneeboard = gKneeboard.lock();
  const std::shared_lock lock(*kneeboard);

  this->RemoveAllEventListeners();
  mTab = {};

  const auto tabs = kneeboard->GetTabsList()->GetTabs();
  const auto it = std::ranges::find_if(
    tabs, [value](const auto& tab) { return tab->GetRuntimeID() == value; });
  if (it == tabs.end()) {
    return;
  }

  const auto tab = *it;
  mTab = tab;

  AddEventListener(tab->evSettingsChangedEvent, [weakThis = get_weak()]() {
    auto strongThis = weakThis.get();
    if (!strongThis) {
      return;
    }
    strongThis->mPropertyChangedEvent(
      *strongThis, PropertyChangedEventArgs(L"Title"));
  });

  const auto hdi = std::dynamic_pointer_cast<IHasDebugInformation>(tab);
  if (!hdi) {
    return;
  }

  AddEventListener(
    hdi->evDebugInformationHasChanged, [weakThis = get_weak()]() {
      auto self = weakThis.get();
      if (!self) {
        return;
      }
      self->mPropertyChangedEvent(
        *self, PropertyChangedEventArgs(L"DebugInformation"));
    });
}

bool BrowserTabUIData::IsSimHubIntegrationEnabled() const {
  return GetTab()->IsSimHubIntegrationEnabled();
}

VisorVR::fire_and_forget BrowserTabUIData::IsSimHubIntegrationEnabled(
  bool value) {
  co_await GetTab()->SetSimHubIntegrationEnabled(value);
}

bool BrowserTabUIData::AreOpenKneeboardAPIsEnabled() const {
  return GetTab()->AreOpenKneeboardAPIsEnabled();
}

VisorVR::fire_and_forget BrowserTabUIData::AreOpenKneeboardAPIsEnabled(
  const bool value) {
  if (value == GetTab()->AreOpenKneeboardAPIsEnabled()) {
    co_return;
  }
  const auto weak = get_weak();
  co_await GetTab()->SetOpenKneeboardAPIsEnabled(value);
  if (const auto self = weak.get(); self && self->mPropertyChangedEvent) {
    self->mPropertyChangedEvent(
      *self, PropertyChangedEventArgs(L"AreOpenKneeboardAPIsEnabled"));
  }
}

bool BrowserTabUIData::IsBackgroundTransparent() const {
  return GetTab()->IsBackgroundTransparent();
}

VisorVR::fire_and_forget BrowserTabUIData::IsBackgroundTransparent(
  bool value) {
  co_await GetTab()->SetBackgroundTransparent(value);
}

double BrowserTabUIData::RenderScalePercent() const {
  return GetTab()->GetRenderScale() * 100;
}

VisorVR::fire_and_forget BrowserTabUIData::RenderScalePercent(
  const double value) {
  if (std::isnan(value)) {
    co_return;
  }
  const auto scale = static_cast<float>(value / 100);
  if (scale == GetTab()->GetRenderScale()) {
    co_return;
  }
  const auto weak = get_weak();
  co_await GetTab()->SetRenderScale(scale);
  // The resolution caption is derived from the scale, so it has to be told.
  if (const auto self = weak.get(); self && self->mPropertyChangedEvent) {
    self->mPropertyChangedEvent(
      *self, PropertyChangedEventArgs(L"RenderResolutionDescription"));
  }
}

double BrowserTabUIData::MaxRenderScalePercent() const {
  return GetTab()->GetMaxRenderScale() * 100;
}

winrt::hstring BrowserTabUIData::RenderResolutionDescription() const {
  const auto size = GetTab()->GetRenderPixelSize();
  return winrt::hstring {std::format(
    L"Rendered at {} × {} pixels", size.mWidth, size.mHeight)};
}

std::shared_ptr<BrowserTab> BrowserTabUIData::GetTab() const {
  auto tab = mTab.lock();
  if (!tab) {
    return {};
  }

  auto refined = std::dynamic_pointer_cast<BrowserTab>(tab);
  if (!refined) [[unlikely]] {
    fatal("Expected a BrowserTab but didn't get one");
  }
  return refined;
}

std::shared_ptr<DCSRadioLogTab> DCSRadioLogTabUIData::GetTab() const {
  auto tab = mTab.lock();
  if (!tab) {
    return {};
  }
  auto refined = std::dynamic_pointer_cast<DCSRadioLogTab>(tab);
  if (!refined) [[unlikely]] {
    fatal("Expected a DCSRadioLogTab but didn't get one");
  }
  return refined;
}

uint8_t DCSRadioLogTabUIData::MissionStartBehavior() const {
  auto tab = this->GetTab();
  return tab ? std::to_underlying(tab->GetMissionStartBehavior()) : 0;
}

void DCSRadioLogTabUIData::MissionStartBehavior(uint8_t value) {
  auto tab = this->GetTab();
  if (!tab) {
    return;
  }
  tab->SetMissionStartBehavior(
    static_cast<DCSRadioLogTab::MissionStartBehavior>(value));
}

bool DCSRadioLogTabUIData::TimestampsEnabled() const {
  return this->GetTab()->GetTimestampsEnabled();
}

void DCSRadioLogTabUIData::TimestampsEnabled(bool value) {
  this->GetTab()->SetTimestampsEnabled(value);
}

winrt::hstring WindowCaptureTabUIData::WindowTitle() {
  return to_hstring(GetTab()->GetMatchSpecification().mTitle);
}

VisorVR::fire_and_forget WindowCaptureTabUIData::WindowTitle(
  const hstring& title) {
  auto spec = GetTab()->GetMatchSpecification();
  spec.mTitle = to_string(title);
  co_await GetTab()->SetMatchSpecification(spec);
}

bool WindowCaptureTabUIData::MatchWindowClass() {
  return GetTab()->GetMatchSpecification().mMatchWindowClass;
}

VisorVR::fire_and_forget WindowCaptureTabUIData::MatchWindowClass(
  bool value) {
  auto spec = GetTab()->GetMatchSpecification();
  spec.mMatchWindowClass = value;
  co_await GetTab()->SetMatchSpecification(spec);
}

uint8_t WindowCaptureTabUIData::MatchWindowTitle() {
  return static_cast<uint8_t>(GetTab()->GetMatchSpecification().mMatchTitle);
}

VisorVR::fire_and_forget WindowCaptureTabUIData::MatchWindowTitle(
  uint8_t value) {
  auto spec = GetTab()->GetMatchSpecification();
  spec.mMatchTitle =
    static_cast<WindowCaptureTab::MatchSpecification::TitleMatchKind>(value);
  co_await GetTab()->SetMatchSpecification(spec);
}

bool WindowCaptureTabUIData::IsInputEnabled() const {
  return GetTab()->IsInputEnabled();
}

void WindowCaptureTabUIData::IsInputEnabled(bool value) {
  GetTab()->SetIsInputEnabled(value);
}

bool WindowCaptureTabUIData::IsCursorCaptureEnabled() const {
  return GetTab()->IsCursorCaptureEnabled();
}

VisorVR::fire_and_forget WindowCaptureTabUIData::IsCursorCaptureEnabled(
  bool value) {
  co_await GetTab()->SetCursorCaptureEnabled(value);
}

bool WindowCaptureTabUIData::CaptureClientArea() const {
  return GetTab()->GetCaptureArea() == HWNDPageSource::CaptureArea::ClientArea;
}

VisorVR::fire_and_forget WindowCaptureTabUIData::CaptureClientArea(
  bool enabled) {
  co_await GetTab()->SetCaptureArea(
    enabled ? HWNDPageSource::CaptureArea::ClientArea
            : HWNDPageSource::CaptureArea::FullWindow);
}

hstring WindowCaptureTabUIData::ExecutablePathPattern() const {
  return to_hstring(GetTab()->GetMatchSpecification().mExecutablePathPattern);
}

VisorVR::fire_and_forget WindowCaptureTabUIData::ExecutablePathPattern(
  hstring pattern) {
  auto spec = GetTab()->GetMatchSpecification();
  spec.mExecutablePathPattern = to_string(pattern);
  co_await GetTab()->SetMatchSpecification(spec);
}

hstring WindowCaptureTabUIData::WindowClass() const {
  return to_hstring(GetTab()->GetMatchSpecification().mWindowClass);
}

VisorVR::fire_and_forget WindowCaptureTabUIData::WindowClass(
  hstring value) {
  auto spec = GetTab()->GetMatchSpecification();
  spec.mWindowClass = to_string(value);
  co_await GetTab()->SetMatchSpecification(spec);
}

std::shared_ptr<WindowCaptureTab> WindowCaptureTabUIData::GetTab() const {
  auto tab = mTab.lock();
  if (!tab) {
    return {};
  }
  auto refined = std::dynamic_pointer_cast<WindowCaptureTab>(tab);
  if (!refined) {
    dprint("Expected a WindowCaptureTab but didn't get one");
    VISORVR_BREAK;
  }
  return refined;
}

winrt::Microsoft::UI::Xaml::DataTemplate TabUIDataTemplateSelector::Generic() {
  return mGeneric;
}

void TabUIDataTemplateSelector::Generic(
  winrt::Microsoft::UI::Xaml::DataTemplate const& value) {
  mGeneric = value;
}

winrt::Microsoft::UI::Xaml::DataTemplate TabUIDataTemplateSelector::Browser() {
  return mBrowser;
}

void TabUIDataTemplateSelector::Browser(
  winrt::Microsoft::UI::Xaml::DataTemplate const& value) {
  mBrowser = value;
}

winrt::Microsoft::UI::Xaml::DataTemplate
TabUIDataTemplateSelector::DCSRadioLog() {
  return mDCSRadioLog;
}

void TabUIDataTemplateSelector::DCSRadioLog(
  winrt::Microsoft::UI::Xaml::DataTemplate const& value) {
  mDCSRadioLog = value;
}

winrt::Microsoft::UI::Xaml::DataTemplate
TabUIDataTemplateSelector::WindowCapture() {
  return mWindowCapture;
}

void TabUIDataTemplateSelector::WindowCapture(
  winrt::Microsoft::UI::Xaml::DataTemplate const& value) {
  mWindowCapture = value;
}

DataTemplate TabUIDataTemplateSelector::SelectTemplateCore(
  const IInspectable& item) {
  if (item.try_as<winrt::VisorVRApp::BrowserTabUIData>()) {
    return mBrowser;
  }
  if (item.try_as<winrt::VisorVRApp::DCSRadioLogTabUIData>()) {
    return mDCSRadioLog;
  }

  if (item.try_as<winrt::VisorVRApp::WindowCaptureTabUIData>()) {
    return mWindowCapture;
  }

  return mGeneric;
}

DataTemplate TabUIDataTemplateSelector::SelectTemplateCore(
  const IInspectable& item,
  const DependencyObject&) {
  return this->SelectTemplateCore(item);
}

}// namespace winrt::VisorVRApp::implementation
