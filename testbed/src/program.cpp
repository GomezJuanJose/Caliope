#include "program.h"
#include <cepch.h>
#include <core/logger.h>
#include <core/input.h>

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
#include <platform/file_system.h>
#include <loaders/resources_types.inl>

#include <any>
#include <array>
#include <vector>

void intialize_sprite_entities() {

	// TODO: TEMPORAL CODE
	caliope::material_resource_data m;
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

	caliope::material_resource_data m2;
	m2.diffuse_color = glm::vec3(0.5f, 1.0f, 1.0f);
	m2.shininess_sharpness = 15.0f;
	m2.shininess_intensity = 5.0f;
	m2.diffuse_texture_name = { "cottageEXTday" };
	m2.specular_texture_name = { "cottageSpecular" };
	m2.normal_texture_name = { "cottageNormal" };
	m2.shader_name = { "Builtin.SpriteShader" };
	m2.name = { "background" };

	caliope::material_resource_data m3;
	m3.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
	m3.shininess_sharpness = 64.0f;
	m3.shininess_intensity = 5.0f;
	m3.diffuse_texture_name = { "warrior_human_woman_06" };
	m3.specular_texture_name = { "" };
	m3.normal_texture_name = { "" };
	m3.shader_name = { "Builtin.SpriteShader" };
	m3.name = { "character2" };

	caliope::material_resource_data m4;
	m4.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
	m4.shininess_sharpness = 64.0f;
	m4.shininess_intensity = 5.0f;
	m4.diffuse_texture_name = { "map_01" };
	m4.specular_texture_name = { "" };
	m4.normal_texture_name = { "" };
	m4.shader_name = { "Builtin.SpriteShader" };
	m4.name = { "transparency" };

	caliope::material_resource_data m5;
	m5.diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
	m5.shininess_sharpness = 64.0f;
	m5.shininess_intensity = 5.0f;
	m5.diffuse_texture_name = { "B_witch_idle" };
	m5.specular_texture_name = { "" };
	m5.normal_texture_name = { "" };
	m5.shader_name = { "Builtin.SpriteShader" };
	m5.name = { "spritesheet" };

	caliope::texture_system_adquire(std::string("B_witch_idle"));
	caliope::texture_system_change_filter(std::string("B_witch_idle"), caliope::FILTER_NEAREST, caliope::FILTER_NEAREST);


	caliope::file_handle w;
	caliope::file_system_open(std::string("assets\\materials\\character1.cemat"), caliope::FILE_MODE_WRITE, w);
	caliope::file_system_write_bytes(w, sizeof(caliope::material_resource_data), &m);
	caliope::file_system_close(w);

	caliope::file_system_open(std::string("assets\\materials\\background.cemat"), caliope::FILE_MODE_WRITE, w);
	caliope::file_system_write_bytes(w, sizeof(caliope::material_resource_data), &m2);
	caliope::file_system_close(w);

	caliope::file_system_open(std::string("assets\\materials\\character2.cemat"), caliope::FILE_MODE_WRITE, w);
	caliope::file_system_write_bytes(w, sizeof(caliope::material_resource_data), &m3);
	caliope::file_system_close(w);

	caliope::file_system_open(std::string("assets\\materials\\transparency.cemat"), caliope::FILE_MODE_WRITE, w);
	caliope::file_system_write_bytes(w, sizeof(caliope::material_resource_data), &m4);
	caliope::file_system_close(w);

	caliope::file_system_open(std::string("assets\\materials\\spritesheet.cemat"), caliope::FILE_MODE_WRITE, w);
	caliope::file_system_write_bytes(w, sizeof(caliope::material_resource_data), &m5);
	caliope::file_system_close(w);

	caliope::sprite_animation_config spritesheet_animation;
	spritesheet_animation.name = "spritesheet_animation";
	std::vector<caliope::sprite_frame> frames;
	for (uint i = 0; i < 5; ++i) {
		caliope::sprite_frame frame;
		frame.material_name = "spritesheet";
		frame.texture_region = caliope::texture_system_calculate_grid_region_coordinates(*caliope::material_system_adquire(std::string("spritesheet"))->diffuse_texture, { 32.0f, 48.0f }, i, 0);
		frames.push_back(frame);
	}
	spritesheet_animation.frames = frames;
	spritesheet_animation.is_looping = true;
	spritesheet_animation.is_playing = true;
	spritesheet_animation.frames_per_second = 2.0f;

	caliope::sprite_animation_system_register(spritesheet_animation);


	caliope::transform_component t1;
	t1.position = glm::vec3(1.0f, 0.0f, 0.0f);
	t1.scale = glm::vec3(1.0f, 0.5f, 1.0f);
	t1.roll_rotation = 0.0f;


	caliope::transform_component t2;
	t2.position = glm::vec3(0.0f, 0.0f, 0.0f);
	t2.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	t2.roll_rotation = 0.0f;


	caliope::transform_component t3;
	t3.position = glm::vec3(-1.0f, 0.0f, 0.0f);
	t3.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	t3.roll_rotation = 45.0f;


	caliope::transform_component t4;
	t4.position = glm::vec3(0.0f, 0.0f, 0.0f);
	t4.scale = glm::vec3(5.0f, 3.0f, 1.0f);
	t4.roll_rotation = 0.0f;


	caliope::transform_component t5;
	t5.position = glm::vec3(0.0f, 0.0f, 0.0f);
	t5.scale = glm::vec3(0.5f, 0.5f, 1.0f);
	t5.roll_rotation = 0.0f;


	caliope::transform_component t6;
	t6.position = glm::vec3(0.2f, 0.0f, 0.0f);
	t6.scale = glm::vec3(0.5f, 0.5f, 1.0f);
	t6.roll_rotation = 0.0f;


	caliope::transform_component t7;
	t7.position = glm::vec3(1.0f, 0.6f, 0.0f);
	t7.scale = glm::vec3(0.25f, 0.5f, 1.0f);
	t7.roll_rotation = 0.0f;


	caliope::transform_component t8;
	t8.position = glm::vec3(1.3f, 0.6f, 0.0f);
	t8.scale = glm::vec3(0.25f, 0.5f, 1.0f);
	t8.roll_rotation = 0.0f;


	std::vector<caliope::transform_component> transforms = { t1, t3, t4, t2, t5, t6, t7, t8 };

	caliope::material_component sc1;
	sc1.material_name = { "character1" };
	sc1.z_order = 1;
	sc1.texture_region[0] = { 258.0f, 1644.0f };
	sc1.texture_region[1] = {873.0f, 2096.0f};

	caliope::material_component sc2;
	sc2.material_name = { "character1" };
	sc2.z_order = 1;
	sc2.texture_region[0] = { 0.0f, 0.0f };
	sc2.texture_region[1] = { 0.0f, 0.0f };

	caliope::material_component sc3;
	sc3.material_name = { "background" };
	sc3.z_order = 0;
	sc3.texture_region[0] = { 0.0f, 0.0f };
	sc3.texture_region[1] = { 0.0f, 0.0f };

	caliope::material_component sc4;
	sc4.material_name = { "character2" };
	sc4.z_order = 1;
	sc4.texture_region[0] = { 0.0f, 0.0f };
	sc4.texture_region[1] = { 0.0f, 0.0f };

	caliope::material_component sc5;
	sc5.material_name = { "transparency" };
	sc5.z_order = 2;
	sc5.texture_region[0] = { 0.0f, 0.0f };
	sc5.texture_region[1] = { 0.0f, 0.0f };

	caliope::material_component sc6;
	sc6.material_name = { "transparency" };
	sc6.z_order = 2;
	sc6.texture_region[0] = { 0.0f, 0.0f };
	sc6.texture_region[1] = { 0.0f, 0.0f };

	caliope::material_component sc7;
	sc7.material_name = { "spritesheet" };
	sc7.z_order = 2;
	sc7.texture_region[0] = { 0.0f, 0.0f };
	sc7.texture_region[1] = { 32.0f, 48.0f };


	std::vector<caliope::material_component> sprites = { sc1, sc2, sc3, sc4, sc5, sc6, sc7};

	for (uint entity_index = 0; entity_index < 7; ++entity_index) {

		std::vector<caliope::component_id> components = { caliope::TRANSFORM_COMPONENT, caliope::MATERIAL_COMPONENT };
		std::vector<void*> data = { &transforms[entity_index] , &sprites[entity_index] };

		caliope::scene_system_instance_entity(std::string("scene_test1"), caliope::ARCHETYPE_SPRITE, 
			components,
			data
		);
	}


	caliope::material_animation_component sac1;
	sac1.animation_name = { "spritesheet_animation" };
	sac1.z_order = 2;
	std::vector<caliope::component_id> components = { caliope::TRANSFORM_COMPONENT, caliope::MATERIAL_ANIMATION_COMPONENT };
	std::vector<void*> data = { &t8, &sac1 };
	caliope::scene_system_instance_entity(std::string("scene_test1"), caliope::ARCHETYPE_SPRITE_ANIMATION,
		components,
		data
	);
}

void initialize_sounds() {
	caliope::sound_emmiter_component test_sound_emmiter;
	caliope::sound_emmiter_component test_music_steeam_emmiter;
	
	test_sound_emmiter.id = caliope::audio_system_create_emmiter(std::string("test_short_sound"));
	test_music_steeam_emmiter.id = caliope::audio_system_create_emmiter(std::string("test"));
	//caliope::audio_system_play_emmiter(test_music_steeam_emmiter.id, 0);
}

void initialize_lights() {
	caliope::transform_component transform;
	transform.position = { 0, 0, 1.0f };
	caliope::point_light_component light;
	light.color = {0.4f, 0.2f, 0.0f, 1.0f};
	light.radius = 10.0f;
	light.constant = 1.0f;
	light.linear = 0.35f;
	light.quadratic = 0.44f;
	std::vector<caliope::component_id> components = { caliope::TRANSFORM_COMPONENT, caliope::POINT_LIGHT_COMPONENT };
	std::vector<void*> data = { &transform, &light };
	caliope::scene_system_instance_entity(std::string("scene_test2"), caliope::ARCHETYPE_POINT_LIGHT,
		components,
		data
	);

	caliope::transform_component transform2;
	transform2.position = { 0.0f, -0.5f, 0.1f };
	caliope::point_light_component light2;
	light2.color = { 0.0f, 0.0f, 1.0f, 1.0f };
	light2.radius = 0.2f;
	light2.constant = 1.0f;
	light2.linear = 0.35f;
	light2.quadratic = 0.44f;
	data = { &transform2, &light2 };
	caliope::scene_system_instance_entity(std::string("scene_test2"), caliope::ARCHETYPE_POINT_LIGHT,
		components,
		data
	);
}

bool initialize_testbed(caliope::game_state& game_state) {
	CE_LOG_INFO("Initialize testbed");

	game_state.world_camera = caliope::camera_system_get_default();

	caliope::scene_system_create_empty(std::string("scene_test1"), true);
	caliope::scene_system_create_empty(std::string("scene_test2"), true);

	intialize_sprite_entities();
	initialize_lights();

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
