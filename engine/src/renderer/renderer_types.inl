#pragma once

#include "defines.h"
#include "cepch.h"
#include "loaders/resources_types.inl"


namespace caliope {

	struct transform;
	struct camera;
	enum view_type;

	typedef enum renderer_backend_type {
		BACKEND_TYPE_VULKAN,
		BACKEND_TYPE_DX
	} renderer_backend_type;

	typedef struct renderpass {

		std::any internal_data;
	}renderpass;


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

		void (*set_and_apply_uniforms)(std::vector<quad_properties>& sprites, std::vector<point_light_definition>& point_lights, glm::vec4 ambient_color, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint number_quads, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

		void (*draw_geometry)(uint quad_count, geometry& geometry);

		void (*draw_object_pick)(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view);
		void (*show_picked_obj)();

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
		bool operator()(const quad_definition& a, const quad_definition& b) {
			return a.z_order > b.z_order;
		};
	};


	typedef struct renderer_view_packet {
		view_type view_type;
		std::any view_packet;
	} renderer_view_packet;

	typedef struct render_view_world_packet {
		float delta_time;
		camera* world_camera;
		glm::vec4 ambient_color;
		std::vector<point_light_definition> point_light_definitions;
		std::unordered_map <std::string, std::priority_queue<quad_definition, std::vector<quad_definition>, z_order_comparator>> sprite_definitions; // Key : shader name, Value: vector of materials
	};

	typedef struct render_view_object_pick_packet {

	}render_view_object_pick_packet;

	typedef struct render_view {
		std::string name;
		view_type type;
		std::any internal_data;

		void (*on_create)(render_view& self);
		void (*on_destroy)(render_view& self);
		void (*on_resize_window)(render_view& self, uint width, uint height);
		bool (*on_build_package)(render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data);
		bool (*on_render)(render_view& self, std::any& packet);
	} render_view;

	typedef struct render_view_world_internal_data {
		uint window_width;
		uint window_height;
		uint max_number_quads;
		uint max_textures_per_batch;
	} render_view_world_internal_data;
}