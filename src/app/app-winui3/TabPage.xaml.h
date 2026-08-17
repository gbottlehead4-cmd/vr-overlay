// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once
#include "TabPage.g.h"

#include <VisorVR/CursorEvent.hpp>
#include <VisorVR/DXResources.hpp>
#include <VisorVR/Events.hpp>
#include <VisorVR/Pixels.hpp>
#include <VisorVR/TabView.hpp>
#include <VisorVR/ThreadGuard.hpp>

#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include <d2d1.h>
#include <dxgi1_2.h>

namespace VisorVR {
class CursorRenderer;
class D2DErrorRenderer;
struct DXResources;
class KneeboardView;
class ISelectableToolbarItem;
class IToolbarFlyout;
class IToolbarItem;
class KneeboardState;
class RenderTarget;
class ToolbarAction;
class ToolbarToggleAction;
}// namespace VisorVR

using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Input;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Input;
using namespace winrt::Microsoft::UI::Xaml::Navigation;
using namespace VisorVR;

namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::VisorVRApp::implementation {
struct TabPage : TabPageT<TabPage>, EventReceiver {
  using TabView = ::VisorVR::TabView;

  TabPage();
  ~TabPage();

  static VisorVR::fire_and_forget final_release(std::unique_ptr<TabPage>);

  void OnNavigatedTo(const NavigationEventArgs&) noexcept;
  void OnCanvasSizeChanged(
    const IInspectable&,
    const SizeChangedEventArgs&) noexcept;
  void OnPointerEvent(const IInspectable&, const PointerEventArgs&) noexcept;
  VisorVR::fire_and_forget ToggleVREditMode(
    const IInspectable&,
    const RoutedEventArgs&);
  VisorVR::fire_and_forget RecenterVR(
    const IInspectable&,
    const RoutedEventArgs&);
  VisorVR::fire_and_forget GoToInputBindings(
    const IInspectable&,
    const RoutedEventArgs&);
  VisorVR::fire_and_forget GoToPanelsList(
    const IInspectable&,
    const RoutedEventArgs&);
  void ShowPreview(const IInspectable&, const RoutedEventArgs&);
  void ShowPanelSettings(const IInspectable&, const RoutedEventArgs&);

  task<void> PaintNow(
    std::source_location loc = std::source_location::current()) noexcept;

 private:
  bool mShuttingDown = false;
  winrt::apartment_context mUIThread;
  std::shared_ptr<TabView> mTabView;
  EventHandlerToken mTabViewRepaintToken;
  std::unique_ptr<CursorRenderer> mCursorRenderer;
  std::unique_ptr<D2DErrorRenderer> mErrorRenderer;
  D2D1_COLOR_F mBackgroundColor;
  winrt::com_ptr<ID2D1SolidColorBrush> mForegroundBrush;

  DispatcherQueueController mDQC {nullptr};
  InputPointerSource mInputPointerSource {nullptr};
  bool mDrawCursor = false;
  void InitializePointerSource();

  std::mutex mCursorEventsMutex;
  std::queue<CursorEvent> mCursorEvents;
  bool mHaveCursorEvents {false};
  void EnqueuePointerPoint(const PointerPoint&);
  void EnqueueCursorEvent(const CursorEvent&);
  void FlushCursorEvents();

  void SetTab(const std::shared_ptr<TabView>&);
  VisorVR::fire_and_forget UpdateToolbar();
  void SetSettingsVisible(bool);
  void UpdatePanelHeader();
  void InitializeSwapChain();
  void ResizeSwapChain();

  bool mNeedsFrame = true;
  void PaintLater();

  task<void> OnToolbarActionClick(std::shared_ptr<ToolbarAction>);

  float mCompositionScaleX {1.0f};
  float mCompositionScaleY {1.0f};
  PixelSize mPanelDimensions {};

  struct PageMetrics {
    PixelSize mNativeSize;
    PixelRect mRenderRect;
    PixelSize mRenderSize;
  };
  PageMetrics GetPageMetrics();

  std::vector<std::shared_ptr<IToolbarItem>> mToolbarItems;

  std::vector<EventHandlerToken> mKneeboardViewEvents;
  std::shared_ptr<KneeboardView> mKneeboardView;
  void UpdateKneeboardView();

  muxc::ICommandBarElement CreateCommandBarElement(
    const std::shared_ptr<IToolbarItem>&);
  muxc::AppBarButton CreateAppBarButtonBase(
    const std::shared_ptr<ISelectableToolbarItem>&);
  muxc::AppBarButton CreateAppBarButton(const std::shared_ptr<ToolbarAction>&);
  muxc::AppBarButton CreateAppBarFlyout(const std::shared_ptr<IToolbarFlyout>&);
  muxc::AppBarToggleButton CreateAppBarToggleButton(
    const std::shared_ptr<ToolbarToggleAction>&);

  muxc::MenuFlyoutItemBase CreateMenuFlyoutItem(
    const std::shared_ptr<IToolbarItem>&);

  void AttachVisibility(const std::shared_ptr<IToolbarItem>&, IInspectable);

  winrt::com_ptr<ID3D11Texture2D> mCanvas;
  PixelSize mSwapChainDimensions {};
  winrt::com_ptr<IDXGISwapChain1> mSwapChain;
  std::shared_ptr<RenderTarget> mRenderTarget;

  static std::unordered_map<TabView::RuntimeID, std::weak_ptr<RenderTarget>>
    sRenderTarget;
  static std::shared_ptr<RenderTarget> GetRenderTarget(
    const audited_ptr<DXResources>&,
    const TabView::RuntimeID&);

  audited_ptr<VisorVR::DXResources> mDXR;
  audited_ptr<VisorVR::KneeboardState> mKneeboard;

  ThreadGuard mThreadGuard;
};
}// namespace winrt::VisorVRApp::implementation
namespace winrt::VisorVRApp::factory_implementation {
struct TabPage : TabPageT<TabPage, implementation::TabPage> {};
}// namespace winrt::VisorVRApp::factory_implementation
