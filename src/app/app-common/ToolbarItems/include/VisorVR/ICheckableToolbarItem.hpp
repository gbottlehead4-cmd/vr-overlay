// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Events.hpp>
#include <VisorVR/ISelectableToolbarItem.hpp>

namespace VisorVR {

class ICheckableToolbarItem : public virtual ISelectableToolbarItem {
 public:
  virtual ~ICheckableToolbarItem() = default;

  virtual bool IsChecked() const = 0;
};

}// namespace VisorVR
