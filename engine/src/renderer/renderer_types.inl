#pragma once

#include "defines.h"
#include "cepch.h"
#include "loaders/resources_types.inl"


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
		uint max_textures_per_batch;
	}renderer_backend_config;

	typedef struct renderer_backend {

		bool (*initialize)(const renderer_backend_config& config);
		void (*stop)();
		void (*shutdown)();
		void (*resize)(uint16 width, uint16 height);

		bool (*begin_frame)(float delta_time);
		bool (*end_frame)(float delta_time);

		bool (*begin_renderpass)();
		bool (*end_renderpass)();

		void (*set_and_apply_uniforms)(std::vector<sprite_properties>& sprites, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint number_quads, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

		void (*draw_geometry)(uint quad_count, geometry& geometry);

		void (*texture_create)(texture& t, uchar* pixels);
		void (*texture_destroy)(texture& t);
		void (*texture_change_filter)(texture& t);

		void (*shader_create)(shader& s);
		void (*shader_destroy)(shader& s);
		void (*shader_use)(shader& s);

		void (*geometry_create)(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices);
		void (*geometry_destroy)(geometry& geometry);
	};

	struct z_order_comparator {
		bool operator()(const sprite_definition& a, const sprite_definition& b) {
			return a.z_order > b.z_order;
		};
	};

	typedef struct renderer_packet {
		float delta_time;
		camera* world_camera;
		//std::unordered_map<std::string, std::vector<std::string>> quad_materials; // Key : shader name, Value: vector of materials
		//std::unordered_map<std::string, std::vector<transform>> quad_transforms; // Key : material name, Value: vector of transfors

		std::unordered_map <std::string, std::priority_queue<sprite_definition, std::vector<sprite_definition>, z_order_comparator>> sprite_definitions; // Key : shader name, Value: vector of materials

	} renderer_packet;
}