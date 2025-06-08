#include "logger.h"

#include "cepch.h"

#include <cstdarg>
#include <iostream>
#include <memory>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"



// TODO: Make custom allocators to use a segment of memory from a previously allocated block to store the system i.e. dynamic allocator!!!

namespace caliope {

	typedef struct logger_system_state {
		std::shared_ptr<spdlog::logger> console_logger;
		std::shared_ptr<spdlog::logger> file_logger;
	} logger_state;

	static std::unique_ptr<logger_system_state> state_ptr;

	bool logger_system_initialize() {

		state_ptr = std::make_unique<logger_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->console_logger = spdlog::stdout_color_mt("logger");
		state_ptr->file_logger = spdlog::basic_logger_mt("file_logger", "logs/caliope_log.log");

		CE_LOG_INFO("Logger initialized");

		return true;
	}

	void logger_system_shutdown() {
		state_ptr->console_logger.reset();
		state_ptr->file_logger.reset();
		state_ptr.reset();
	}

	void logger_output(const std::string& string, log_level level, ...) {

		spdlog::level::level_enum log_levels[5]{ 
			spdlog::level::level_enum::critical, 
			spdlog::level::level_enum::err,
			spdlog::level::level_enum::warn,
			spdlog::level::level_enum::info
		};

		char buffer[512];
		std::va_list args;
		va_start(args, level);
		vsprintf_s(buffer, string.c_str(), args);
		va_end(args);

		if (state_ptr && state_ptr->console_logger && state_ptr->file_logger) {
			std::string format_string(buffer);
			state_ptr->console_logger->log(log_levels[level], format_string);
			state_ptr->file_logger->log(log_levels[level], format_string);
		}
		else {
			char* message = buffer;
			std::printf("[UNINITIALIZED_LOGGER]: %s\n",message);
		}
	}

	void logger_plain_output(log_level level, const char* string, ...) {
		const char* string_levels[] = {"FATAL", "ERROR", "WARNING", "INFO"};

		char buffer[512];
		std::va_list args;
		va_start(args, string);
		vsprintf_s(buffer, string, args);
		va_end(args);

		std::printf("[%s]: %s\n", string_levels[level], buffer);
	}
}