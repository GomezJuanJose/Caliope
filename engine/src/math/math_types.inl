#pragma once
#include "defines.h"

#include <glm/glm.hpp>

namespace caliope {
	typedef struct vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 tex_coord;
	} vertex;
}