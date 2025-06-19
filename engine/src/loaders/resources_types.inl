#pragma once
#include "defines.h"


namespace caliope {
	typedef enum resource_type{
		RESOURCE_TYPE_BINARY = 0,
		RESOURCE_TYPE_IMAGE
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
}