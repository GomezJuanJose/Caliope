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


	typedef enum material_type {
		MATERIAL_TYPE_SCENE,
		MATERIAL_TYPE_UI,
		MATERIAL_TYPE_UNDEFINED
	};

	typedef struct material_configuration {
		std::string name;
		material_type type;
		glm::vec3 diffuse_color;
		std::string diffuse_texture_name;
	}material_configuration;

	typedef struct material {
		std::string name;
		material_type type;
		glm::vec3 diffuse_color;
		std::shared_ptr<texture> diffuse_texture;
	}material;
}