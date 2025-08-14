#include "logger.h"

#include "cepch.h"
#include "platform/platform.h"
#include "platform/file_system.h"

#include <cstdarg>
#include <iostream>
#include <memory>


// TODO: Make custom allocators to use a segment of memory from a previously allocated block to store the system i.e. dynamic allocator!!!

namespace caliope {

	typedef struct logger_system_state {
		file_handle logger_file_handle;
	} logger_state;

	static std::unique_ptr<logger_system_state> state_ptr;

	bool logger_system_initialize() {

		state_ptr = std::make_unique<logger_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		file_system_open(std::string("logs/caliope_log.log"), FILE_MODE_WRITE, state_ptr->logger_file_handle);

		CE_LOG_INFO("Logger initialized");

		return true;
	}

	void logger_system_shutdown() {
		file_system_close(state_ptr->logger_file_handle);
		state_ptr.reset();
	}

	void logger_output(const char* string, log_level level, ...) {
		const char* string_levels[] = { "[FATAL]: ", "[ERROR]: ", "[WARNING]: ", "[INFO]: " };
	
		char buffer[2048]; // NOTE: Increase the buffer size in case an exception of callstack is called
		std::va_list args;
		va_start(args, level);
		vsprintf_s(buffer, string, args);
		va_end(args);

		char result[2048]; // NOTE: Increase the buffer size in case an exception of callstack is called
		strcpy(result, string_levels[level]);
		strcat(result, buffer);
		strcat(result, "\n");

		char* message = result;
		platform_system_console_write(message, level);
		
		if (state_ptr && state_ptr->logger_file_handle.is_valid) {
			platform_system_file_write_text(state_ptr->logger_file_handle.handle, message);
		}
	}
}