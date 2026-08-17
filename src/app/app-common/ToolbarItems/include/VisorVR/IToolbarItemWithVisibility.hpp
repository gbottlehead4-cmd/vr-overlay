// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/IToolbarItem.hpp>

namespace VisorVR {

class IToolbarItemWithVisibility : public virtual IToolbarItem {
 public:
  virtual ~IToolbarItemWithVisibility() = default;

  virtual bool IsVisible() const = 0;
};

}// namespace VisorVR
