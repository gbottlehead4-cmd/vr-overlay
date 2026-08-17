// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Events.hpp>

namespace VisorVR {

class IToolbarItem {
 public:
  virtual ~IToolbarItem();

  Event<> evStateChangedEvent;
};

}// namespace VisorVR
