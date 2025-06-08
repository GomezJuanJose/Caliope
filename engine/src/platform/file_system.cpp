#include "platform/file_system.h"

#include "core/logger.h"

#include <stdio.h>
#include <sys/stat.h>

namespace caliope {
	bool file_system_exists(std::string& path) {
#ifdef _MSC_VER
		struct _stat buffer;
		return _stat(path.c_str(), &buffer);
#else
		struct stat buffer;
		return stat(path.c_str(), &buffer) == 0;
#endif // _MSC_VER

	}

	bool file_system_open(std::string& path, file_modes mode, bool binary, file_handle& out_handle) {
		out_handle.is_valid = false;
		out_handle.handle = nullptr;
		std::string mode_str;

		if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0) {
			mode_str = binary ? "w+b" : "w+";
		}
		else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0) {
			mode_str = binary ? "rb" : "r";
		}
		else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0) {
			mode_str = binary ? "wb" : "w";
		}
		else {
			CE_LOG_ERROR("Invalid mode passed while trying to open file: '%s'", path);
			return false;
		}

		FILE* file = fopen(path.c_str(), mode_str.c_str());
		if (!file) {
			CE_LOG_ERROR("Error opening file '%s'", path);
			return false;
		}

		out_handle.handle = file;
		out_handle.is_valid = true;

		return true;
	}

	void file_system_close(file_handle& handle) {
		if (handle.is_valid) {
			fclose(std::any_cast<FILE*>(handle.handle));
			handle.handle = nullptr;
			handle.is_valid = false;
		}
	}

	bool file_system_size(file_handle& handle, uint64& out_size) {
		if (handle.is_valid) {
			fseek(std::any_cast<FILE*>(handle.handle), 0, SEEK_END);
			out_size = ftell(std::any_cast<FILE*>(handle.handle));
			rewind(std::any_cast<FILE*>(handle.handle));
			return true;
		}
	}

	bool file_system_read_line(file_handle& handle, uint64 max_length, std::string& line_buf, uint64& out_line_lenght) {
		if (handle.is_valid && max_length > 0) {
			char* buffer;
			if (fgets(buffer, max_length, std::any_cast<FILE*>(handle.handle)) != 0) {
				line_buf = buffer;
				out_line_lenght = line_buf.size();
				return true;
			}
		}

		return false;
	}

	bool file_system_write_line(file_handle& handle, const std::string& text) {
		if (handle.is_valid) {
			int result = fputs(text.c_str(), std::any_cast<FILE*>(handle.handle));
			if (result != EOF) {
				result = fputc('\n', std::any_cast<FILE*>(handle.handle));
			}

			fflush(std::any_cast<FILE*>(handle.handle));
			return result != EOF;
		}

		return false;
	}

	bool file_system_read_all_bytes(file_handle& handle, std::shared_ptr<void> out_bytes, uint64& out_bytes_read) {
		if (handle.is_valid) {
			uint64 size = 0;
			if (!file_system_size(handle, size)) {
				return false;
			}

			out_bytes_read = fread(out_bytes.get(), 1, size, std::any_cast<FILE*>(handle.handle));
			return out_bytes_read == size;
		}

		return false;
	}

	bool file_system_read_all_text(file_handle& handle, std::string& out_text, uint64& out_bytes_read) {
		if (handle.is_valid) {
			uint64 size = 0;
			if (!file_system_size(handle, size)) {
				return false;
			}

			char* buffer;
			out_bytes_read = fread(buffer, 1, size, std::any_cast<FILE*>(handle.handle));
			out_text = buffer;
			return out_bytes_read == size;
		}

		return false;
	}
}