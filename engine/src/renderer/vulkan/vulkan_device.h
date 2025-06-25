#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_device_create(vulkan_context& context);
	void vulkan_device_destroy(vulkan_context& context);

	swapchain_support_details vulkan_device_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkFormat vulkan_device_check_format_support(vulkan_context& context, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
}