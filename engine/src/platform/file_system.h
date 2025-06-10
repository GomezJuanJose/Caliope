#pragma once
#include "defines.h"

#include <string>
#include <any>


// TODO: Make the file system platform specific!!

namespace caliope {
	typedef struct file_handle {
		std::any handle;
		bool is_valid;
	} file_handle;

	typedef enum file_modes {
		FILE_MODE_READ = 0X1,
		FILE_MODE_WRITE = 0X2
	} file_modes;

	CE_API bool file_system_exists(std::string& path);

	CE_API bool file_system_open(std::string& path, file_modes mode, bool binary, file_handle& out_handle);

	CE_API void file_system_close(file_handle& handle);

	CE_API bool file_system_size(file_handle& handle, uint64& out_size);

	CE_API bool file_system_read_line(file_handle& handle, uint64 max_length, std::string& line_buf, uint64& out_line_lenght);

	CE_API bool file_system_write_line(file_handle& handle, const std::string& text);

	CE_API bool file_system_read_all_bytes(file_handle& handle, void* out_bytes, uint64& out_bytes_read);

	CE_API bool file_system_read_all_text(file_handle& handle, std::string& out_text, uint64& out_bytes_read);

}