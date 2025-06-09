#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_device_create(vulkan_context& context);
	void vulkan_device_destroy(vulkan_context& context);
}