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
		std::array<char, MAX_NAME_LENGTH> material_name;
		uint z_order;
		std::array<glm::vec2, 2> texture_region;
	} material_component;

	typedef struct material_animation_component {
		std::array<char, MAX_NAME_LENGTH> animation_name;
		uint z_order;
	} material_animation_component;

	typedef struct sound_emmiter_component {
		uint id;
		bool loopeable;
		float volume;
		float pitch;
	} sound_emmiter_component;

	typedef struct point_light_component {
		glm::vec4 color;
		float radius;
		float constant;
		float linear;
		float quadratic;
	} point_light_component;

	typedef struct ui_material_component {
		std::array<char, MAX_NAME_LENGTH> material_name;
		std::array<glm::vec2, 2> texture_region;
	} ui_material_component;
}