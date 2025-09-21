#pragma once
#include "defines.h"
#include "math/transform.h"
#include "systems/ecs_system.h"// TODO: Move archetypes to other place, avoid to include the whole system
#include "components/components.inl"

#include <glm/glm.hpp>

namespace caliope {
	struct audio_file_internal;
	struct renderpass;

	enum scene_state;
	enum renderpass_type;

	#define MAX_NAME_LENGTH 256

	typedef enum resource_type{
		RESOURCE_TYPE_BINARY = 0,
		RESOURCE_TYPE_IMAGE,
		RESOURCE_TYPE_MATERIAL,
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


		/*uint entities_count;
		uint components_count;
		std::array<archetype, 50> entities_archetype; // Entities in the scene
		std::array<transform_component, 50> entities_transform; // The data of all components in the scene
		std::array<material_component, 50> entities_material; // The data of all components in the scene
		std::array<material_animation_component, 50> entities_animation; // The data of all components in the scene
		std::array<point_light_component,50> entities_light; // The data of all components in the scene*/
		
		//void* entities_data;
		//std::array<char, 2048> enitity_archetype;
		//std::array<char,49152> entity_data;
		//std::array<archetype, 500> entity_archetype;
		//std::array<std::array<const char*,10>, 500> entity_data; // 10 its the number of components per entities
		//std::array<std::array<component_id,10>, 500> entity_component_ids;
	} scene_resource_data;




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

	typedef struct shader_config {
		std::string name;
		renderpass_type renderpass_type;
	} shader_config;

	typedef struct shader {
		std::string name;
		renderpass_type renderpass_type;
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

	// NOTE: Remember to always align the memory by 128 bits and always go from the higher size to the lowest
	typedef struct quad_properties {
		alignas(16)	glm::mat4 model;
		alignas(8)	glm::vec3 diffuse_color;
		alignas(8)	std::array<glm::vec2, 4> texture_region;
		alignas(4)	uint id;
		alignas(4)	uint diffuse_index;
		alignas(4)	uint normal_index;
		alignas(4)	uint specular_index;
		alignas(4)	float shininess_intensity;
		alignas(4)	float shininess_sharpness;
	} quad_properties;

	// TODO: TEMPORAL
	typedef struct pick_quad_properties {
		alignas(16)	glm::mat4 model;
		alignas(8)	std::array<glm::vec2, 4> texture_region;
		alignas(4)	uint id;
		alignas(4)	uint diffuse_index;
	} pick_quad_properties;
	// TODO: TEMPORAL

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