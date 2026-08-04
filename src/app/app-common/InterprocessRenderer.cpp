// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <OpenKneeboard/CreateTabActions.hpp>
#include <OpenKneeboard/CursorClickableRegions.hpp>
#include <OpenKneeboard/CursorEvent.hpp>
#include <OpenKneeboard/CursorRenderer.hpp>
#include <OpenKneeboard/D3D11.hpp>
#include <OpenKneeboard/GetSystemColor.hpp>
#include <OpenKneeboard/ITab.hpp>
#include <OpenKneeboard/InterprocessRenderer.hpp>
#include <OpenKneeboard/KneeboardState.hpp>
#include <OpenKneeboard/KneeboardView.hpp>
#include <OpenKneeboard/Spriting.hpp>
#include <OpenKneeboard/TabView.hpp>
#include <OpenKneeboard/ToolbarAction.hpp>

#include <OpenKneeboard/dprint.hpp>
#include <OpenKneeboard/hresult.hpp>
#include <OpenKneeboard/scope_exit.hpp>
#include <OpenKneeboard/tracing.hpp>

#include <atomic>
#include <mutex>
#include <ranges>
#include <stop_token>
#include <thread>

#include <DirectXColors.h>
#include <d3d11_4.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <hidusage.h>
#include <wincodec.h>

namespace OpenKneeboard {

// P1b: relative mouse capture for the in-VR edit mode.
//
// The previous approach read GetCursorPos and SetCursorPos(centre) every
// frame on the render thread. That fought itself (jitter around centre) and
// left the physical cursor stuck after edit mode ended. Instead we register
// a raw-input (WM_INPUT) sink on a hidden message-only window running on its
// own thread. Raw input reports true relative device deltas regardless of
// foreground focus (the game owns focus while racing) and regardless of the
// cursor hitting a screen edge, and never moves the physical cursor.
class RawMouseCapture final {
 public:
  RawMouseCapture() {
    mThread = std::jthread([this](std::stop_token st) { this->ThreadMain(st); });
  }

  ~RawMouseCapture() {
    mThread.request_stop();
    // std::jthread joins on destruction; the ThreadMain loop wakes at least
    // every 100ms to observe the stop request and tears down the raw-input
    // registration + window itself.
  }

  RawMouseCapture(const RawMouseCapture&) = delete;
  RawMouseCapture& operator=(const RawMouseCapture&) = delete;

  // Called from the render thread. Returns the relative device counts and
  // wheel delta accumulated since the last call, and resets the accumulators.
  void Fetch(float& dx, float& dy, float& wheel) noexcept {
    dx = static_cast<float>(mAccumX.exchange(0));
    dy = static_cast<float>(mAccumY.exchange(0));
    wheel = static_cast<float>(mAccumWheel.exchange(0)) / WHEEL_DELTA;
  }

 private:
  static constexpr wchar_t kClassName[] = L"OpenKneeboard_RawMouseCapture";

  std::atomic<int64_t> mAccumX {0};
  std::atomic<int64_t> mAccumY {0};
  std::atomic<int64_t> mAccumWheel {0};
  std::jthread mThread;

  void OnRawInput(HRAWINPUT handle) noexcept {
    RAWINPUT ri {};
    UINT size = sizeof(ri);
    if (
      GetRawInputData(
        handle, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER))
      == static_cast<UINT>(-1)) {
      return;
    }
    if (ri.header.dwType != RIM_TYPEMOUSE) {
      return;
    }
    // Ignore absolute-coordinate devices (e.g. some tablets / RDP); we only
    // integrate relative motion.
    if ((ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0) {
      return;
    }
    mAccumX.fetch_add(ri.data.mouse.lLastX, std::memory_order_relaxed);
    mAccumY.fetch_add(ri.data.mouse.lLastY, std::memory_order_relaxed);
    if ((ri.data.mouse.usButtonFlags & RI_MOUSE_WHEEL) != 0) {
      // usButtonData is a signed wheel delta (multiples of WHEEL_DELTA=120).
      mAccumWheel.fetch_add(
        static_cast<SHORT>(ri.data.mouse.usButtonData),
        std::memory_order_relaxed);
    }
  }

  static LRESULT CALLBACK
  WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
      auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
      SetWindowLongPtrW(
        hwnd,
        GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    } else if (msg == WM_INPUT) {
      auto self = reinterpret_cast<RawMouseCapture*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
      if (self) {
        self->OnRawInput(reinterpret_cast<HRAWINPUT>(lParam));
      }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }

  void ThreadMain(std::stop_token stop) noexcept {
    const auto hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &RawMouseCapture::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;
    // Ignore "already registered" if a previous capture registered it.
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
      0,
      kClassName,
      L"",
      0,
      0,
      0,
      0,
      0,
      HWND_MESSAGE,
      nullptr,
      hInstance,
      this);
    if (!hwnd) {
      return;
    }

    RAWINPUTDEVICE rid {};
    rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid.usUsage = HID_USAGE_GENERIC_MOUSE;
    // INPUTSINK: receive input even though the game, not us, has focus.
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    MSG msg {};
    while (!stop.stop_requested()) {
      // Wake for messages or every 100ms to re-check the stop token.
      MsgWaitForMultipleObjectsEx(
        0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
      while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        DispatchMessageW(&msg);
      }
    }

    rid.dwFlags = RIDEV_REMOVE;
    rid.hwndTarget = nullptr;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    DestroyWindow(hwnd);
  }
};

void InterprocessRenderer::SubmitFrame(
  const std::vector<SHM::LayerConfig>& shmLayers,
  uint64_t /*inputLayerID*/) noexcept {
  if (!mSHM) {
    return;
  }

  OPENKNEEBOARD_TraceLoggingScopedActivity(
    activity, "InterprocessRenderer::SubmitFrame()");

  auto ctx = mDXR->mD3D11ImmediateContext.get();
  const D3D11_BOX srcBox {
    0,
    0,
    0,
    static_cast<UINT>(mCanvasSize.mWidth),
    static_cast<UINT>(mCanvasSize.mHeight),
    1,
  };

  auto srcTexture = mCanvas->d3d().texture();

  TraceLoggingWriteTagged(activity, "AcquireSHMLock/start");
  const std::unique_lock shmLock(mSHM);
  TraceLoggingWriteTagged(activity, "AcquireSHMLock/stop");

  auto ipcTextureInfo = mSHM.BeginFrame();
  auto destResources =
    this->GetIPCTextureResources(ipcTextureInfo.mTextureIndex, mCanvasSize);

  auto fence = destResources->mFence.get();
  {
    OPENKNEEBOARD_TraceLoggingScope(
      "CopyFromCanvas",
      TraceLoggingValue(ipcTextureInfo.mTextureIndex, "TextureIndex"),
      TraceLoggingValue(ipcTextureInfo.mFenceOut, "FenceOut"));
    {
      OPENKNEEBOARD_TraceLoggingScope("CopyFromCanvas/CopySubresourceRegion");
      ctx->CopySubresourceRegion(
        destResources->mTexture.get(), 0, 0, 0, 0, srcTexture, 0, &srcBox);
    }
    {
      OPENKNEEBOARD_TraceLoggingScope("CopyFromCanvas/FenceOut");
      check_hresult(ctx->Signal(fence, ipcTextureInfo.mFenceOut));
    }
  }

  SHM::Config config {
    .mGlobalInputLayerID =
      mKneeboard->GetActiveInGameView()->GetRuntimeID().GetTemporaryValue(),
    .mVR = static_cast<const VRRenderSettings&>(mKneeboard->GetVRSettings()),
    .mTextureSize = destResources->mTextureSize,
    .mEditActive = mKneeboard->IsVREditMode(),
  };
  // P1b: when setup mode is on, feed the mouse into the edit channel using
  // raw-input relative deltas (RawMouseCapture). The physical cursor is never
  // moved, so there is no jitter and nothing to release when edit mode ends.
  // The OpenXR layer turns the accumulated -1..1 cursor into a VR cursor +
  // panel grab.
  if (config.mEditActive) {
    if (!mMouseCapture) {
      mMouseCapture = std::make_unique<RawMouseCapture>();
      // Start centred + un-grabbed each time edit mode is entered.
      mEditCursorX = 0.0f;
      mEditCursorY = 0.0f;
      mEditGrabToggle = false;
      mPrevLButton = false;
    }
    float dx = 0.0f;
    float dy = 0.0f;
    float wheel = 0.0f;
    mMouseCapture->Fetch(dx, dy, wheel);
    // Accumulated wheel notches (monotonic while editing); the OpenXR layer
    // diffs this per frame (resize when hovering, distance when grabbing).
    mEditScroll += wheel;

    // Grab is a TOGGLE: left-click grabs the hovered panel, click again drops
    // it. Nothing needs to be held while positioning/rotating/scaling.
    const bool lButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (lButton && !mPrevLButton) {
      mEditGrabToggle = !mEditGrabToggle;
    }
    mPrevLButton = lButton;
    const bool grab = mEditGrabToggle;
    const bool rotateMod = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (grab && rotateMod) {
      // Shift + drag while holding a panel: rotate it. Feed the mouse into the
      // rotation accumulators and FREEZE the pointer so the panel does not
      // orbit while you angle it. ~200 counts per radian.
      constexpr float rotSensitivity = 1.0f / 200.0f;
      mEditRotYaw += dx * rotSensitivity;
      mEditRotPitch += dy * rotSensitivity;
    } else {
      // The cursor is an angle offset in RADIANS that the OpenXR layer adds to
      // the facing direction captured when edit mode was entered. ~400 device
      // counts per radian (~57 deg). X (yaw) can sweep almost all the way
      // around; Y (pitch) is limited so you cannot point straight up/down.
      constexpr float sensitivity = 1.0f / 400.0f;
      constexpr float pi = 3.14159265f;
      mEditCursorX = std::clamp(mEditCursorX + dx * sensitivity, -pi, pi);
      mEditCursorY = std::clamp(mEditCursorY + dy * sensitivity, -1.45f, 1.45f);
    }
    config.mEditCursorX = mEditCursorX;
    config.mEditCursorY = mEditCursorY;
    config.mEditScroll = mEditScroll;
    config.mEditRotYaw = mEditRotYaw;
    config.mEditRotPitch = mEditRotPitch;
    config.mEditGrab = grab;
  } else if (mMouseCapture) {
    // Leaving edit mode: stop the capture thread + unregister raw input.
    mMouseCapture.reset();
  }
  const auto tint = mKneeboard->GetUISettings().mTint;
  if (tint.mEnabled) {
    config.mTint = {
      tint.mRed * tint.mBrightness,
      tint.mGreen * tint.mBrightness,
      tint.mBlue * tint.mBrightness,
      /* alpha = */ 1.0f,
    };
  }

  {
    OPENKNEEBOARD_TraceLoggingScope("SHMSubmitFrame");
    mSHM.SubmitFrame(
      ipcTextureInfo,
      config,
      shmLayers,
      destResources->mTextureHandle.get(),
      destResources->mFenceHandle.get());
  }
}

uint64_t InterprocessRenderer::GetFrameCountForMetricsOnly() const {
  return mSHM.GetFrameCountForMetricsOnly();
}

void InterprocessRenderer::InitializeCanvas(const PixelSize& size) {
  if (mCanvasSize == size) {
    return;
  }

  OPENKNEEBOARD_TraceLoggingScope("InterprocessRenderer::InitializeCanvas()");

  if (size.IsEmpty()) [[unlikely]] {
    OPENKNEEBOARD_BREAK;
    return;
  }

  D3D11_TEXTURE2D_DESC desc {
    .Width = static_cast<UINT>(size.mWidth),
    .Height = static_cast<UINT>(size.mHeight),
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = SHM::SHARED_TEXTURE_PIXEL_FORMAT,
    .SampleDesc = {1, 0},
    .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };

  auto device = mDXR->mD3D11Device.get();

  winrt::com_ptr<ID3D11Texture2D> texture;
  check_hresult(device->CreateTexture2D(&desc, nullptr, texture.put()));
  mCanvas =
    RenderTargetWithMultipleIdentities::Create(mDXR, texture, MaxViewCount);
  mCanvasSize = size;

  // Let's force a clean start on the clients, including resetting the session
  // ID
  mIPCSwapchain = {};
  const std::unique_lock shmLock(mSHM);
  mSHM.Detach();
}

void InterprocessRenderer::PostUserAction(UserAction action) {
  switch (action) {
    case UserAction::TOGGLE_VISIBILITY:
      mVisible = !mVisible;
      break;
    case UserAction::SHOW:
      mVisible = true;
      break;
    case UserAction::HIDE:
      mVisible = false;
      break;
    default:
      OPENKNEEBOARD_BREAK;
      return;
  }

  // Force an SHM update, even if we don't have new pixels
  if (!mVisible) {
    mPreviousFrameWasVisible = true;
  }

  mKneeboard->SetRepaintNeeded();
}

InterprocessRenderer::IPCTextureResources*
InterprocessRenderer::GetIPCTextureResources(
  uint8_t textureIndex,
  const PixelSize& size) {
  auto& ret = mIPCSwapchain.at(textureIndex);
  if (ret.mTextureSize == size) [[likely]] {
    return &ret;
  }

  OPENKNEEBOARD_TraceLoggingScopedActivity(
    activity,
    "InterprocessRenderer::GetIPCTextureResources:",
    TraceLoggingValue(textureIndex, "textureIndex"),
    TraceLoggingValue(size.mWidth, "width"),
    TraceLoggingValue(size.mHeight, "height"));

  ret = {};

  auto device = mDXR->mD3D11Device.get();

  D3D11_TEXTURE2D_DESC textureDesc {
    .Width = static_cast<UINT>(size.mWidth),
    .Height = static_cast<UINT>(size.mHeight),
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = SHM::SHARED_TEXTURE_PIXEL_FORMAT,
    .SampleDesc = {1, 0},
    .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    .MiscFlags =
      D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED,
  };

  check_hresult(
    device->CreateTexture2D(&textureDesc, nullptr, ret.mTexture.put()));
  // Our IPC textures will be used within SHM::SwapchainLength (3) frames, so
  // evicting them from VRAM to RAM will pretty much always cause problems.
  ret.mTexture->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);

  check_hresult(device->CreateRenderTargetView(
    ret.mTexture.get(), nullptr, ret.mRenderTargetView.put()));
  check_hresult(ret.mTexture.as<IDXGIResource1>()->CreateSharedHandle(
    nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, ret.mTextureHandle.put()));

  TraceLoggingWriteTagged(activity, "Creating new fence");
  check_hresult(device->CreateFence(
    0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(ret.mFence.put())));
  check_hresult(ret.mFence->CreateSharedHandle(
    nullptr, GENERIC_ALL, nullptr, ret.mFenceHandle.put()));

  ret.mViewport = {
    0,
    0,
    static_cast<FLOAT>(size.mWidth),
    static_cast<FLOAT>(size.mHeight),
    0.0f,
    1.0f,
  };
  ret.mTextureSize = size;

  return &ret;
}

std::shared_ptr<InterprocessRenderer> InterprocessRenderer::Create(
  const audited_ptr<DXResources>& dxr,
  KneeboardState* kneeboard) {
  auto ret = shared_with_final_release(new InterprocessRenderer(dxr));
  ret->Initialize(kneeboard);
  return ret;
}

OpenKneeboard::fire_and_forget InterprocessRenderer::final_release(
  std::unique_ptr<InterprocessRenderer> it) {
  // Delete in the correct thread
  co_await it->mOwnerThread;
}

std::mutex InterprocessRenderer::sSingleInstance;

InterprocessRenderer::InterprocessRenderer(const audited_ptr<DXResources>& dxr)
  : mInstanceLock(sSingleInstance),
    mDXR(dxr),
    mSHM(dxr->mAdapterLUID) {
  dprint(__FUNCTION__);
}

void InterprocessRenderer::Initialize(KneeboardState* kneeboard) {
  mKneeboard = kneeboard;
}

InterprocessRenderer::~InterprocessRenderer() {
  dprint(__FUNCTION__);
  this->RemoveAllEventListeners();
  {
    // SHM::Writer's destructor will do this, but let's make sure to
    // tear it down before the vtable and other members go - especially
    // the D3D resources
    const std::unique_lock shmLock(mSHM);
    mSHM.Detach();
  }
  {
    const std::unique_lock d2dlock(*mDXR);
    auto ctx = mDXR->mD2DDeviceContext.get();
    ctx->Flush();
    // De-allocate D3D resources while we have the lock
    mIPCSwapchain = {};
    mCanvas = {};
  }
}

task<SHM::LayerConfig> InterprocessRenderer::RenderLayer(
  const ViewRenderInfo& layer,
  const PixelRect& bounds) noexcept {
  OPENKNEEBOARD_TraceLoggingScope("InterprocessRenderer::RenderLayer");
  const auto view = layer.mView.get();

  SHM::LayerConfig ret {};
  ret.mLayerID = view->GetRuntimeID().GetTemporaryValue();

  if (layer.mVR) {
    ret.mVREnabled = true;
    ret.mVR = *layer.mVR;
    ret.mVR.mLocationOnTexture.mOffset.mX += bounds.mOffset.mX;
    ret.mVR.mLocationOnTexture.mOffset.mY += bounds.mOffset.mY;
  }

  co_await view->RenderWithChrome(
    mCanvas.get(),
    PixelRect {bounds.mOffset, layer.mFullSize},
    layer.mIsActiveForInput);

  // In-VR edit mode (M1): draw a bright frame around each panel so it's clear
  // the panel is editable. Grab/move/resize comes in later milestones.
  if (mKneeboard->IsVREditMode()) {
    auto d2d = mCanvas->d2d();
    winrt::com_ptr<ID2D1SolidColorBrush> brush;
    d2d->CreateSolidColorBrush(
      D2D1::ColorF(0.0f, 0.8f, 1.0f, 0.95f), brush.put());
    constexpr float strokeWidth = 12.0f;
    const auto o = bounds.mOffset;
    const auto s = layer.mFullSize;
    const D2D1_RECT_F frame {
      static_cast<float>(o.mX) + strokeWidth / 2,
      static_cast<float>(o.mY) + strokeWidth / 2,
      static_cast<float>(o.mX + s.mWidth) - strokeWidth / 2,
      static_cast<float>(o.mY + s.mHeight) - strokeWidth / 2,
    };
    d2d->DrawRectangle(frame, brush.get(), strokeWidth);

    // Control hints, drawn once on the input-active panel.
    if (layer.mIsActiveForInput) {
      if (!mEditHintFormat) {
        mDXR->mDWriteFactory->CreateTextFormat(
          L"Segoe UI",
          nullptr,
          DWRITE_FONT_WEIGHT_SEMI_BOLD,
          DWRITE_FONT_STYLE_NORMAL,
          DWRITE_FONT_STRETCH_NORMAL,
          32.0f,
          L"en-us",
          mEditHintFormat.put());
      }
      static constexpr wchar_t hint[] =
        L"SETUP MODE\n"
        L"Click:  grab / drop panel\n"
        L"Move mouse:  reposition\n"
        L"Scroll:  resize  (distance when grabbed)\n"
        L"Shift + move:  rotate / tilt\n"
        L"Setup button:  exit + save";
      const float pad = 24.0f;
      const D2D1_RECT_F box {
        static_cast<float>(o.mX) + pad,
        static_cast<float>(o.mY) + pad,
        static_cast<float>(o.mX) + pad + 620.0f,
        static_cast<float>(o.mY) + pad + 320.0f,
      };
      winrt::com_ptr<ID2D1SolidColorBrush> bg;
      d2d->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.55f), bg.put());
      d2d->FillRectangle(box, bg.get());
      if (mEditHintFormat) {
        const D2D1_RECT_F textRect {
          box.left + 20.0f, box.top + 14.0f, box.right - 12.0f, box.bottom};
        d2d->DrawTextW(
          hint,
          static_cast<UINT32>(std::size(hint) - 1),
          mEditHintFormat.get(),
          textRect,
          brush.get());
      }
    }
  }

  co_return ret;
}

task<void> InterprocessRenderer::RenderNow() noexcept {
  if (mRendering.test_and_set()) {
    dprint("Two renders in the same instance");
    OPENKNEEBOARD_BREAK;
    co_return;
  }

  const scope_exit markDone([this]() { mRendering.clear(); });

  OPENKNEEBOARD_TraceLoggingScopedActivity(
    activity, "InterprocessRenderer::RenderNow()");

  const auto renderInfos = mKneeboard->GetViewRenderInfo();
  const auto layerCount = renderInfos.size();

  // layerCount == 0 'should' be impossible as it's not meant to be possible to
  // disable the non-VR view for view 1, however a bug in v1.10.0 and v1.10.2
  // could lead to view 1 being fully disabled
  if (layerCount == 0 || !mVisible) {
    if (layerCount == 0) {
      TraceLoggingWriteTagged(activity, "NoLayers");
    } else {
      TraceLoggingWriteTagged(activity, "Invisible");
    }
    if (mSHM && mPreviousFrameWasVisible) {
      std::unique_lock lock(mSHM);
      mSHM.SubmitEmptyFrame();
    }
    mPreviousFrameWasVisible = false;
    co_return;
  }
  mPreviousFrameWasVisible = true;

  const auto canvasSize = Spriting::GetBufferSize(layerCount);

  TraceLoggingWriteTagged(activity, "AcquireDXLock/start");
  const std::unique_lock dxlock(*mDXR);
  TraceLoggingWriteTagged(activity, "AcquireDXLock/stop");
  this->InitializeCanvas(canvasSize);
  mDXR->mD3D11ImmediateContext->ClearRenderTargetView(
    mCanvas->d3d().rtv(), DirectX::Colors::Transparent);

  std::vector<SHM::LayerConfig> shmLayers;
  shmLayers.reserve(layerCount);
  uint64_t inputLayerID = 0;

  for (uint8_t i = 0; i < layerCount; ++i) {
    const auto bounds = Spriting::GetRect(i, layerCount);
    const auto& renderInfo = renderInfos.at(i);
    if (renderInfo.mIsActiveForInput) {
      inputLayerID = renderInfo.mView->GetRuntimeID().GetTemporaryValue();
    }

    mCanvas->SetActiveIdentity(i);

    shmLayers.push_back(co_await this->RenderLayer(renderInfo, bounds));
  }

  this->SubmitFrame(shmLayers, inputLayerID);
}

}// namespace OpenKneeboard
