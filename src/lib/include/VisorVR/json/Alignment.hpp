// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Alignment.hpp>

#include <VisorVR/json.hpp>

namespace VisorVR::Alignment {

NLOHMANN_JSON_SERIALIZE_ENUM(
  Horizontal,
  {
    {Horizontal::Left, "Left"},
    {Horizontal::Center, "Center"},
    {Horizontal::Right, "Right"},
  });

NLOHMANN_JSON_SERIALIZE_ENUM(
  Vertical,
  {
    {Vertical::Top, "Top"},
    {Vertical::Middle, "Middle"},
    {Vertical::Bottom, "Bottom"},
  });

}// namespace VisorVR::Alignment
