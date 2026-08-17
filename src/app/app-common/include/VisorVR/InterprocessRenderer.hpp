// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/DXResources.hpp>
#include <VisorVR/Events.hpp>
#include <VisorVR/KneeboardState.hpp>
#include <VisorVR/KneeboardView.hpp>
#include <VisorVR/SHM.hpp>

#include <VisorVR/audited_ptr.hpp>
#include <VisorVR/config.hpp>
#include <VisorVR/final_release_deleter.hpp>

#include <shims/winrt/base.h>

#include <d3d11.h>

#include <memory>
#include <mutex>
#include <optional>

#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11_3.h>
#include <dwrite.h>

namespace VisorVR {
struct CursorEvent;
class CursorRenderer;
class ITab;
class ToolbarAction;
struct GameInstance;
struct DXResources;

// P1b: raw-input relative mouse capture used by the in-VR edit mode
// (defined in InterprocessRenderer.cpp). Replaces the old
// GetCursorPos/SetCursorPos re-centring, which jittered and left the
// physical cursor confined after edit mode was turned off.
class RawMouseCapture;

class InterprocessRenderer final
  : private EventReceiver,
    public std::enable_shared_from_this<InterprocessRenderer> {
 public:
  ~InterprocessRenderer() override;
  static VisorVR::fire_and_forget final_release(
    std::unique_ptr<InterprocessRenderer>);

  static std::shared_ptr<InterprocessRenderer> Create(
    const audited_ptr<DXResources>&,
    KneeboardState*);

  void PostUserAction(UserAction);
  [[nodiscard]] task<void> RenderNow() noexcept;

  [[nodiscard]]
  uint64_t GetFrameCountForMetricsOnly() const;

 private:
  InterprocessRenderer() = delete;
  InterprocessRenderer(const audited_ptr<DXResources>&);
  // Split out from the constructor so that `shared_from_this()` is valid
  // when invoked
  void Initialize(KneeboardState*);

  // If we replace the shared_ptr while a draw is in progress,
  // we need to delay things a little
  static std::mutex sSingleInstance;
  std::unique_lock<std::mutex> mInstanceLock;
  winrt::apartment_context mOwnerThread;

  audited_ptr<DXResources> mDXR;
  VisorVR::SHM::Writer mSHM;

  KneeboardState* mKneeboard = nullptr;

  std::atomic_flag mRendering;

  struct IPCTextureResources {
    winrt::com_ptr<ID3D11Texture2D> mTexture;
    winrt::com_ptr<ID3D11RenderTargetView> mRenderTargetView;

    winrt::handle mTextureHandle;
    PixelSize mTextureSize;

    winrt::com_ptr<ID3D11Fence> mFence;
    winrt::handle mFenceHandle;

    D3D11_VIEWPORT mViewport {};
  };

  std::array<IPCTextureResources, SHMSwapchainLength> mIPCSwapchain;

  IPCTextureResources* GetIPCTextureResources(
    uint8_t textureIndex,
    const PixelSize&);

  std::shared_ptr<RenderTargetWithMultipleIdentities> mCanvas;
  PixelSize mCanvasSize;

  void InitializeCanvas(const PixelSize&);

  task<SHM::LayerConfig> RenderLayer(
    const ViewRenderInfo&,
    const PixelRect& bounds) noexcept;

  void SubmitFrame(
    const std::vector<SHM::LayerConfig>&,
    uint64_t inputLayerID) noexcept;

  bool mVisible {true};
  bool mPreviousFrameWasVisible {false};

  // In-VR edit mode: raw-input mouse capture + accumulated -1..1 cursor.
  // Created on entering edit mode, destroyed on leaving it.
  std::unique_ptr<RawMouseCapture> mMouseCapture;
  float mEditCursorX {0.0f};
  float mEditCursorY {0.0f};
  float mEditScroll {0.0f};
  float mEditRotYaw {0.0f};
  float mEditRotPitch {0.0f};
  bool mEditGrabToggle {false};
  bool mPrevLButton {false};

  // Cached text format for the in-VR setup-mode control hints.
  winrt::com_ptr<IDWriteTextFormat> mEditHintFormat;
};

}// namespace VisorVR
