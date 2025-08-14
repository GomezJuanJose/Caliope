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
#include "systems/geometry_system.h"
#include "systems/camera_system.h"
#include "systems/sprite_animation_system.h"
#include "systems/ecs_system.h"

#include "math/transform.h"

#include "loaders/resources_types.inl"
#include <glm/glm.hpp>

namespace caliope {

	typedef struct application_state {
		std::shared_ptr<program_config> program_config;

		float last_frame_time;

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

		renderer_frontend_config renderer_config;
		renderer_config.application_name = config.name;
		renderer_config.window_width = config.width;
		renderer_config.window_height = config.height;
		renderer_config.max_number_quads = 10000;// TODO: Configure based on the resources needed for the developed game
		renderer_config.max_textures_per_batch = 400;
		if (!renderer_system_initialize(renderer_config)) {
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

		if (!sprite_animation_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize animation system; shutting down");
			return false;
		}

		if (!geometry_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize geometry system; shutting down");
			return false;
		}

		if (!camera_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize camera system; shutting down");
			return false;
		}

		if (!ecs_system_initialize()) {
			CE_LOG_FATAL("Failed to initialize ecs system; shutting down");
			return false;
		}


		
		if (!state_ptr->program_config->initialize(state_ptr->program_config->game_state)) {
			CE_LOG_FATAL("Failed to initialize the program; shutting down");
			return false;
		}

		// Register events
		event_register(EVENT_CODE_MOUSE_RESIZED, application_on_resize);


		CE_LOG_INFO(get_memory_stats().c_str());
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
				float current_time = platform_system_get_time();
				float delta_time = current_time - state_ptr->last_frame_time;
				state_ptr->last_frame_time = current_time;

				if (!state_ptr->program_config->update(state_ptr->program_config->game_state, delta_time)) {
					CE_LOG_ERROR("Failed to update the program;");
				}

				renderer_packet packet;
				packet.delta_time = delta_time;
				packet.world_camera = state_ptr->program_config->game_state.world_camera;
				uint quad_id = 0;

				//TODO: Move to the future view system when builds the package
				// Gets all sprites entities
				std::vector<std::vector<void*>>& sprite_data = ecs_system_get_archetype_data(ARCHETYPE_SPRITE);
				for (uint entity_index = 0; entity_index < sprite_data[0].size(); ++entity_index) {
					
					quad_definition quad_definition;
					quad_definition.id = quad_id;

					transform_component* tran_comp = (transform_component*)sprite_data[0][entity_index];
					transform transform = transform_create();
					transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
					transform_set_scale(transform, tran_comp->scale);
					transform_set_position(transform, tran_comp->position);
					quad_definition.transform = transform;

					material_component* sprite_comp = (material_component*)sprite_data[1][entity_index];
					packet.sprite_definitions.insert({ material_system_adquire(std::string(sprite_comp->material_name.data()))->shader->name, {} });
					quad_definition.material_name = std::string(sprite_comp->material_name.data()); // TODO: Change to char array
					quad_definition.z_order = sprite_comp->z_order;
					quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
						*material_system_adquire(std::string(sprite_comp->material_name.data()))->diffuse_texture,
						sprite_comp->texture_region[0], 
						sprite_comp->texture_region[1]
					);


					packet.sprite_definitions.at(
						material_system_adquire(std::string(sprite_comp->material_name.data()))->shader->name
					).push(quad_definition);

					quad_id++;
				}

				//TODO: Move to the future view system when builds the package
				// Gets all animations sprites entities
				std::vector<std::vector<void*>>& sprite_animation_data = ecs_system_get_archetype_data(ARCHETYPE_SPRITE_ANIMATION);
				for (uint entity_index = 0; entity_index < sprite_animation_data[0].size(); ++entity_index) {

					quad_definition quad_definition;
					quad_definition.id = quad_id;

					transform_component* tran_comp = (transform_component*)sprite_animation_data[0][entity_index];
					transform transform = transform_create();
					transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
					transform_set_scale(transform, tran_comp->scale);
					transform_set_position(transform, tran_comp->position);
					quad_definition.transform = transform;

					material_animation_component* anim_comp = (material_animation_component*)sprite_animation_data[1][entity_index];
					sprite_frame& frame = sprite_animation_system_acquire_frame(std::string(anim_comp->animation_name.data()), delta_time);
					packet.sprite_definitions.insert({ material_system_adquire(frame.material_name)->shader->name, {} });
					quad_definition.material_name = frame.material_name;
					quad_definition.z_order = anim_comp->z_order;
					quad_definition.texture_region = frame.texture_region;


					packet.sprite_definitions.at(
						material_system_adquire(frame.material_name)->shader->name
					).push(quad_definition);

					quad_id++;
				}


				if (!renderer_draw_frame(packet)) {
					CE_LOG_FATAL("Failed to render frame");
					return false;
				}

				input_system_update_inputs(delta_time);
			}
		}

		state_ptr->program_config.reset();
		state_ptr.reset();

		ecs_system_shutdown();

		camera_system_shutdown();

		renderer_system_stop();

		geometry_system_shutdown();

		sprite_animation_system_shutdown();

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
		float width = size[0];
		float height = size[1];
		float aspect_ratio = width / height;
		renderer_on_resized(width, height);

		state_ptr->is_suspended = false;
		if (size[0] == 0 || size[1] == 0) {
			state_ptr->is_suspended = true;
		}

		return false;
	}

}