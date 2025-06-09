#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_imageview_create(vulkan_context& context);
	void vulkan_imageview_destroy(vulkan_context& context);
}