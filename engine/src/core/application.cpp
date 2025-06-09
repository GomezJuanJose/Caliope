#include "application.h"

#include "program_types.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/event.h"
#include "core/input.h"

#include "renderer/renderer_frontend.h"

#include "platform/platform.h"

namespace caliope {

	typedef struct application_state {
		std::shared_ptr<program_config> program_config;

		bool is_running;
		bool is_suspended;
	} application_state;

	static std::unique_ptr<application_state> state_ptr;

	bool application_create(program_config& config) {
		CE_LOG_INFO("Creating application");

		memory_system_configuration memory_config = {};
		memory_config.total_alloc_size = GIBIBYTES(1);
		if (!memory_system_initialize(memory_config)) {
			CE_LOG_FATAL("Failed to initialize memory system; shutting down");
			return false;
		}

		state_ptr = std::make_unique<application_state>();
		state_ptr->program_config = std::make_shared<program_config>(config);
		state_ptr->is_running = true;

		if (!logger_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize logger; shutting down");
			return false;
		}

		if (!event_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize platform; shutting down");
			return false;
		}

		if (!input_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize platform; shutting down");
			return false;
		}

		if (!platform_system_initialize(config.name, config.width, config.height)) {
			CE_LOG_FATAL("Failed to initialize platform; shutting down");
			return false;
		}

		if (!renderer_system_initialize(config.name)) {
			CE_LOG_FATAL("Failed to initialize rederer; shutting down");
			return false;
		}

		if (!state_ptr->program_config->initialize()) {
			CE_LOG_FATAL("Failed to initialize the program; shutting down");
			return false;
		}


		CE_LOG_INFO("\n" + get_memory_stats());
		CE_LOG_INFO("Total usage of memory: %.2fMb/%.2fMb", get_memory_usage() / 1024.0 / 1024.0, memory_config.total_alloc_size / 1024.0 / 1024.0);
		
		return true;
	}

	bool application_run() {

		while (state_ptr->is_running) {
			if (!platform_system_pump_event()) {
				CE_LOG_INFO("Closing application");
				state_ptr->is_running = false;
			}

			if (!state_ptr->is_suspended) {
				if (!state_ptr->program_config->update(0.0f)) {
					CE_LOG_ERROR("Failed to update the program;");
				}

				renderer_packet packet;
				if (!renderer_draw_frame(packet)) {
					CE_LOG_FATAL("Failed to render frame");
					return false;
				}

				input_system_update_inputs(0.0f);
			}
		}

		renderer_system_shutdown();

		platform_system_shutdown();

		input_system_shutdown();

		event_system_shutdown();

		logger_system_shutdown();

		memory_system_shutdown();



		return true;
	}

}