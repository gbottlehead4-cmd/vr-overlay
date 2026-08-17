// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#include <VisorVR/DirectInputSettings.hpp>

#include <VisorVR/json.hpp>

namespace VisorVR {

template <>
void from_json_postprocess<DirectInputSettings::ButtonBinding>(
  const nlohmann::json& j,
  DirectInputSettings::ButtonBinding& v) {
  if (!j.contains("Action")) {
    return;
  }

  if (j.at("Action") == "SWITCH_KNEEBOARDS") {
    v.mAction = UserAction::SWAP_FIRST_TWO_VIEWS;
  }
}

// Not using sparse json as an individual binding should not be diffed/merged:
// if either the buttons or actions differ, it's a different binding, not a
// modified one.
VISORVR_DEFINE_JSON(
  DirectInputSettings::ButtonBinding,
  mButtons,
  mAction);
VISORVR_DEFINE_SPARSE_JSON(
  DirectInputSettings::Device,
  mID,
  mName,
  mKind,
  mButtonBindings);
VISORVR_DEFINE_SPARSE_JSON(
  DirectInputSettings,
  mEnableMouseButtonBindings,
  mDevices);

}// namespace VisorVR
