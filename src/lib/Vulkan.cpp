// OpenKneeboard
//
// Copyright (c) 2025 Fred Emmott <fred@fredemmott.com>
//
// This program is open source; see the LICENSE file in the root of the
// OpenKneeboard repository.

#include <VisorVR/Vulkan.hpp>

namespace VisorVR::Vulkan {

Dispatch::Dispatch(
  VkInstance instance,
  PFN_vkGetInstanceProcAddr getInstanceProcAddr) {
#define IT(vkfun) \
  this->vkfun = reinterpret_cast<PFN_vk##vkfun>( \
    getInstanceProcAddr(instance, "vk" #vkfun));
  VISORVR_VK_FUNCS
#undef IT
}

std::optional<uint32_t> FindMemoryType(
  Dispatch* vk,
  VkPhysicalDevice physicalDevice,
  uint32_t filter,
  VkMemoryPropertyFlags flags) {
  VkPhysicalDeviceMemoryProperties properties;
  vk->GetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
  for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
    if (!(filter & (1 << i))) {
      continue;
    }
    if ((flags & properties.memoryTypes[i].propertyFlags) == flags) {
      return i;
    }
  }

  return {};
}

}// namespace VisorVR::Vulkan
