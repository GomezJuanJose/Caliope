#pragma once

#include "defines.h"


namespace caliope {
	bool platform_system_initialize(const std::string& window_name, int width, int height);
	void platform_system_shutdown();

	bool platform_system_pump_event();

	std::any platform_system_get_window();

	void* platform_system_allocate_memory(size_t size);
	void platform_system_free_memory(void* block);
	void* platform_system_zero_memory(void* block, uint64 size);
	void* platform_system_copy_memory(void* dest, const void* source, uint64 size);
	void* platform_system_set_memory(void* dest, int value, uint64 size);
}