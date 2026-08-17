// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/VRSettings.hpp>

#include <VisorVR/json/VRSettings.hpp>

#include <VisorVR/json.hpp>

namespace VisorVR {

VRPose VRPose::GetHorizontalMirror() const {
  auto ret = *this;
  ret.mX = -ret.mX;
  // Yaw
  ret.mRY = -ret.mRY;
  // Roll
  ret.mRZ = -ret.mRZ;
  return ret;
}

VISORVR_DEFINE_SPARSE_JSON(VRPose, mX, mEyeY, mZ, mRX, mRY, mRZ)

NLOHMANN_JSON_SERIALIZE_ENUM(
  VRRenderSettings::Quirks::Upscaling,
  {
    {VRRenderSettings::Quirks::Upscaling::Automatic, "Automatic"},
    {VRRenderSettings::Quirks::Upscaling::AlwaysOff, "AlwaysOff"},
    {VRRenderSettings::Quirks::Upscaling::AlwaysOn, "AlwaysOn"},
  });

VISORVR_DEFINE_SPARSE_JSON(VRRenderSettings::Quirks, mOpenXR_Upscaling)

VISORVR_DEFINE_SPARSE_JSON(GazeTargetScale, mVertical, mHorizontal);

VISORVR_DEFINE_SPARSE_JSON(VROpacitySettings, mNormal, mGaze);

VISORVR_DEFINE_SPARSE_JSON(
  VRRenderSettings,
  mQuirks,
  mEnableGazeInputFocus)

template <>
void from_json_postprocess<VRSettings>(const nlohmann::json& j, VRSettings& v) {
  from_json(j, static_cast<VRRenderSettings&>(v));
  from_json(j, v.mDeprecated.mPrimaryLayer);

  // Backwards compatibility
  if (j.contains("height")) {
    v.mDeprecated.mMaxHeight = j.at("height");
  }
  if (j.contains("width")) {
    v.mDeprecated.mMaxWidth = j.at("width");
  }
}

template <>
void to_json_postprocess<VRSettings>(
  nlohmann::json& j,
  const VRSettings& parent_v,
  const VRSettings& v) {
  to_json_with_default(
    j,
    static_cast<const VRRenderSettings&>(parent_v),
    static_cast<const VRRenderSettings&>(v));
  to_json_with_default(
    j, parent_v.mDeprecated.mPrimaryLayer, v.mDeprecated.mPrimaryLayer);
}

VISORVR_DEFINE_SPARSE_JSON(
  VRSettings::Deprecated,
  mMaxWidth,
  mMaxHeight,
  mEnableGazeZoom,
  mZoomScale,
  mGazeTargetScale,
  mOpacity)
VISORVR_DEFINE_SPARSE_JSON(VRSettings, mEnableSteamVR)

}// namespace VisorVR
