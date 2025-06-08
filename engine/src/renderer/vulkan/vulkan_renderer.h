#pragma once
#include "renderer/renderer_types.inl"

#include <string>

namespace caliope {
	bool vulkan_renderer_backend_initialize(const std::string& application_name);
	void vulkan_renderer_backend_shutdown();
	void vulkan_renderer_backend_resize(uint16 width, uint16 height);

	bool vulkan_renderer_begin_frame(float delta_time);
	bool vulkan_renderer_end_frame(float delta_time);
}