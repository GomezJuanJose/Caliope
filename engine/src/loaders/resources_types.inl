#pragma once
#include "defines.h"
#include <glm/glm.hpp>
#include "math/transform.h"

namespace caliope {
	typedef enum resource_type{
		RESOURCE_TYPE_BINARY = 0,
		RESOURCE_TYPE_IMAGE,
		RESOURCE_TYPE_MATERIAL
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

#define MAX_NAME_LENGTH 256
	typedef struct material_configuration {
		std::array<char, MAX_NAME_LENGTH> name;
		std::array<char, MAX_NAME_LENGTH> shader_name;
		glm::vec3 diffuse_color;
		float shininess_intensity;
		float shininess_sharpness;
		std::array<char, MAX_NAME_LENGTH> diffuse_texture_name;
		std::array<char, MAX_NAME_LENGTH> specular_texture_name;
		std::array<char, MAX_NAME_LENGTH> normal_texture_name;
	}material_configuration;

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
	typedef struct sprite_properties {
		alignas(16)	glm::mat4 model;
		alignas(8)	glm::vec3 diffuse_color;
		alignas(8)	std::array<glm::vec2, 4> texture_region;
		alignas(4)	uint diffuse_index;
		alignas(4)	uint normal_index;
		alignas(4)	uint specular_index;
		alignas(4)	float shininess_intensity;
		alignas(4)	float shininess_sharpness;
	} sprite_properties;

	typedef struct sprite_definition {
		uint z_order;
		std::string material_name;
		transform transform;
		std::array<glm::vec2,4> texture_region;
	} sprite_definition;

	typedef struct geometry{
		std::string name;
		uint index_count;
		std::any internal_data;
	} geometry;
}