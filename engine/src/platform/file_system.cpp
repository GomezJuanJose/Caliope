#include "platform/file_system.h"
#include "platform/platform.h"

#include "core/logger.h"

#include <stdio.h>
#include <sys/stat.h>

namespace caliope {
	bool file_system_exists(std::string& path) {
#ifdef _MSC_VER
		struct _stat buffer;
		return _stat(path.c_str(), &buffer) == 0;
#else
		struct stat buffer;
		return stat(path.c_str(), &buffer) == 0;
#endif // _MSC_VER

	}

	bool file_system_open(std::string& path, file_modes mode, file_handle& out_handle) {
		out_handle.is_valid = false;
		out_handle.handle = nullptr;
		out_handle.is_valid = platform_system_open_file(path.c_str(), out_handle.handle, mode);

		return true;
	}

	void file_system_close(file_handle& handle) {
		platform_system_close_file(handle.handle);
	}

	bool file_system_size(file_handle& handle, uint64& out_size) {
		if (handle.is_valid) {
			out_size = platform_system_file_size(handle.handle);

			return true;
		}

		return false;
	}

	bool file_system_read_text(file_handle& handle, uint64 max_length, std::string& line_buf) {
		if (handle.is_valid && max_length > 0) {
			return platform_system_file_read_text(handle.handle, max_length, line_buf.data());
			
		}

		return false;
	}

	bool file_system_write_text(file_handle& handle, const std::string& text) {
		if (handle.is_valid) {
			return platform_system_file_write_text(handle.handle, text.c_str());
		}

		return false;
	}

	bool file_system_read_all_bytes(file_handle& handle, std::vector<uchar>& out_bytes, uint64& out_bytes_read) {
		if (handle.is_valid) {
			uint64 file_size = platform_system_file_size(handle.handle);

			out_bytes.resize(file_size);
			out_bytes_read = platform_system_file_read_bytes(handle.handle, file_size, out_bytes.data());

			return out_bytes_read != 0;
		}

		return false;
	}

	bool file_system_read_all_text(file_handle& handle, std::string& out_text, uint64& out_bytes_read) {
		if (handle.is_valid) {
			uint64 file_size = platform_system_file_size(handle.handle);
			return platform_system_file_read_text(handle.handle, file_size, out_text.data());
		}

		return false;
	}
}