#pragma once
#include "defines.h"
#include "math/transform.h"
#include "systems/ecs_system.h"// TODO: Move archetypes to other place, avoid to include the whole system
#include "components/components.inl"

#include <glm/glm.hpp>

namespace caliope {
	struct audio_file_internal;
	struct renderpass;
	struct vertex_attribute_definition;
	struct descriptor_definition;
	struct descriptor_buffer_definition;
	struct sprite_frame;

	enum scene_state;
	enum renderpass_type;
	enum attachment_format_type;
	enum renderpass_stage_type;
	enum renderpass_access_type;

	#define MAX_NAME_LENGTH 256

	typedef enum resource_type{
		RESOURCE_TYPE_BINARY = 0,
		RESOURCE_TYPE_IMAGE,
		RESOURCE_TYPE_SHADER,
		RESOURCE_TYPE_MATERIAL,
		RESOURCE_TYPE_SPRITE_ANIMATION,
		RESOURCE_TYPE_AUDIO,
		RESOURCE_TYPE_SCENE
	} resource_type;

	typedef struct resource {
		uint64 data_size;
		std::any data;
		std::string loader_name;
	} resource;

	typedef struct image_resource_data {
		uchar channel_count;
		uint width;
		uint height;
		uchar* pixels;
	}image_resource_data;

	typedef struct renderpass_resource_data {
		glm::vec4 clear_color;
		float depth;
		uint stencil;
		bool has_prev_pass;
		bool has_next_pass;

		std::vector<attachment_format_type> attachment_formats;

		int subpass_src_stage_mask;
		int subpass_dst_stage_mask;

		int subpass_src_access_mask;
		int subpass_dst_access_mask;
	} renderpass_resource_data;

	typedef struct shader_resource_data {
		std::string name;
		std::vector<uchar> vertex_code;
		uint64 vertex_code_size;
		std::vector<uchar> fragment_code;
		uint64 fragment_code_size;
		renderpass_type renderpass_type;

		std::vector<vertex_attribute_definition> vertex_attribute_definitions;
		std::vector<descriptor_definition> descriptor_definitions;
		std::vector<descriptor_buffer_definition> descriptor_buffer_definitions;
	} shader_resource_data;

	typedef struct material_resource_data {
		std::array<char, MAX_NAME_LENGTH> name;
		std::array<char, MAX_NAME_LENGTH> shader_name;
		uint renderpass_type;
		glm::vec3 diffuse_color;
		float shininess_intensity;
		float shininess_sharpness;
		std::array<char, MAX_NAME_LENGTH> diffuse_texture_name;
		std::array<char, MAX_NAME_LENGTH> specular_texture_name;
		std::array<char, MAX_NAME_LENGTH> normal_texture_name;
	}material_resource_data;

	typedef struct sprite_frame_resource_data {
		std::string material_name;
		glm::vec2 grid_size;
	}sprite_frame_resource_data;

	typedef struct sprite_animation_resource_data {
		std::array<char, MAX_NAME_LENGTH> name;
		bool is_looping;
		bool is_playing;
		float frames_per_second;
		uint number_of_rows;
		uint number_of_columns;
		uint starting_row;
		uint starting_column;
		std::vector<sprite_frame_resource_data> frames_data;
	}sprite_animation_resource_data;

	typedef enum audio_file_type {
		AUDIO_FILE_TYPE_SOUND_EFFECT,
		AUDIO_FILE_TYPE_MUSIC_STREAM
	}audio_file_type;

	typedef struct audio_clip_resource_data {
		audio_file_type type;

		uint format;
		int channels;
		uint sample_rate;
		uint total_samples_left;
		void* buffer;
		uint buffer_size;

	} audio_clip_resource_data;

	typedef struct scene_resource_data {
		std::array<char, MAX_NAME_LENGTH> name;
		std::vector<archetype> archetypes;
		std::vector<std::vector<component_id>> components;
		std::vector< std::vector<std::vector<component_data_type>>> components_data_types; // Note: entity_id < component_id < data_type > > >
		std::vector<std::vector<void*>> components_data;

		std::unordered_map<component_id, uint> components_sizes;
	}scene_resource_data;


	typedef enum texture_filter {
		FILTER_LINEAR,
		FILTER_NEAREST
	} texture_filter;

	typedef struct texture {
		std::string name;
		uint id;
		uint width;
		uint height;
		uint channel_count;
		texture_filter magnification_filter;
		texture_filter minification_filter;
		bool has_transparency;
		std::any internal_data;
	} texture;

	typedef struct shader {
		std::string name;
		std::any internal_data;
	} shader;

	typedef struct material {
		std::string name;
		glm::vec3 diffuse_color;
		float shininess_intensity;
		float shininess_sharpness;
		shader* shader;
		texture* diffuse_texture;
		texture* specular_texture;
		texture* normal_texture;
	}material;


	typedef struct sprite_frame {
		std::string material_name;
		std::array<glm::vec2, 4> texture_region;
	};

	typedef struct sprite_animation {
		std::string name;
		std::vector<sprite_frame> frames;
		bool is_looping;
		bool is_playing;
		float frames_per_second;
		float accumulated_delta;
		uint current_frame;
	} sprite_animation;

	// NOTE: Remember to always align the memory by 128 bits and always go from the higher size to the lowest
	typedef struct shader_world_quad_properties {
		alignas(16)	glm::mat4 model;
		alignas(8)	glm::vec3 diffuse_color;
		alignas(8)	std::array<glm::vec2, 4> texture_region;
		alignas(4)	uint id;
		alignas(4)	uint diffuse_index;
		alignas(4)	uint normal_index;
		alignas(4)	uint specular_index;
		alignas(4)	float shininess_intensity;
		alignas(4)	float shininess_sharpness;
	} shader_world_quad_properties;

	typedef struct shader_ui_quad_properties {
		alignas(16)	glm::mat4 model;
		alignas(8)	glm::vec3 diffuse_color;
		alignas(8)	std::array<glm::vec2, 4> texture_region;
		alignas(4)	uint id;
		alignas(4)	uint diffuse_index;
	} shader_ui_quad_properties;

	typedef struct shader_pick_quad_properties {
		alignas(16)	glm::mat4 model;
		alignas(8)	std::array<glm::vec2, 4> texture_region;
		alignas(4)	uint id;
		alignas(4)	uint diffuse_index;
	} shader_pick_quad_properties;


	typedef struct quad_definition {
		uint id;
		uint z_order;
		std::string material_name;
		transform transform;
		std::array<glm::vec2,4> texture_region;
	} quad_definition;

	typedef struct point_light_definition {
		glm::vec4 color;
		glm::vec4 position;
		float radius;
		float constant;
		float linear;
		float quadratic;
	}point_light_definition;

	typedef struct geometry{
		std::string name;
		uint index_count;
		std::any internal_data;
	} geometry;


	typedef struct scene {
		std::vector<uint> entities;
		std::array<char, MAX_NAME_LENGTH> name;
		bool is_enabled;
	} scene;
}