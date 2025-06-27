#pragma once

#include "defines.h"
#include "loaders/resources_types.inl"

#include <string>


namespace caliope {

	struct transform;
	struct camera;

	typedef enum renderer_backend_type {
		BACKEND_TYPE_VULKAN,
		BACKEND_TYPE_DX
	} renderer_backend_type;

	typedef struct renderer_backend {

		bool (*initialize)(const std::string& application_name);
		void (*shutdown)();
		void (*resize)(uint16 width, uint16 height);

		bool (*begin_frame)(float delta_time);
		bool (*end_frame)(float delta_time);

		bool (*begin_renderpass)();
		bool (*end_renderpass)();

		void (*set_and_apply_uniforms)(std::shared_ptr<material>& m, glm::mat4& model, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

		void (*draw_geometry)();

		void (*texture_create)(texture& t, uchar* pixels);
		void (*texture_destroy)(texture& t);

		void (*shader_create)(shader& s);
		void (*shader_destroy)(shader& s);
		void (*shader_use)(shader& s);
	};

	typedef struct renderer_packet {
		float delta_time;
		std::shared_ptr<camera> world_camera;
		std::unordered_map<std::string, std::vector<transform>> quad_definitions; // Key : material name, Value: vector of transforms
	} renderer_packet;
}