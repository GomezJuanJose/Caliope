#pragma once
#include "defines.h"

#include <string>

namespace caliope {

	struct resource;
	enum resource_type;


	typedef struct resource_system_config {
		uint max_number_loaders;
		std::string base_path;
	}resource_system_config;

	typedef struct resource_loader {
		resource_type type;
		std::string custom_type;
		std::string resource_folder;
		bool (*load)(std::string* file, resource* out_resource);
		void (*unload)(resource* resource);
	};

	bool resource_system_initialize(resource_system_config& config);
	void resource_system_shutdown();

	CE_API bool resource_system_register_loader(resource_loader loader);
	
	CE_API bool resource_system_load(std::string& name, resource_type type, resource& resource);
	CE_API bool resource_system_load_custom(std::string& name, std::string& custom_type, resource& resource);

	CE_API void resource_system_unload(resource& resource);

}