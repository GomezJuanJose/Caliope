#pragma once

#include "defines.h"

#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_pipeline_create(vulkan_context& context);
	void vulkan_pipeline_destroy(vulkan_context& context);
}