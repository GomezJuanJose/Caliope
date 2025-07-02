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


	typedef struct renderer_backend_config {
		std::string application_name;
		uint max_quads;
	}renderer_backend_config;

	typedef struct renderer_backend {

		bool (*initialize)(const renderer_backend_config& config);
		void (*shutdown)();
		void (*resize)(uint16 width, uint16 height);

		bool (*begin_frame)(float delta_time);
		bool (*end_frame)(float delta_time);

		bool (*begin_renderpass)();
		bool (*end_renderpass)();

		void (*set_and_apply_uniforms)(std::vector<quad_properties>& quads, std::any& shader_internal_data, uint number_quads, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

		void (*draw_geometry)(uint quad_count);

		void (*texture_create)(texture& t, uchar* pixels);
		void (*texture_destroy)(texture& t);

		void (*shader_create)(shader& s);
		void (*shader_destroy)(shader& s);
		void (*shader_use)(shader& s);
	};

	typedef struct renderer_packet {
		float delta_time;
		std::shared_ptr<camera> world_camera;
		std::unordered_map<std::string, std::vector<std::string>> quad_materials; // Key : shader name, Value: vector of materials
		std::unordered_map<std::string, std::vector<transform>> quad_transforms; // Key : material name, Value: vector of transfors
	} renderer_packet;
}