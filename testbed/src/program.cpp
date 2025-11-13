#include "program.h"
#include <cepch.h>
#include <core/logger.h>
#include <core/input.h>
#include <core/event.h>

#include <renderer/camera.h>
#include <systems/camera_system.h>
#include <math/transform.h>
#include <glm/glm.hpp>

#include <systems/ecs_system.h>

#include <systems/texture_system.h>
#include <systems/material_system.h>
#include <systems/sprite_animation_system.h>
#include <systems/audio_system.h>
#include <systems/scene_system.h>
#include <systems/ui_system.h>
#include <platform/file_system.h>
#include <resources/resources_types.inl>
#include <core/cestring.h>

#include <any>
#include <array>
#include <vector>

void initialize_sounds() {
	caliope::sound_emmiter_component test_sound_emmiter;
	caliope::sound_emmiter_component test_music_steeam_emmiter;
	
	test_sound_emmiter.id = caliope::audio_system_create_emmiter(std::string("test_short_sound"));
	test_music_steeam_emmiter.id = caliope::audio_system_create_emmiter(std::string("test"));
	//caliope::audio_system_play_emmiter(test_music_steeam_emmiter.id, 0);
}

bool on_entity_hover(caliope::event_system_code code, std::any data) {
	uint* id = std::any_cast<uint*>(data);
	//CE_LOG_INFO("%d", id);

	return true;
}

bool initialize_testbed(caliope::game_state& game_state) {
	CE_LOG_INFO("Initialize testbed");

	game_state.world_camera = caliope::camera_system_get_default_world();
	game_state.ui_camera = caliope::camera_system_get_default_ui();

	caliope::texture_system_adquire(std::string("B_witch_idle"));
	caliope::texture_system_change_filter(std::string("B_witch_idle"), caliope::FILTER_NEAREST, caliope::FILTER_NEAREST);


	caliope::scene_system_load(std::string("scene_test1"), true);
	caliope::scene_system_save(std::string("scene_test1"));
	caliope::scene_system_load(std::string("scene_test2"), true);
	//caliope::scene_system_create_empty(std::string("scene_test1"), true);
	//caliope::scene_system_create_empty(std::string("scene_test2"), true);

	caliope::event_register(caliope::EVENT_CODE_ON_ENTITY_HOVER, on_entity_hover);


	caliope::ui_system_create_empty_layout(std::string("ui_layout_test"), true);

	caliope::transform_component t;
	t.position = glm::vec3(0.0f, 0.0f, 0.0f);
	t.scale = glm::vec3(0.5f, 0.5f, 0.5f);
	t.roll_rotation = 0.0f;
	caliope::ui_material_component sc;
	sc.material_name = { "ui_image_test" };
	sc.texture_region[0] = { 0.0f, 0.0f };
	sc.texture_region[1] = { 0.0f, 0.0f };
	caliope::ui_system_instance_image(std::string("ui_layout_test"),t,sc);

	initialize_sounds();

	return true;
}

bool update_testbed(caliope::game_state& game_state, float delta_time) {

	float speed = 5.01f;

	if (caliope::is_key_pressed(caliope::KEY_A)) {
		caliope::camera_move_left(*game_state.world_camera, speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_D)) {
		caliope::camera_move_right(*game_state.world_camera, speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_W)) {
		caliope::camera_move_up(*game_state.world_camera, speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_S)) {
		caliope::camera_move_down(*game_state.world_camera, speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_Q)) {
		caliope::camera_zoom_set(*game_state.world_camera, caliope::camera_zoom_get(*game_state.world_camera) + speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_E)) {
		caliope::camera_zoom_set(*game_state.world_camera, caliope::camera_zoom_get(*game_state.world_camera) - speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_X)) {
		caliope::camera_roll(*game_state.world_camera, speed * delta_time);
	}

	if (caliope::is_key_pressed(caliope::KEY_1)) {
		caliope::scene_system_load(std::string("scene_test1"), true);
	}

	if (caliope::is_key_pressed(caliope::KEY_2)) {
		caliope::scene_system_load(std::string("scene_test2"), true);
	}

	if (caliope::is_key_pressed(caliope::KEY_4)) {
		caliope::scene_system_unload(std::string("scene_test1"));
		caliope::scene_system_unload(std::string("scene_test2"));
	}

	if (caliope::is_key_pressed(caliope::KEY_5)) {
		caliope::scene_system_enable(std::string("scene_test1"), true);
	}

	if (caliope::is_key_pressed(caliope::KEY_6)) {
		caliope::scene_system_enable(std::string("scene_test1"), false);
	}

	if (caliope::is_key_pressed(caliope::KEY_P)) {
		caliope::scene_system_destroy_entity(std::string("scene_test1"), 7);
		caliope::scene_system_destroy_entity(std::string("scene_test1"), 3);
		caliope::scene_system_destroy_entity(std::string("scene_test1"), 3333);
		caliope::audio_system_pause_emmiter(0, 0);
		caliope::audio_system_destroy_emmiter(0);
		uint x = caliope::audio_system_create_emmiter(std::string("test_short_sound"));
	}


	if (caliope::is_key_pressed(caliope::KEY_L)) {
		caliope::ecs_system_enable_entity(7, false);
		caliope::ecs_system_enable_entity(3, false);
		caliope::ecs_system_enable_entity(3333, false);
	}

	if (caliope::is_key_pressed(caliope::KEY_K)) {
		caliope::ecs_system_enable_entity(7, true);
		caliope::ecs_system_enable_entity(3, true);
		caliope::ecs_system_enable_entity(3333, true);
	}

	if (caliope::is_key_pressed(caliope::KEY_O)) {
		caliope::audio_system_loop_emmiter(0, true);
		caliope::audio_system_set_emmiter_gain(0, 0.3f);
		caliope::audio_system_play_emmiter(0, 0);
	}

	return true;
}

bool resize_testbed() {
	return true;
}

void shutdown_testbed() {
}
