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

	typedef struct parent_component {
		uint parent;
	} parent_component;


	typedef enum ui_anchor_position {
		UI_ANCHOR_TOP_LEFT = 0,
		UI_ANCHOR_TOP_CENTER,
		UI_ANCHOR_TOP_RIGHT,
		UI_ANCHOR_CENTER_LEFT,
		UI_ANCHOR_CENTER,
		UI_ANCHOR_CENTER_RIGHT,
		UI_ANCHOR_BOTTOM_LEFT,
		UI_ANCHOR_BOTTOM_CENTER,
		UI_ANCHOR_BOTTOM_RIGHT,

	} ui_anchor_position;

	typedef struct ui_transform_component {
		glm::vec3 position;
		glm::vec2 bounds_max_point; // NOTE: Assumes that the min point is 0,0
		glm::vec2 bounds_offset; // NOTE: Goes from 0 to 1
		float roll_rotation;
		ui_anchor_position anchor;
	} ui_transform_component;

	typedef struct ui_material_component {
		std::array<char, MAX_NAME_LENGTH> material_name;
		std::array<glm::vec2, 2> texture_region;
	} ui_material_component;

	typedef struct ui_dynamic_material_component {
		std::array<char, MAX_NAME_LENGTH> normal_texture;
		std::array<char, MAX_NAME_LENGTH> hover_texture;
		std::array<char, MAX_NAME_LENGTH> pressed_texture;
		glm::vec4 normal_color;
		glm::vec4 hover_color;
		glm::vec4 pressed_color;

		std::array<char, MAX_NAME_LENGTH> material_name;
		std::array<glm::vec2, 2> texture_region;
	} ui_dynamic_material_component;

	enum event_system_code;
	typedef struct ui_events_component {
		bool(*on_ui_hover)(event_system_code code, std::any data);
		bool(*on_ui_unhover)(event_system_code code, std::any data);
		bool(*on_ui_pressed)(event_system_code code, std::any data);
		bool(*on_ui_released)(event_system_code code, std::any data);
	} ui_events_component;

	typedef struct ui_text_component {
		std::array<char, 2048> text; // TODO: Think what to do with this because with std::String does not function as expected
		std::array<char, MAX_NAME_LENGTH> style_table_name;
	}ui_text_component;

	typedef enum ui_visibility {
		UI_VISIBILITY_VISIBLE = 0,
		UI_VISIBILITY_VISIBLE_NO_HIT, // The cursor pick will ignore this element
		UI_VISIBILITY_COLLAPSE
	} ui_visibility;

	typedef struct ui_behaviour_component {
		ui_visibility visibility;
	}ui_behaviour_component;
}