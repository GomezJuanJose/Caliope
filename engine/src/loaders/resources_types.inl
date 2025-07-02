#pragma once
#include "defines.h"
#include <glm/glm.hpp>

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

	typedef struct texture {
		std::string name;
		uint width;
		uint height;
		uint channel_count;
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
		glm::vec4 diffuse_color;
		float shininess;
		std::array<char, MAX_NAME_LENGTH> diffuse_texture_name;
		std::array<char, MAX_NAME_LENGTH> specular_texture_name;
		std::array<char, MAX_NAME_LENGTH> normal_texture_name;
	}material_configuration;

	typedef struct material {
		std::string name;
		glm::vec4 diffuse_color;
		float shininess;
		std::shared_ptr<shader> shader;
		std::shared_ptr<texture> diffuse_texture;
		std::shared_ptr<texture> specular_texture;
		std::shared_ptr<texture> normal_texture;

	}material;

	typedef struct quad_properties {
		glm::mat4 model;
	} quad_properties;
}