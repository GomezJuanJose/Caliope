#pragma once

#include "defines.h"

#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_renderpass_create(vulkan_context& context);
	void vulkan_renderpass_destroy(vulkan_context& context);
}