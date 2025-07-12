#pragma once

#include "defines.h"


namespace caliope {
	bool platform_system_initialize(const std::string& window_name, int width, int height);
	void platform_system_shutdown();

	bool platform_system_pump_event();

	std::any platform_system_get_window();

	float platform_system_get_time();//TODO: Do it platform specific??

	void platform_system_console_write(const char* message, uchar log_level);


	void* platform_system_allocate_memory(size_t size);
	void platform_system_free_memory(void* block);
	void* platform_system_zero_memory(void* block, uint64 size);
	void* platform_system_copy_memory(void* dest, const void* source, uint64 size);
	void* platform_system_set_memory(void* dest, int value, uint64 size);


	bool platform_system_open_file(const char* path, std::any& handle, int mode);
	void platform_system_close_file(std::any& handle);
	uint64 platform_system_file_size(std::any& handle);

	bool platform_system_file_read_text(std::any& handle, uint64 max_length, char* line_buf);
	bool platform_system_file_write_text(std::any& handle, const char* text);

	uint64 platform_system_file_read_bytes(std::any& handle, uint64 size, uchar* data);
	bool platform_system_file_write_bytes(std::any& handle, uint64 size, void* data);
}