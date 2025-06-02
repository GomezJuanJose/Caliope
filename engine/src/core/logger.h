#pragma once
#include "../defines.h"

#include <memory>

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
		LOG_LEVEL_INFO = 3,
		LOG_LEVEL_DEBUG = 4
	} log_level;

	bool logger_initialize(int* memory_requirement, void* state);
	void logger_shutdow();

	CE_API void logger_output(log_level level, const std::string string, ...);
}

#ifndef CE_LOG_FATAL
#define CE_LOG_FATAL(message, ...) caliope::logger_output(caliope::LOG_LEVEL_FATAL, message, ##__VA_ARGS__)
#endif

#ifndef CE_LOG_ERROR
#define CE_LOG_ERROR(message, ...) caliope::logger_output(caliope::LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#endif

#if CE_WARNING_ENABLED == 1
#define CE_LOG_WARNING(message, ...) caliope::logger_output(caliope::LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#endif

#if CE_INFO_ENABLED == 1
#define CE_LOG_INFO(message, ...) caliope::logger_output(caliope::LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#endif

#if CE_DEBUG_ENABLED == 1
#define CE_LOG_DEBUG(message, ...) caliope::logger_output(caliope::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#endif