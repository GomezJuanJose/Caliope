#include "application.h"

#include "program_types.h"

#include "core/logger.h"
#include "platform/platform.h"

namespace caliope {

	typedef struct application_state {
		program_config* program_config;
	} application_state;

	static std::unique_ptr<application_state> state_ptr;

	bool application_create(program_config* config) {
		CE_LOG_INFO("Creating application");

		state_ptr = std::make_unique<application_state>();
		state_ptr->program_config = config;

		if (!logger_initialize()) {
			CE_LOG_FATAL("Failed to initialize logger; shutting down");
			return false;
		}

		if (!platform_system_startup("", 0, 0)) {
			CE_LOG_FATAL("Failed to initialize platform; shutting down");
			return false;
		}

		if (!state_ptr->program_config->initialize()) {
			CE_LOG_FATAL("Failed to initialize the program; shutting down");
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