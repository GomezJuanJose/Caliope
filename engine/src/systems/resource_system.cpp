#include "resource_system.h"
#include "cepch.h"

#include "core/logger.h"

#include "resources/resources_types.inl"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/shader_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/sprite_animation_loader.h"
#include "resources/loaders/audio_loader.h"
#include "resources/loaders/entity_loader.h"
#include "resources/loaders/text_font_loader.h"
#include "resources/loaders/text_style_loader.h"

#include "resources/parsers/entity_parser.h"

namespace caliope {

	typedef struct resource_system_state {
		resource_system_config config;
		std::unordered_map<std::string, resource_loader> loaders;
		std::unordered_map<std::string, resource_parser> parsers;
	} resource_system_state;

	static std::unique_ptr<resource_system_state> state_ptr;

	bool resource_system_initialize(resource_system_config& config) {

		state_ptr = std::make_unique<resource_system_state>();
		
		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->config = config;

		// Loaders
		resource_system_register_loader(binary_resource_loader_create());
		resource_system_register_loader(image_resource_loader_create());
		resource_system_register_loader(shader_resource_loader_create());
		resource_system_register_loader(material_resource_loader_create());
		resource_system_register_loader(sprite_animation_resource_loader_create());
		resource_system_register_loader(audio_resource_loader_create());
		resource_system_register_loader(scene_resource_loader_create());
		resource_system_register_loader(ui_layout_resource_loader_create());
		resource_system_register_loader(text_font_resource_loader_create());
		resource_system_register_loader(text_style_resource_loader_create());

		// Parsers
		resource_system_register_parser(scene_resource_parser_create());
		resource_system_register_parser(ui_layout_resource_parser_create());

		CE_LOG_INFO("Resource system initialized.");
		return true;
	}

	void resource_system_shutdown() {
		state_ptr.reset();
		state_ptr = nullptr;
	}

	bool resource_system_register_loader(resource_loader& loader) {

		loader.resource_folder = state_ptr->config.base_path + loader.resource_folder;
		state_ptr->loaders.insert({std::to_string(loader.type), loader});
		return true;
	}

	bool resource_system_register_parser(resource_parser& parser)
	{
		parser.resource_folder = state_ptr->config.base_path + parser.resource_folder;
		state_ptr->parsers.insert({ std::to_string(parser.type), parser });
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

	bool resource_system_parse(std::string& name, resource_type type, void* data)
	{
		if (state_ptr->parsers.find(std::to_string(type)) == state_ptr->parsers.end()) {
			return false;
		}

		std::string file_path = state_ptr->parsers[std::to_string(type)].resource_folder + name;
		return state_ptr->parsers[std::to_string(type)].parse(&file_path, data);
	}

	void resource_system_unload(resource& resource) {
		if (state_ptr->loaders.find(resource.loader_name) == state_ptr->loaders.end()) {
			return;
		}

		state_ptr->loaders[std::string(resource.loader_name)].unload(&resource);
	}
}