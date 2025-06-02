#include "application.h"

#include "logger.h"

namespace caliope {

	typedef struct application_state {

	} application_state;

	bool application_create() {
		CE_LOG_INFO("Creating application!");

		int logger_memory_requirement;
		logger_initialize(&logger_memory_requirement, nullptr);
		void* block = malloc(logger_memory_requirement);
		if (!logger_initialize(&logger_memory_requirement, block)) {
			CE_LOG_ERROR("Failed to initialize logger; shutting down");
			return false;
		}

		return true;
	}

	bool application_run() {

		while (true) {

		}

		logger_shutdow();

		return true;
	}

}