// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <OpenKneeboard/RayIntersectsRect.hpp>
#include <OpenKneeboard/SHM/ActiveConsumers.hpp>
#include <OpenKneeboard/VRKneeboard.hpp>

#include <algorithm>
#include <cmath>
#include <ranges>

using namespace DirectX::SimpleMath;

namespace OpenKneeboard {

VRKneeboard::Pose VRKneeboard::GetKneeboardPose(
  const VRRenderSettings& vr,
  const SHM::LayerConfig& layer,
  const Pose& hmdPose) {
  if (!mEyeHeight) {
    mEyeHeight = {hmdPose.mPosition.y};
  }
  const auto& pose = layer.mVR.mPose;
  this->MaybeRecenter(vr, hmdPose);
  auto matrix = Matrix::CreateRotationX(pose.mRX)
    * Matrix::CreateRotationY(pose.mRY) * Matrix::CreateRotationZ(pose.mRZ)
    * Matrix::CreateTranslation({
      pose.mX,
      pose.mEyeY + *mEyeHeight,
      pose.mZ,
    })
    * mRecenter;

  return {
    .mPosition = matrix.Translation(),
    .mOrientation = Quaternion::CreateFromRotationMatrix(matrix),
  };
}

VRPose VRKneeboard::WorldPositionToVRPose(
  const Vector3& worldPosition,
  const VRPose& base,
  const Quaternion* worldOrientation) const {
  // GetKneeboardPose builds: worldPos = Transform((mX, mEyeY + eyeHeight, mZ),
  // mRecenter). Invert that to recover the stored, recenter-relative pose.
  const float eyeHeight = mEyeHeight.value_or(0.0f);
  const auto local = Vector3::Transform(worldPosition, mRecenter.Invert());
  VRPose ret = base;
  ret.mX = local.x;
  ret.mEyeY = local.y - eyeHeight;
  ret.mZ = local.z;

  if (worldOrientation) {
    // Forward rotation = RotX(RX)*RotY(RY)*RotZ(RZ) * recenterRotation. Remove
    // the recenter, then read the X/Y/Z Euler angles back out of the matrix.
    Matrix recenterRot = mRecenter;
    recenterRot._41 = 0;
    recenterRot._42 = 0;
    recenterRot._43 = 0;
    const Matrix m
      = Matrix::CreateFromQuaternion(*worldOrientation) * recenterRot.Invert();
    ret.mRY = std::asin(std::clamp(-m._13, -1.0f, 1.0f));
    ret.mRX = std::atan2(m._23, m._33);
    ret.mRZ = std::atan2(m._12, m._11);
  }
  return ret;
}

Vector2 VRKneeboard::GetKneeboardSize(
  const SHM::Config& config,
  const SHM::LayerConfig& layer,
  bool isLookingAtKneeboard) {
  const auto sizes = this->GetSizes(config.mVR, layer);

  return config.mVR.mForceZoom
      || (isLookingAtKneeboard && layer.mVR.mEnableGazeZoom)
    ? sizes.mZoomedSize
    : sizes.mNormalSize;
}

VRKneeboard::Sizes VRKneeboard::GetSizes(
  const VRRenderSettings&,
  const SHM::LayerConfig& layer) const {
  const auto& physicalSize = layer.mVR.mPhysicalSize;
  const auto virtualWidth = physicalSize.mWidth;
  const auto virtualHeight = physicalSize.mHeight;

  return {
    .mNormalSize = {virtualWidth, virtualHeight},
    .mZoomedSize =
      {virtualWidth * layer.mVR.mZoomScale,
       virtualHeight * layer.mVR.mZoomScale},
  };
}

void VRKneeboard::MaybeRecenter(
  const VRRenderSettings& vr,
  const Pose& hmdPose) {
  if (vr.mRecenterCount == mRecenterCount) {
    return;
  }
  this->Recenter(vr, hmdPose);
}

void VRKneeboard::Recenter(const VRRenderSettings& vr, const Pose& hmdPose) {
  auto pos = hmdPose.mPosition;
  mEyeHeight = {pos.y};
  pos.y = 0;

  // We're only going to respect ry (yaw) as we want the new
  // center to remain gravity-aligned

  auto quat = hmdPose.mOrientation;

  // clang-format off
    mRecenter =
      Matrix::CreateRotationY(quat.ToEuler().y) 
      * Matrix::CreateTranslation({pos.x, pos.y, pos.z});
  // clang-format on

  mRecenterCount = vr.mRecenterCount;
}

std::vector<VRKneeboard::Layer> VRKneeboard::GetLayers(
  const SHM::Config& config,
  const std::span<const SHM::LayerConfig>& layers,
  const Pose& hmdPose) {
  if (!mEyeHeight) {
    mEyeHeight = {hmdPose.mPosition.y};
  }

  std::vector<Layer> ret;
  ret.reserve(layers.size());
  for (const auto& layerConfig: layers) {
    if (!layerConfig.mVREnabled) {
      continue;
    }

    ret.push_back(
      Layer {&layerConfig, GetRenderParameters(config, layerConfig, hmdPose)});
  }

  if (!config.mVR.mEnableGazeInputFocus) {
    return ret;
  }
  const auto activeLayerID = config.mGlobalInputLayerID;

  const auto activeIt =
    std::ranges::find(ret, activeLayerID, [](const Layer& layer) {
      return layer.mLayerConfig->mLayerID;
    });
  if (
    (activeIt != ret.end())
    && (activeIt->mRenderParameters.mIsLookingAtKneeboard)) {
    return ret;
  }

  for (const auto& [layerConfig, renderParams]:
       std::ranges::reverse_view(ret)) {
    if (
      renderParams.mIsLookingAtKneeboard
      && layerConfig->mLayerID != activeLayerID) {
      SHM::ActiveConsumers::SetActiveInGameViewID(layerConfig->mLayerID);
      return ret;
    }
  }

  return ret;
}

VRKneeboard::RenderParameters VRKneeboard::GetRenderParameters(
  const SHM::Config& config,
  const SHM::LayerConfig& layer,
  const Pose& hmdPose) {
  const auto kneeboardPose = this->GetKneeboardPose(config.mVR, layer, hmdPose);
  const auto isLookingAtKneeboard =
    this->IsLookingAtKneeboard(config, layer, hmdPose, kneeboardPose);

  return {
    .mKneeboardPose = kneeboardPose,
    .mKneeboardSize =
      this->GetKneeboardSize(config, layer, isLookingAtKneeboard),
    .mKneeboardOpacity = isLookingAtKneeboard ? layer.mVR.mOpacity.mGaze
                                              : layer.mVR.mOpacity.mNormal,
    .mIsLookingAtKneeboard = isLookingAtKneeboard,
  };
}

bool VRKneeboard::IsLookingAtKneeboard(
  const SHM::Config& config,
  const SHM::LayerConfig& layer,
  const Pose& hmdPose,
  const Pose& kneeboardPose) {
  auto& isLookingAtKneeboard = mIsLookingAtKneeboard[layer.mLayerID];

  if (
    layer.mVR.mGazeTargetScale.mHorizontal < 0.1
    || layer.mVR.mGazeTargetScale.mVertical < 0.1) {
    return false;
  }

  const auto sizes = this->GetSizes(config.mVR, layer);
  auto currentSize =
    isLookingAtKneeboard ? sizes.mZoomedSize : sizes.mNormalSize;

  currentSize.x *= layer.mVR.mGazeTargetScale.mHorizontal;
  currentSize.y *= layer.mVR.mGazeTargetScale.mVertical;

  isLookingAtKneeboard = RayIntersectsRect(
    hmdPose.mPosition,
    hmdPose.mOrientation,
    kneeboardPose.mPosition,
    kneeboardPose.mOrientation,
    currentSize);

  return isLookingAtKneeboard;
}

}// namespace OpenKneeboard
