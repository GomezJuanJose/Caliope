#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_imageviews_create(vulkan_context& context);
	void vulkan_imageview_destroy(vulkan_context& context);

	VkImageView vulkan_image_view_create(vulkan_context& context, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

	bool vulkan_image_create(vulkan_context& context, uint width, uint height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);

	void vulkan_image_transition_layout(vulkan_context& context, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);

	void vulkan_image_copy_buffer_to_image(vulkan_context& context, VkBuffer buffer, VkImage image, uint width, uint height);
}