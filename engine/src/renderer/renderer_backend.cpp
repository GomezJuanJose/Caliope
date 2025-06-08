#include "renderer_backend.h"

#include "renderer/vulkan/vulkan_renderer.h"


namespace caliope {
	bool renderer_backend_system_create(renderer_backend_type type, renderer_backend& out_renderer_backend) {
		if (type == renderer_backend_type::BACKEND_TYPE_VULKAN) {
			out_renderer_backend.initialize = vulkan_renderer_backend_initialize;
			out_renderer_backend.shutdown = vulkan_renderer_backend_shutdown;
			out_renderer_backend.resize = vulkan_renderer_backend_resize;
			out_renderer_backend.begin_frame = vulkan_renderer_begin_frame;
			out_renderer_backend.end_frame = vulkan_renderer_end_frame;
		
			return true;
		}

		return false;
	}

	void renderer_backend_system_destroy(renderer_backend& renderer_backend) {
		renderer_backend.initialize = nullptr;
		renderer_backend.shutdown = nullptr;
		renderer_backend.resize = nullptr;
		renderer_backend.begin_frame = nullptr;
		renderer_backend.end_frame = nullptr;
	}
}