#pragma once

#include "defines.h"
#include "cepch.h"
#include "loaders/resources_types.inl"


namespace caliope {

	struct transform;
	struct camera;
	struct renderpass;
	struct render_target;
	enum view_type;

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

		uint (*window_images_count_get)();
		uint(*window_image_index_get)();

		std::shared_ptr<std::any>(*window_attachment_get)(uint index);
		std::shared_ptr<std::any>(*depth_attachment_get)();

		bool (*render_target_create)(renderpass& pass, render_target& target);
		bool (*render_target_destroy)(render_target& target);

		bool (*renderpass_create)(renderpass& pass, glm::vec4 clear_color, float depth, uint stencil, bool has_prev_pass, bool has_next_pass);
		void (*renderpass_destroy)(renderpass& pass);
		bool (*renderpass_begin)(renderpass& pass, render_target& target);
		bool (*renderpass_end)();

		void (*set_and_apply_uniforms)(std::vector<quad_properties>& sprites, std::vector<point_light_definition>& point_lights, glm::vec4 ambient_color, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint number_quads, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

		void (*draw_geometry)(uint quad_count, geometry& geometry);

		void (*draw_object_pick)(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view);
		void (*show_picked_obj)();

		void (*texture_create)(texture& t, uchar* pixels);
		void (*texture_destroy)(texture& t);
		void (*texture_change_filter)(texture& t);

		bool (*shader_create)(shader_resource_data& shader_config, shader& out_shader, renderpass& pass);
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

	typedef struct render_target {
		std::vector<std::shared_ptr<std::any>> attachments;
		std::any internal_framebuffer;
	} render_target;

	typedef enum renderpass_type {
		RENDERPASS_TYPE_WORLD = 0,
		RENDERPASS_TYPE_OBJECT_PICK,
		RENDERPASS_TYPE_UI
	} renderpass_type;

	// NOTE: Aligned with vulkan types 
	typedef enum vertex_attribute_type {
		VERTEX_ATTRIBUTE_R32G32B32A32 = 109,
		VERTEX_ATTRIBUTE_R32G32B32 = 106,
		VERTEX_ATTRIBUTE_R32G32 = 103
	} vertex_attribute_type;

	typedef struct vertex_attribute_definition {
		vertex_attribute_type type;
		uint64 size;
	} vertex_attribute_definition;

	// NOTE: Aligned with vulkan types 
	typedef enum descriptor_type {
		DESCRIPTOR_TYPE_IMAGE_SAMPLER = 1,
		DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
		DESCRIPTOR_TYPE_STORAGE_BUFFER = 7
	} descriptor_type;

	// NOTE: Aligned with vulkan types 
	typedef enum descriptor_stage {
		DESCRIPTOR_STAGE_VERTEX = 1,
		DESCRIPTOR_STAGE_FRAGMENT = 16,
	} descriptor_stage;

	typedef struct descriptor_definition {
		descriptor_type type;
		uint count;
		descriptor_stage stage;
	} descriptor_definition;

	// NOTE: Aligned with vulkan types 
	typedef enum descriptor_buffer_usage {
		DESCRIPTOR_BUFFER_USAGE_UNIFORM = 16,
		DESCRIPTOR_BUFFER_USAGE_STORAGE = 32
	} descriptor_buffer_usage;

	typedef struct descriptor_buffer_definition {
		descriptor_buffer_usage usage;
		uint64 size;
	} descriptor_buffer_definition;

	typedef enum renderpass_clear_flag {
		RENDERPASS_CLEAR_FLAG_NONE = 0x0,
		RENDERPASS_CLEAR_FLAG_COLOR_BUFFER = 0x1,
		RENDERPASS_CLEAR_FLAG_DEPTH_BUFFER = 0x2,
	} renderpass_clear_flag;

	typedef struct renderpass {
		renderpass_type type;
		renderpass_clear_flag flags;

		glm::vec4 render_area;
		glm::vec4 clear_color;
		float depth;
		uint stencil;
		bool has_prev_pass;
		bool has_next_pass;

		std::vector<render_target> targets;
		std::any internal_data;
	}renderpass;

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
		renderpass_type renderpass;
		std::any internal_config;

		void (*on_create)(render_view& self);
		void (*on_destroy)(render_view& self);
		void (*on_resize_window)(render_view& self, uint width, uint height);
		bool (*on_build_package)(render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data);
		bool (*on_render)(render_view& self, std::any& packet, uint render_target_index);
	} render_view;

	typedef struct render_view_world_config {
		uint window_width;
		uint window_height;
		uint max_number_quads;
		uint max_textures_per_batch;
		float aspect_ratio;
	} render_view_world_config;
}