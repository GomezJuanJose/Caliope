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

	typedef struct logger_state {
		std::shared_ptr<spdlog::logger> console_logger;
		std::shared_ptr<spdlog::logger> file_logger;
	} logger_state;

	static std::unique_ptr<logger_state> state_ptr = std::make_unique<logger_state>();

	bool logger_initialize() {

		state_ptr->console_logger = spdlog::stdout_color_mt("logger");
		state_ptr->file_logger = spdlog::basic_logger_mt("file_logger", "logs/caliope_log.log");

		CE_LOG_INFO("Logger initialized");

		return true;
	}

	void logger_shutdow() {
		state_ptr->console_logger.reset();
		state_ptr->file_logger.reset();
		state_ptr.reset();
	}

	void logger_output(log_level level, const std::string string, ...) {
		std::string string_levels[5]{ "FATAL", "ERROR", "WARNING", "INFO", "DEBUG" };
		spdlog::level::level_enum log_levels[5]{ 
			spdlog::level::level_enum::critical, 
			spdlog::level::level_enum::err,
			spdlog::level::level_enum::warn,
			spdlog::level::level_enum::info,
			spdlog::level::level_enum::debug
		};


		char buffer[512];
		std::va_list args;
		va_start(args, string);
		vsprintf_s(buffer, string.c_str(), args);
		va_end(args);

		std::string format_string(buffer);

		

		if (state_ptr && state_ptr->console_logger && state_ptr->file_logger) {
			state_ptr->console_logger->log(log_levels[level], format_string);
			state_ptr->file_logger->log(log_levels[level], format_string);
		}
		else {
			std::string message = "[" + string_levels[level] + "]: " + format_string + "\n";
			std::printf(message.c_str());
		}
	}
}