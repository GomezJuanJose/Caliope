#include "resource_system.h"
#include "cepch.h"

#include "core/logger.h"

#include "loaders/resources_types.inl"
#include "loaders/image_loader.h"
#include "loaders/binary_loader.h"
#include "loaders/material_loader.h"
#include "loaders/audio_loader.h"
#include "loaders/scene_loader.h"

namespace caliope {

	typedef struct resource_system_state {
		resource_system_config config;
		std::unordered_map<std::string, resource_loader> loaders;
	} resource_system_state;

	static std::unique_ptr<resource_system_state> state_ptr;

	bool resource_system_initialize(resource_system_config& config) {

		state_ptr = std::make_unique<resource_system_state>();
		
		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->config = config;

		resource_system_register_loader(binary_resource_loader_create());
		resource_system_register_loader(image_resource_loader_create());
		resource_system_register_loader(material_resource_loader_create());
		resource_system_register_loader(audio_resource_loader_create());
		resource_system_register_loader(scene_resource_loader_create());

		CE_LOG_INFO("Resource system initialized.");
		return true;
	}

	void resource_system_shutdown() {
		state_ptr.reset();
		state_ptr = nullptr;
	}

	bool resource_system_register_loader(resource_loader loader) {

		loader.resource_folder = state_ptr->config.base_path + loader.resource_folder;
		state_ptr->loaders.insert({std::to_string(loader.type), loader});
		return true;
	}

	bool resource_system_load(std::string& name, resource_type type, resource& resource) {
		
		if (state_ptr->loaders.find(std::to_string(type)) == state_ptr->loaders.end()) {
			return false;
		}
		
		resource.loader_name = std::to_string(type);

		std::string file_path = state_ptr->loaders[std::to_string(type)].resource_folder + name;
		
		return state_ptr->loaders[std::to_string(type)].load(&file_path, &resource);
	}

	bool resource_system_load_custom(std::string& name, std::string& custom_type, resource& resource) {
		
		if (state_ptr->loaders.find(custom_type) == state_ptr->loaders.end()) {
			return false;
		}

		resource.loader_name = custom_type;

		std::string file_path = state_ptr->loaders[custom_type].resource_folder + name;
		state_ptr->loaders[custom_type].load(&file_path, &resource);
		return true;
	}

	void resource_system_unload(resource& resource) {
		if (state_ptr->loaders.find(resource.loader_name) == state_ptr->loaders.end()) {
			return;
		}

		state_ptr->loaders[std::string(resource.loader_name)].unload(&resource);
	}
}