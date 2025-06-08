#include "vulkan_renderer.h"

namespace caliope {
	bool vulkan_renderer_backend_initialize(const std::string& application_name) {
		return false;
	}

	void vulkan_renderer_backend_shutdown() {
	}

	void vulkan_renderer_backend_resize(uint16 width, uint16 height) {
	}

	bool vulkan_renderer_begin_frame(float delta_time) {
		return false;
	}

	bool vulkan_renderer_end_frame(float delta_time) {
		return false;
	}
}