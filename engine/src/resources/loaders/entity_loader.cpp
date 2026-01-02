#include "entity_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

namespace caliope {

	bool entity_loader_load(std::string* file, resource* out_resource) {
		file_handle text_file;
		if (!file_system_open(*file, FILE_MODE_READ, text_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		scene_resource_data scene_config = {};
		std::string text;
		uint64 read_bytes;
		file_system_read_all_text(text_file, text, read_bytes);

		std::istringstream stream(text.c_str());
		std::string line;

		sprite_frame_resource_data sprite_frame_data;

		uint entity_index = -1;
		uint component_index = 0;
		uint compt_id = 0;
		uint component_size = 0;
		uint offset_component_data = 0;

		while (std::getline(stream, line)) {//TODO: Use the function read file by line instead of store the whole text
			if (line == "" || line.begin()[0] == '#') {
				continue;
			}
			string_trim_character(&line, '\r');

			std::string field, value;
			string_split(&line, &field, &value, '=');
			string_trim_character(&field, ' ');
			string_trim_character(&field, '\t');

			if (strings_equali(&field, &std::string("name"))) {
				copy_memory(&scene_config.name, value.c_str(), sizeof(char) * MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("entity_id")))
			{
				uint entity_id;
				string_to_uint(&value, &entity_id);
				scene_config.entity_ids.push_back(entity_id);
			}
			else if (strings_equali(&field, &std::string("archetype_id")))
			{
				uint archetype_id;
				component_index = 0;
				string_to_uint(&value, &archetype_id);
				scene_config.archetypes.push_back((archetype)archetype_id);
				scene_config.components.push_back(std::vector<component_id>());
				scene_config.components_data_types.push_back(std::vector<std::vector<component_data_type>>());
				scene_config.components_data.push_back(std::vector<void*>());
				entity_index++;

			}
			else if (strings_equali(&field, &std::string("component_id")))
			{
				string_to_uint(&value, &compt_id);
				scene_config.components[entity_index].push_back((component_id)compt_id);

				scene_config.components_data_types[entity_index].push_back(std::vector<component_data_type>());

			}
			else if (strings_equali(&field, &std::string("component_size")))
			{
				
				string_to_uint(&value, &component_size);
				scene_config.components_sizes[(component_id)compt_id] = component_size;

				scene_config.components_data[entity_index].push_back(allocate_memory(MEMORY_TAG_LOADER, component_size));
			}
			else if (strings_equali(&field, &std::string("end_component")))
			{
				component_index++;
				offset_component_data = 0;
			}
			else if (strings_equali(&field, &std::string("string")))
			{
				char* memory_dir = (char*)scene_config.components_data[entity_index][component_index];
				copy_memory(memory_dir + offset_component_data, value.c_str(), sizeof(char) * MAX_NAME_LENGTH);
				offset_component_data += sizeof(char) * MAX_NAME_LENGTH;

				scene_config.components_data_types[entity_index][component_index].push_back(COMPONENT_DATA_TYPE_STRING);
			}
			else if (strings_equali(&field, &std::string("vector4")))
			{
				glm::vec4 vec4;
				string_to_vec4(&value, &vec4);
				char* memory_dir = (char*)scene_config.components_data[entity_index][component_index];
				copy_memory(memory_dir + offset_component_data, &vec4, sizeof(glm::vec4));
				offset_component_data += sizeof(glm::vec4);

				scene_config.components_data_types[entity_index][component_index].push_back(COMPONENT_DATA_TYPE_VEC4);

			}
			else if (strings_equali(&field, &std::string("vector3")))
			{
				glm::vec3 vec3;
				string_to_vec3(&value, &vec3);
				char* memory_dir = (char*)scene_config.components_data[entity_index][component_index];
				copy_memory(memory_dir + offset_component_data, &vec3, sizeof(glm::vec3));
				offset_component_data += sizeof(glm::vec3);

				scene_config.components_data_types[entity_index][component_index].push_back(COMPONENT_DATA_TYPE_VEC3);
			}
			else if (strings_equali(&field, &std::string("vector2")))
			{
				glm::vec2 vec2;
				string_to_vec2(&value, &vec2);
				char* memory_dir = (char*)scene_config.components_data[entity_index][component_index];
				copy_memory(memory_dir + offset_component_data, &vec2, sizeof(glm::vec2));
				offset_component_data += sizeof(glm::vec2);

				scene_config.components_data_types[entity_index][component_index].push_back(COMPONENT_DATA_TYPE_VEC2);
			}
			else if (strings_equali(&field, &std::string("float")))
			{
				float f;
				string_to_float(&value, &f);
				char* memory_dir = (char*)scene_config.components_data[entity_index][component_index];
				copy_memory(memory_dir + offset_component_data, &f, sizeof(float));
				offset_component_data += sizeof(float);

				scene_config.components_data_types[entity_index][component_index].push_back(COMPONENT_DATA_TYPE_FLOAT);
			}
			else if (strings_equali(&field, &std::string("integer")))
			{
				uint i;
				string_to_uint(&value, &i);
				char* memory_dir = (char*)scene_config.components_data[entity_index][component_index];
				copy_memory(memory_dir + offset_component_data, &i, sizeof(uint));
				offset_component_data += sizeof(uint);

				scene_config.components_data_types[entity_index][component_index].push_back(COMPONENT_DATA_TYPE_UINT);
			}
		}

		out_resource->data = scene_config;
		file_system_close(text_file);


		return true;
	}

	void entity_loader_unload(resource* resource) {
		scene_resource_data scene_data = std::any_cast<scene_resource_data>(resource->data);
		for (uint i = 0; i < scene_data.components_data.size(); ++i) {
			for (uint j = 0; j < scene_data.components_data[i].size(); ++j) {
				uint component_size = scene_data.components_sizes[scene_data.components[i][j]];
				free_memory(MEMORY_TAG_LOADER, scene_data.components_data[i][j], component_size);
			}
		}

		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.clear();
	}

	resource_loader scene_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_SCENE;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("scenes/");
		loader.load = entity_loader_load;
		loader.unload = entity_loader_unload;

		return loader;
	}

	resource_loader ui_layout_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_UI_LAYOUT;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("ui_layouts/");
		loader.load = entity_loader_load;
		loader.unload = entity_loader_unload;

		return loader;
	}
}