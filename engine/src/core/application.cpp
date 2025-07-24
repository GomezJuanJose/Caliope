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

		if (!state_ptr->program_config->initialize(state_ptr->program_config->game_state)) {
			CE_LOG_FATAL("Failed to initialize the program; shutting down");
			return false;
		}

		// Register events
		event_register(EVENT_CODE_MOUSE_RESIZED, application_on_resize);

		CE_LOG_INFO(get_memory_stats().c_str());
		CE_LOG_INFO("Total usage of memory: %.2fMb/%.2fMb", get_memory_usage() / 1024.0 / 1024.0, memory_config.total_alloc_size / 1024.0 / 1024.0);

		// TODO: TEMPORAL CODE
		material_configuration m;
		m.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
		m.shininess_sharpness = 16.0f;
		m.shininess_intensity = 3.0f;
		m.diffuse_texture_name = { "cobblestone" };
		m.specular_texture_name = { "cobblestone_SPEC" };
		m.normal_texture_name = { "cobblestone_NRM" }; 
		//m.diffuse_texture_name = { "knight_human_man_04_alt" };
		//m.specular_texture_name = { "knightSpecularMap" };
		//m.normal_texture_name = { "knightNormalMap" };
		m.shader_name = { "Builtin.SpriteShader" };
		m.name = { "character1" };

		material_configuration m2;
		m2.diffuse_color = glm::vec3(0.5f, 1.0f, 1.0f);
		m2.shininess_sharpness = 15.0f;
		m2.shininess_intensity = 5.0f;
		m2.diffuse_texture_name = { "cottageEXTday" };
		m2.specular_texture_name = { "cottageSpecular" };
		m2.normal_texture_name = { "cottageNormal" };
		m2.shader_name = { "Builtin.SpriteShader" };
		m2.name = { "background" };

		material_configuration m3;
		m3.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
		m3.shininess_sharpness = 64.0f;
		m3.shininess_intensity = 5.0f;
		m3.diffuse_texture_name = { "warrior_human_woman_06" };
		m3.specular_texture_name = { "" };
		m3.normal_texture_name = { "" };
		m3.shader_name = { "Builtin.SpriteShader" };
		m3.name = { "character2" };

		material_configuration m4;
		m4.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
		m4.shininess_sharpness = 64.0f;
		m4.shininess_intensity = 5.0f;
		m4.diffuse_texture_name = { "map_01" };
		m4.specular_texture_name = { "" };
		m4.normal_texture_name = { "" };
		m4.shader_name = { "Builtin.SpriteShader" };
		m4.name = { "transparency" };

		material_configuration m5;
		m5.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
		m5.shininess_sharpness = 64.0f;
		m5.shininess_intensity = 5.0f;
		m5.diffuse_texture_name = { "B_witch_idle" };
		m5.specular_texture_name = { "" };
		m5.normal_texture_name = { "" };
		m5.shader_name = { "Builtin.SpriteShader" };
		m5.name = { "spritesheet" };

		texture_system_adquire(std::string("B_witch_idle"));
		texture_system_change_filter(std::string("B_witch_idle"), FILTER_NEAREST, FILTER_NEAREST);


		file_handle w;
		file_system_open(std::string("assets\\materials\\character1.cemat"), FILE_MODE_WRITE, w);
		file_system_write_bytes(w, sizeof(material_configuration), &m);
		file_system_close(w);

		file_system_open(std::string("assets\\materials\\background.cemat"), FILE_MODE_WRITE, w);
		file_system_write_bytes(w, sizeof(material_configuration), &m2);
		file_system_close(w);

		file_system_open(std::string("assets\\materials\\character2.cemat"), FILE_MODE_WRITE, w);
		file_system_write_bytes(w, sizeof(material_configuration), &m3);
		file_system_close(w);

		file_system_open(std::string("assets\\materials\\transparency.cemat"), FILE_MODE_WRITE, w);
		file_system_write_bytes(w, sizeof(material_configuration), &m4);
		file_system_close(w);

		file_system_open(std::string("assets\\materials\\spritesheet.cemat"), FILE_MODE_WRITE, w);
		file_system_write_bytes(w, sizeof(material_configuration), &m5);
		file_system_close(w);

		sprite_animation_config spritesheet_animation;
		spritesheet_animation.name = "spritesheet_animation";
		std::vector<sprite_frame> frames;
		for (uint i = 0; i < 5; ++i) {
			sprite_frame frame;
			frame.material_name = "spritesheet";
			frame.texture_region = texture_system_calculate_grid_region_coordinates(*material_system_adquire(std::string("spritesheet"))->diffuse_texture, { 32.0f, 48.0f }, i, 0);
			frames.push_back(frame);
		}
		spritesheet_animation.frames = frames;
		spritesheet_animation.is_looping = true;
		spritesheet_animation.is_playing = true;
		spritesheet_animation.frames_per_second = 6.0f;

		sprite_animation_system_register(spritesheet_animation);
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
				float current_time = platform_system_get_time();
				float delta_time = current_time - state_ptr->last_frame_time;
				state_ptr->last_frame_time = current_time;

				if (!state_ptr->program_config->update(state_ptr->program_config->game_state, delta_time)) {
					CE_LOG_ERROR("Failed to update the program;");
				}

				renderer_packet packet;
				packet.delta_time = delta_time;
				packet.world_camera = state_ptr->program_config->game_state.world_camera;

				//TODO: TEMP CODE, give all this list directly from the application or when the scene is loaded. or when a object is created/destroyed but never do this per frame or use an ECS
				transform t1 = transform_create();
				transform_set_rotation(t1, glm::angleAxis(glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f)));
				transform_set_scale(t1, glm::vec3(1.0f, 0.5f, 1.0f));
				transform_set_position(t1, glm::vec3(1.0f, 0.0f, 0.0f));
				transform t2 = transform_create();

				transform_set_rotation(t2, glm::angleAxis(glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f)));
				transform_set_scale(t2, glm::vec3(1.0f, 1.0f, 1.0f));
				transform_set_position(t2, glm::vec3(0.0f, 0.0f, 0.0f));
				transform t3 = transform_create();

				transform_set_rotation(t3, glm::angleAxis(glm::radians(45.f), glm::vec3(0.f, 0.f, 1.f)));
				transform_set_scale(t3, glm::vec3(1.0f, 1.0f, 1.0f));
				transform_set_position(t3, glm::vec3(-1.0f, 0.0f, 0.0f));

				transform t4 = transform_create();
				transform_set_rotation(t4, glm::angleAxis(glm::radians(45.f), glm::vec3(0.f, 0.f, 0.f)));
				transform_set_scale(t4, glm::vec3(5.0f, 3.0f, 1.0f));
				transform_set_position(t4, glm::vec3(0.0f, 0.0f, -1.0f));

				transform t5 = transform_create();
				transform_set_rotation(t5, glm::angleAxis(glm::radians(45.f), glm::vec3(0.f, 0.f, 0.f)));
				transform_set_scale(t5, glm::vec3(0.5f, 0.5f, 1.0f));
				transform_set_position(t5, glm::vec3(0.0f, 0.0f, 1.0f));

				transform t6 = transform_create();
				transform_set_rotation(t6, glm::angleAxis(glm::radians(45.f), glm::vec3(0.f, 0.f, 0.f)));
				transform_set_scale(t6, glm::vec3(0.5f, 0.5f, 1.0f));
				transform_set_position(t6, glm::vec3(0.2f, 0.0f, 1.0f));

				transform t7 = transform_create();
				transform_set_rotation(t7, glm::angleAxis(glm::radians(45.f), glm::vec3(0.f, 0.f, 0.f)));
				transform_set_scale(t7, glm::vec3(0.25f, 0.5f, 1.0f));
				transform_set_position(t7, glm::vec3(1.0f, 0.6f, 1.0f));

				transform t8 = transform_create();
				transform_set_rotation(t8, glm::angleAxis(glm::radians(45.f), glm::vec3(0.f, 0.f, 0.f)));
				transform_set_scale(t8, glm::vec3(0.25f, 0.5f, 1.0f));
				transform_set_position(t8, glm::vec3(1.3f, 0.6f, 1.0f));

				// In the future get the vector from the shader name and push the material name, same with the transforms
				std::string testmat_name = "character1";
				std::string testmat2_name = "background";
				std::string testmat3_name = "character2";
				std::string testmat4_name = "transparency";
				std::string testmat5_name = "spritesheet";

				// TODO: Shader use is implicit on the insertion order, give the shader map already inserted in the correct order wanted by the user
				packet.sprite_definitions.insert({ material_system_adquire(testmat_name)->shader->name, {} });
				sprite_definition sd;
				sd.material_name = testmat_name;
				sd.transform = t1;
				sd.z_order = 1;
				sd.texture_region = texture_system_calculate_custom_region_coordinates(*material_system_adquire(testmat_name)->diffuse_texture, { 258.0f, 1644.0f }, { 873.0f, 2096.0f });
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);

				sd.material_name = testmat_name;
				sd.transform = t3;
				sd.z_order = 1;
				sd.texture_region = texture_system_calculate_custom_region_coordinates(*material_system_adquire(testmat_name)->diffuse_texture, { 0.0f, 0.0f }, { 0.0f, 0.0f });
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);

				sd.material_name = testmat2_name;
				sd.transform = t4;
				sd.z_order = 0;
				sd.texture_region = texture_system_calculate_custom_region_coordinates(*material_system_adquire(testmat2_name)->diffuse_texture, { 0.0f, 0.0f }, { 0.0f, 0.0f });
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);

				sd.material_name = testmat3_name;
				sd.transform = t2;
				sd.z_order = 1;
				sd.texture_region = texture_system_calculate_custom_region_coordinates(*material_system_adquire(testmat3_name)->diffuse_texture, { 0.0f, 0.0f }, { 0.0f, 0.0f });
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);

				sd.material_name = testmat4_name;
				sd.transform = t5;
				sd.z_order = 2;
				sd.texture_region = texture_system_calculate_custom_region_coordinates(*material_system_adquire(testmat4_name)->diffuse_texture, { 0.0f, 0.0f }, { 0.0f, 0.0f });
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);

				sd.material_name = testmat4_name;
				sd.transform = t6;
				sd.z_order = 2;
				sd.texture_region = texture_system_calculate_custom_region_coordinates(*material_system_adquire(testmat4_name)->diffuse_texture, { 0.0f, 0.0f }, { 0.0f, 0.0f });
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);

				sd.material_name = testmat5_name;
				sd.transform = t7;
				sd.z_order = 2;
				sd.texture_region = texture_system_calculate_grid_region_coordinates(*material_system_adquire(testmat5_name)->diffuse_texture, {32.0f, 48.0f}, 0, 0);
				packet.sprite_definitions.at(material_system_adquire(testmat_name)->shader->name).push(sd);


				sprite_frame f = sprite_animation_system_acquire_frame(std::string("spritesheet_animation"), delta_time);
				sd.material_name = f.material_name;
				sd.transform = t8;
				sd.z_order = 2;
				sd.texture_region = f.texture_region;
				packet.sprite_definitions.at(material_system_adquire(f.material_name)->shader->name).push(sd);
				// TODO: TEMP CODE

				if (!renderer_draw_frame(packet)) {
					CE_LOG_FATAL("Failed to render frame");
					return false;
				}

				input_system_update_inputs(delta_time);
			}
		}

		state_ptr->program_config.reset();
		state_ptr.reset();

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