// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <OpenKneeboard/SHM.hpp>
#include <OpenKneeboard/VRSettings.hpp>

#include <directxtk/SimpleMath.h>

namespace OpenKneeboard {

class VRKneeboard {
 public:
  using Matrix = DirectX::SimpleMath::Matrix;
  using Quaternion = DirectX::SimpleMath::Quaternion;
  using Vector2 = DirectX::SimpleMath::Vector2;
  using Vector3 = DirectX::SimpleMath::Vector3;

  struct Pose {
    Vector3 mPosition {};
    Quaternion mOrientation {};
  };

  struct RenderParameters {
    Pose mKneeboardPose;
    Vector2 mKneeboardSize;
    float mKneeboardOpacity;
    bool mIsLookingAtKneeboard;
  };

  struct Layer {
    const SHM::LayerConfig* mLayerConfig {nullptr};
    RenderParameters mRenderParameters;
  };

 protected:
  std::vector<Layer> GetLayers(
    const SHM::Config&,
    const std::span<const SHM::LayerConfig>&,
    const Pose& hmdPose);

  // Inverse of GetKneeboardPose: given a world-space position (same space
  // GetKneeboardPose returns), produce a VRPose whose stored X/EyeY/Z reproduce
  // it. If `worldOrientation` is given, RX/RY/RZ are also solved to reproduce
  // that orientation; otherwise rotations are copied from `base`. Used to
  // persist an in-VR-dragged/rotated panel back to the config file.
  VRPose WorldPositionToVRPose(
    const Vector3& worldPosition,
    const VRPose& base,
    const Quaternion* worldOrientation = nullptr) const;

 private:
  RenderParameters GetRenderParameters(
    const SHM::Config&,
    const SHM::LayerConfig&,
    const Pose& hmdPose);

  struct Sizes {
    Vector2 mNormalSize;
    Vector2 mZoomedSize;
  };

  uint64_t mRecenterCount = 0;
  Matrix mRecenter = Matrix::Identity;
  std::unordered_map<uint64_t, bool> mIsLookingAtKneeboard;
  std::optional<float> mEyeHeight;

  Pose GetKneeboardPose(
    const VRRenderSettings& vr,
    const SHM::LayerConfig&,
    const Pose& hmdPose);

  Vector2 GetKneeboardSize(
    const SHM::Config& config,
    const SHM::LayerConfig&,
    bool isLookingAtKneeboard);

  bool IsLookingAtKneeboard(
    const SHM::Config&,
    const SHM::LayerConfig&,
    const Pose& hmdPose,
    const Pose& kneeboardPose);

  Sizes GetSizes(const VRRenderSettings&, const SHM::LayerConfig&) const;

  void MaybeRecenter(const VRRenderSettings& vr, const Pose& hmdPose);
  void Recenter(const VRRenderSettings& vr, const Pose& hmdPose);
};

}// namespace OpenKneeboard
