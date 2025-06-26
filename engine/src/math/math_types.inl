#pragma once
#include "defines.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace caliope {
	typedef struct vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec4 tangent;
		glm::vec2 tex_coord;
	} vertex;

	typedef struct transform {
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		glm::mat4 local;

		bool is_dirty;

		std::shared_ptr<transform> parent;
	};
}