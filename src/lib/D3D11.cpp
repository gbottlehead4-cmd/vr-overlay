// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/D3D11.hpp>

#include <VisorVR/dprint.hpp>
#include <VisorVR/hresult.hpp>
#include <VisorVR/tracing.hpp>

#include <shims/winrt/base.h>

#include <d3d11_3.h>

namespace VisorVR::D3D11 {

DeviceContextState::DeviceContextState(ID3D11Device1* device) {
  VISORVR_TraceLoggingScope("D3D11::DeviceContextState(ID3D11Device1*)");

  auto featureLevel = device->GetFeatureLevel();
  check_hresult(device->CreateDeviceContextState(
    (device->GetCreationFlags() & D3D11_CREATE_DEVICE_SINGLETHREADED)
      ? D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED
      : 0,
    &featureLevel,
    1,
    D3D11_SDK_VERSION,
    __uuidof(ID3D11Device),
    nullptr,
    mState.put()));
}

ScopedDeviceContextStateChange::ScopedDeviceContextStateChange(
  const winrt::com_ptr<ID3D11DeviceContext1>& context,
  DeviceContextState* newState)
  : mContext(context) {
  VISORVR_TraceLoggingScope("D3D11::ScopedDeviceContextStateChange()");
  if (!newState->IsValid()) {
    winrt::com_ptr<ID3D11Device> device;
    context->GetDevice(device.put());
    *newState = {device.as<ID3D11Device1>().get()};
  }
  context->SwapDeviceContextState(newState->Get(), mOriginalState.put());
}

ScopedDeviceContextStateChange::~ScopedDeviceContextStateChange() {
  VISORVR_TraceLoggingScope("~D3D11::ScopedDeviceContextStateChange()");
  mContext->SwapDeviceContextState(mOriginalState.get(), nullptr);
}

}// namespace VisorVR::D3D11
