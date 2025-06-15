#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_command_buffer_allocate(vulkan_context& context);

	void vulkan_command_buffer_record(vulkan_context& context, VkCommandBuffer command_buffer, uint image_index);

	VkCommandBuffer vulkan_command_buffer_single_use_begin(vulkan_context& context);
	void vulkan_command_buffer_single_use_end(vulkan_context& context, VkCommandBuffer command_buffer);
}