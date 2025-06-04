#pragma once

namespace caliope {
	bool platform_system_startup(const std::string& window_name, int width, int height);
	void platform_system_shutdown();

	void* platform_system_allocate_memory(size_t size);
}