// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <string>

namespace VisorVR {
using GeoReal = double;
}

namespace VisorVR::Coordinates {

std::string DMSFormat(GeoReal angle, char pos, char neg);
std::string DMFormat(GeoReal angle, char pos, char neg);
std::string MGRSFormat(GeoReal latitude, GeoReal longitude);

}// namespace VisorVR::Coordinates
