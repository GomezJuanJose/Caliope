#include "vulkan_utils.h"

#include "core/logger.h"

namespace caliope {
	int find_memory_type(VkPhysicalDevice& device, int type_filter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

		for (int i = 0; memory_properties.memoryTypeCount; ++i) {
			if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		CE_LOG_FATAL("find_memory_type failed to find suitable memory type, returning -1.");
		return -1;
	}
}