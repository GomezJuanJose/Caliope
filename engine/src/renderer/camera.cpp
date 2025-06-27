#include "camera.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace caliope {
	camera camera_create() {
		camera c;
		camera_reset(c);
		return c;
	}

	void camera_reset(camera& c) {
		c.position = glm::vec3(0.0f, 0.0f, 5.0f);
		c.rotation = 0.0f;
		c.zoom = 1.0f;
		c.view = camera_view_get(c);
		c.projection  = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f);
		c.is_view_dirty = false;
		c.is_projection_dirty = false;
	}

	glm::vec3 camera_position_get(const camera& c) {
		return c.position;
	}

	void camera_position_set(camera& c, glm::vec3 position) {
		c.position = position;
		c.is_view_dirty = true;
	}

	float camera_rotation_get(const camera& c) {
		return c.rotation;
	}

	void camera_rotation_set(camera& c, float rotation) {
		c.rotation = 0.0f;
		c.is_view_dirty = true;
	}

	float camera_zoom_get(const camera& c) {
		return c.zoom;
	}

	void camera_zoom_set(camera& c, float zoom) {
		c.zoom = zoom;
		c.is_projection_dirty = true;
	}

	glm::mat4 camera_view_get(camera& c) {
		
		if (c.is_view_dirty) {
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), c.rotation, glm::vec3(0, 0, 1));
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), c.position);

			c.view = glm::inverse(translation * rotation);

			c.is_view_dirty = false;
		}

		return c.view;
	}

	glm::mat4 camera_projection_get(camera& c) {
		
		if (c.is_projection_dirty) {
			c.projection = glm::ortho(-1.0f * c.zoom, 1.0f * c.zoom, -c.zoom, c.zoom, -100.0f, 100.0f);
			
			c.is_projection_dirty = false;
		}

		return c.projection;
	}

	void camera_move_left(camera& c, float amount) {
		c.position.x -= amount;
		c.is_view_dirty = true;
	}

	void camera_move_right(camera& c, float amount) {
		c.position.x += amount;
		c.is_view_dirty = true;
	}

	void camera_move_up(camera& c, float amount) {
		c.position.y += amount;
		c.is_view_dirty = true;
	}

	void camera_move_down(camera& c, float amount) {
		c.position.y -= amount;
		c.is_view_dirty = true;
	}

	void camera_roll(camera& c, float amount) {
		c.rotation += amount;
	}
}