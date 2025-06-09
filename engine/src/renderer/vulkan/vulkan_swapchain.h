#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_swapchain_create(vulkan_context& context);
	void vulkan_swapchain_destroy(vulkan_context& context);
}