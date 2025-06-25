#pragma once
#include "vulkan_types.inl"

namespace caliope {
	int find_memory_type(VkPhysicalDevice& device, int type_filter, VkMemoryPropertyFlags properties);
}