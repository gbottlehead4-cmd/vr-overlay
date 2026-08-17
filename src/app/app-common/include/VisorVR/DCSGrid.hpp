// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Coordinates.hpp>
#include <VisorVR/DCSEvents.hpp>

#include <GeographicLib/TransverseMercator.hpp>

namespace VisorVR {

class DCSGrid final {
 public:
  using GeoReal = DCSEvents::GeoReal;
  static_assert(std::same_as<VisorVR::GeoReal, DCSGrid::GeoReal>);

  DCSGrid() = delete;
  DCSGrid(GeoReal originLat, GeoReal originLong);
  std::tuple<GeoReal, GeoReal> LatLongFromXY(GeoReal x, GeoReal y) const;

 private:
  GeoReal mOffsetX;
  GeoReal mOffsetY;
  GeoReal mZoneMeridian;
};

}// namespace VisorVR
