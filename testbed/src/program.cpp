#include "program.h"

#include <core/logger.h>
#include <core/input.h>

#include <renderer/camera.h>
#include <systems/camera_system.h>
#include <glm/glm.hpp>

bool initialize_testbed(caliope::game_state& game_state) {
	CE_LOG_INFO("Initialize testbed");

	game_state.world_camera = caliope::camera_system_get_default();

	return true;
}

bool update_testbed(caliope::game_state& game_state, float delta_time) {

	if (caliope::is_key_pressed(caliope::KEY_A)) {
		caliope::camera_move_left(*game_state.world_camera, 0.01f);
	}

	if (caliope::is_key_pressed(caliope::KEY_D)) {
		caliope::camera_move_right(*game_state.world_camera, 0.01f);
	}

	if (caliope::is_key_pressed(caliope::KEY_W)) {
		caliope::camera_move_up(*game_state.world_camera, 0.01f);
	}

	if (caliope::is_key_pressed(caliope::KEY_S)) {
		caliope::camera_move_down(*game_state.world_camera, 0.01f);
	}

	if (caliope::is_key_pressed(caliope::KEY_Q)) {
		caliope::camera_zoom_set(*game_state.world_camera, caliope::camera_zoom_get(*game_state.world_camera) + 0.01f);
	}

	if (caliope::is_key_pressed(caliope::KEY_E)) {
		caliope::camera_zoom_set(*game_state.world_camera, caliope::camera_zoom_get(*game_state.world_camera) - 0.01f);
	}

	if (caliope::is_key_pressed(caliope::KEY_X)) {
		caliope::camera_roll(*game_state.world_camera, 0.01f);
	}

	return true;
}

bool resize_testbed() {
	return true;
}

void shutdown_testbed() {
}
