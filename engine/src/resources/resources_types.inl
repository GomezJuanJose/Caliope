#pragma once
#include "defines.h"
#include "math/transform.h"
#include "systems/ecs_system.h"// TODO: Move archetypes to other place, avoid to include the whole system
#include "components/components.inl"

#include <glm/glm.hpp>
#include <vendors/stb_truetype/stb_truetype.h>
#include <string>

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

	typedef enum resource_type{
		RESOURCE_TYPE_BINARY = 0,
		RESOURCE_TYPE_IMAGE,
		RESOURCE_TYPE_SHADER,
		RESOURCE_TYPE_MATERIAL,
		RESOURCE_TYPE_SPRITE_ANIMATION,
		RESOURCE_TYPE_AUDIO,
		RESOURCE_TYPE_SCENE,
		RESOURCE_TYPE_UI_LAYOUT,
		RESOURCE_TYPE_TEXT_FONT,
		RESOURCE_TYPE_TEXT_STYLE

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
		std::vector<uint> entity_ids;
		std::vector<archetype> archetypes;
		std::vector<std::vector<component_id>> components;
		std::vector< std::vector<std::vector<component_data_type>>> components_data_types; // Note: entity_id < component_id < data_type > > >
		std::vector<std::vector<void*>> components_data;

		std::unordered_map<component_id, uint> components_sizes;
	}scene_resource_data;

	typedef struct text_font_resource_data {
		std::array<char, MAX_NAME_LENGTH> name;
		std::vector<uchar> binary_data;
		stbtt_fontinfo stb_font_info;
	} text_font_resource_data;

	typedef struct text_style_resource_data {
		std::array<char, MAX_NAME_LENGTH> name;

		std::vector<std::array<char, MAX_NAME_LENGTH>> style_tag_names;
		std::vector<std::array<char, MAX_NAME_LENGTH>> text_fonts;
		std::vector<uint> font_sizes;
		std::vector<glm::vec4> text_colors;
		std::vector<float> additional_interline_spaces;

		std::vector<std::array<char, MAX_NAME_LENGTH>> image_tag_names;
		std::vector<std::array<char, MAX_NAME_LENGTH>> image_materials;
		std::vector<glm::vec2> image_sizes;
		std::vector<glm::vec4> texture_coordinates;
	} text_style_resource_data;




	typedef enum texture_filter {
		FILTER_LINEAR,
		FILTER_NEAREST
	} texture_filter;

	typedef struct texture {
		std::string name;
		uint normal_render_batch_index; // Position that occupies into the render world batch texture array
		uint pick_render_batch_index; // Position that occupies into the render ui batch texture array
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


	typedef struct quad_instance_definition {
		uint id;
		uint z_order;
		transform transform;
		std::array<glm::vec2,4> texture_region;
	
		glm::vec3 diffuse_color;
		float shininess_intensity;
		float shininess_sharpness;
		shader* shader;
		texture* diffuse_texture;
		texture* specular_texture;
		texture* normal_texture;

	} quad_instance_definition;

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


	typedef struct text_font_glyph {
		uint codepoint;
		uint kerning_index;
		uint16 x;
		uint16 y;
		uint16 width;
		uint16 height;
		int16 x_offset;
		int16 y_offset;
		int16 y_offset2;
		int16 x_advance;

	}text_font_glyph;

	typedef struct text_font_kerning {
		uint codepoint1;
		uint codepoint2;
		int advance;
	} text_font_kerning;

	typedef struct text_font {
		std::string name;

		glm::vec2 atlas_size;
		material* atlas_material;

		std::vector<int> codepoints;
		std::vector<text_font_glyph> glyphs;
		std::vector<text_font_kerning> kernings;

		int line_height;
		int x_advance_space;
		int x_advance_tab;

	} text_font;

	typedef struct text_style_table {
		std::string name;

		std::unordered_map<std::string,uint> tag_style_indexes;
		std::vector<text_font*> fonts;
		std::vector <uint> text_sizes;
		std::vector <glm::vec4> text_colors;
		std::vector<float> additional_interline_spaces;

		std::unordered_map<std::string, uint> tag_image_indexes;
		std::vector<material*> materials;
		std::vector<glm::vec2> image_sizes;
		std::vector<std::array<glm::vec2, 2>> texture_coordinates;
	};
}