// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.
#pragma once

#include <VisorVR/Vulkan/Dispatch.hpp>
#include <VisorVR/Vulkan/SpriteBatch.hpp>
#include <VisorVR/Vulkan/vkresult.hpp>

#include <VisorVR/dprint.hpp>

#include <shims/vulkan/vulkan.h>

namespace VisorVR::Vulkan {

std::optional<uint32_t> FindMemoryType(
  Dispatch*,
  VkPhysicalDevice,
  uint32_t filter,
  VkMemoryPropertyFlags flags);
}// namespace VisorVR::Vulkan
