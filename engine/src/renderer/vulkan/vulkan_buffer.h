#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_buffer_create(vulkan_context& context, uint64 size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, bool bind_on_create, vulkan_buffer& out_buffer);
	void vulkan_buffer_destroy(vulkan_context& context, vulkan_buffer& buffer);

	void vulkan_buffer_bind(vulkan_context& context, vulkan_buffer& buffer, uint64 offset);

	void* vulkan_buffer_lock_memory(vulkan_context& context, vulkan_buffer& buffer, uint64 offset, uint64 size, uint flags);
	void vulkan_buffer_unlock_memory(vulkan_context& context, vulkan_buffer& buffer);

	void vulkan_buffer_load_data(vulkan_context& context, vulkan_buffer& buffer, uint64 offset, uint64 size, uint flags, const void* data);
	bool vulkan_buffer_copy(vulkan_context& context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source, uint source_offset, VkBuffer dest, uint64 dest_offset, uint64 size);

	int find_memory_type(VkPhysicalDevice& device, int type_filter, VkMemoryPropertyFlags properties);
}