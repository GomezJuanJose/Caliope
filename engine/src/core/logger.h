#pragma once
#include "../defines.h"
#include <string>

#define CE_WARNING_ENABLED 1
#define CE_INFO_ENABLED 1
#define CE_DEBUG_ENABLED 1

#ifdef CE_RELEASE
	#define CE_INFO_ENABLED 0
	#define CE_DEBUG_ENABLED 0
#endif // CE_RELEASE

namespace caliope {
	typedef enum log_level {
		LOG_LEVEL_FATAL = 0,
		LOG_LEVEL_ERROR = 1,
		LOG_LEVEL_WARNING = 2,
		LOG_LEVEL_INFO = 3
	} log_level;

	bool logger_system_initialize();
	void logger_system_shutdown();

	CE_API void logger_output(const char* string, log_level level, ...);
}

#ifndef CE_LOG_FATAL
#define CE_LOG_FATAL(message, ...) caliope::logger_output(message, caliope::LOG_LEVEL_FATAL, ##__VA_ARGS__)
#endif

#ifndef CE_LOG_ERROR
#define CE_LOG_ERROR(message, ...) caliope::logger_output(message, caliope::LOG_LEVEL_ERROR, ##__VA_ARGS__)
#endif

#if CE_WARNING_ENABLED == 1
#define CE_LOG_WARNING(message, ...) caliope::logger_output(message, caliope::LOG_LEVEL_WARNING, ##__VA_ARGS__)
#endif

#if CE_INFO_ENABLED == 1
#define CE_LOG_INFO(message, ...) caliope::logger_output(message, caliope::LOG_LEVEL_INFO, ##__VA_ARGS__)
#endif