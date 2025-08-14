#pragma once
#include "defines.h"
#include <glm/glm.hpp>


namespace caliope {
	typedef struct transform_component {
		glm::vec3 position;
		glm::vec3 scale;
		float roll_rotation;
	} transform_component;

#define MAX_NAME_LENGTH 255

	typedef struct material_component {
		std::array<glm::vec2, 2> texture_region;
		uint z_order;
		std::array<char, MAX_NAME_LENGTH> material_name;
	} material_component;

	typedef struct material_animation_component {
		uint z_order;
		std::array<char, MAX_NAME_LENGTH> animation_name;
	} material_animation_component;
}