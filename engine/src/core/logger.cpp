#include "logger.h"
#include <cstdarg>
#include <iostream>
#include <stdio.h>

namespace caliope {

	typedef struct logger_state {
		bool initialized;
	} logger_state;

	static logger_state* state_ptr;

	bool logger_initialize(int* memory_requirement, void* state) {
		*memory_requirement = sizeof(logger_state);
		
		if (state == nullptr) {
			return true;
		}

		state_ptr = static_cast<logger_state*>(state);
		state_ptr->initialized = true;
		return true;
	}

	void logger_shutdow() {
		state_ptr = nullptr;

	}

	void logger_output(log_level level, const std::string string, ...) {
		std::string string_levels[5]{ "FATAL", "ERROR", "WARNING", "INFO", "DEBUG" };
		std::string color_levels[5]{ "0;41", "31;0", "33;0", "96;0", "90;0"};


		char buffer[512];
		std::va_list args;
		va_start(args, string);
		vsprintf_s(buffer, string.c_str(), args);
		va_end(args);

		std::string format_string(buffer);

		std::string message = "\033["+ color_levels[level] + "m["+ string_levels[level] + "]: " + format_string + "\n\033[0m";

		std::printf(format_string.c_str());
	}
}