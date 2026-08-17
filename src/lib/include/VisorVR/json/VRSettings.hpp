// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/json_fwd.hpp>

namespace VisorVR {

struct GazeTargetScale;
struct VROpacitySettings;
struct VRPose;
struct VRSettings;

VISORVR_DECLARE_SPARSE_JSON(GazeTargetScale)
VISORVR_DECLARE_SPARSE_JSON(VROpacitySettings)
VISORVR_DECLARE_SPARSE_JSON(VRPose)
VISORVR_DECLARE_SPARSE_JSON(VRSettings)

}// namespace VisorVR
