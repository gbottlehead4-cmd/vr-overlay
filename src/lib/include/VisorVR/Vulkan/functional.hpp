// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include "impl/VISORVR_VK_FUNCS.h"

#include <shims/vulkan/vulkan.h>

#include <functional>
#include <type_traits>

namespace VisorVR::Vulkan {

#define IT(x) \
  using STDFN_vk##x = std::function<std::remove_pointer_t<PFN_vk##x>>;
VISORVR_VK_FUNCS
#undef IT

}// namespace VisorVR::Vulkan
