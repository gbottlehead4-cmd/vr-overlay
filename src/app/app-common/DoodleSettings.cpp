// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.

#include <VisorVR/DoodleSettings.hpp>

#include <VisorVR/json.hpp>

namespace VisorVR {
VISORVR_DEFINE_SPARSE_JSON(
  DoodleSettings::Tool,
  mMinimumRadius,
  mSensitivity)
VISORVR_DEFINE_SPARSE_JSON(DoodleSettings, mPen, mEraser)

}// namespace VisorVR
