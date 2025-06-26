#include "application.h"

#include "program_types.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/event.h"
#include "core/input.h"

#include "renderer/renderer_frontend.h"
#include "platform/file_system.h"
#include "platform/platform.h"

#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/shader_system.h"
#include "systems/material_system.h"

#include "math/transform.h"

#include "loaders/resources_types.inl"
#include <glm/glm.hpp>

namespace caliope {

	typedef struct application_state {
		std::shared_ptr<program_config> program_config;

		bool is_running;
		bool is_suspended;
	} application_state;

	static std::unique_ptr<application_state> state_ptr;

	// Event handlers
	bool application_on_resize(event_system_code code, std::any data);

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

		resource_system_config resource_config;
		resource_config.base_path = "assets\\";
		resource_config.max_number_loaders = 32;
		if (!resource_system_initialize(resource_config)) {
			CE_LOG_FATAL("Failed to initialize resource system; shutting down");
			return false;
		}

		if (!renderer_system_initialize(config.name)) {
			CE_LOG_FATAL("Failed to initialize rederer; shutting down");
			return false;
		}

		if (!texture_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize texture system; shutting down");
			return false;
		}

		if (!shader_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize shader system; shutting down");
			return false;
		}

		if (!material_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize material system; shutting down");
			return false;
		}



		if (!state_ptr->program_config->initialize()) {
			CE_LOG_FATAL("Failed to initialize the program; shutting down");
			return false;
		}

		// Register events
		event_register(EVENT_CODE_MOUSE_RESIZED, application_on_resize);

		CE_LOG_INFO("\n" + get_memory_stats());
		CE_LOG_INFO("Total usage of memory: %.2fMb/%.2fMb", get_memory_usage() / 1024.0 / 1024.0, memory_config.total_alloc_size / 1024.0 / 1024.0);

		// TODO: TEMPORAL CODE
		material_configuration m;
		m.diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m.shininess = 64.0f;
		m.diffuse_texture_name = { "dummy_character" };
		m.specular_texture_name = { "dummy_specular" };
		m.normal_texture_name = { "dummy_normal" };
		m.shader_name = { "Builtin.SpriteShader" };
		m.name = { "scene" };

		file_handle w;
		file_system_open(std::string("assets\\materials\\scene.cemat"), FILE_MODE_WRITE, w);
		file_system_write_bytes(w, sizeof(material_configuration) + 255, &m);
		file_system_close(w);
		// TODO: END TEMPORAL CODE
		
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
				//TODO: TEMP CODE
				transform t1 = transform_create();
				transform_set_rotation(t1, glm::angleAxis(glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f)));
				transform_set_scale(t1, glm::vec3(0.5f, 1.0f, 1.0f));
				transform_set_position(t1, glm::vec3(0.0f, 0.0f, 0.0f));
				transform t2 = transform_create();
				transform_set_rotation(t2, glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)));
				packet.quad_definitions.insert({ std::string("scene"), {t1} });
				// TODO: TEMP CODE

				if (!renderer_draw_frame(packet)) {
					CE_LOG_FATAL("Failed to render frame");
					return false;
				}

				input_system_update_inputs(0.0f);
			}
		}


		material_system_shutdown();

		shader_system_shutdown();

		texture_system_shutdown();

		resource_system_shutdown();

		renderer_system_shutdown();
		
		platform_system_shutdown();

		input_system_shutdown();

		event_system_shutdown();

		logger_system_shutdown();

		memory_system_shutdown();



		return true;
	}

	bool application_on_resize(event_system_code code, std::any data) {

		int* size = std::any_cast<int*>(data);
		renderer_on_resized(size[0], size[1]);

		state_ptr->is_suspended = false;
		if (size[0] == 0 || size[1] == 0) {
			state_ptr->is_suspended = true;
		}

		return false;
	}

}