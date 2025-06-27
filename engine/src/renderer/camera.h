#pragma once

#include "defines.h"
#include <glm/glm.hpp>

namespace caliope {


	typedef struct camera {
		glm::vec3 position;
		float rotation;
		float zoom;

		glm::mat4 view;
		glm::mat4 projection;

		bool is_view_dirty;
		bool is_projection_dirty;
	} camera;

	camera camera_create();
	void camera_reset(camera& c);

	CE_API glm::vec3 camera_position_get(const camera& c);
	CE_API void camera_position_set(camera& c, glm::vec3 position);

	CE_API float camera_rotation_get(const camera& c);
	CE_API void camera_rotation_set(camera& c, float rotation);

	CE_API float camera_zoom_get(const camera& c);
	CE_API void camera_zoom_set(camera& c, float zoom);

	glm::mat4 camera_view_get(camera& c);
	glm::mat4 camera_projection_get(camera& c);

	CE_API void camera_move_left(camera& c, float amount);
	CE_API void camera_move_right(camera& c, float amount);
	CE_API void camera_move_up(camera& c, float amount);
	CE_API void camera_move_down(camera& c, float amount);

	CE_API void camera_roll(camera& c, float amount);
}