#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_buffer_create(vulkan_context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);

	bool vulkan_buffer_copy(vulkan_context& context, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

	int find_memory_type(VkPhysicalDevice& device, int type_filter, VkMemoryPropertyFlags properties);
}