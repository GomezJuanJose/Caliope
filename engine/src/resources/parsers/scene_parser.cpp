#include "scene_parser.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

#include "systems/ecs_system.h"

namespace caliope {

	bool scene_parser_parse(std::string* file, void* data) {
		

		scene* scene_data = (scene*)data;

		file_handle text_file;
		if (!file_system_open(*file + ".cescene", FILE_MODE_WRITE, text_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}

		file_system_write_text(text_file, "# Scene definition\n");
		file_system_write_text(text_file, "# NOTE: The component information must be in the same order as the defined in the code structure\n");
		file_system_write_text(text_file, "# NOTE: 'component_size' defines the size in bytes of each component_id, if is a string should be counted as 255*sizeof(char)\n\n");
		file_system_write_text(text_file, "name=" + std::string(&scene_data->name[0]) + "\n");

		for (uint entity_index = 0; entity_index < scene_data->entities.size(); ++entity_index) {
			uint entity = scene_data->entities[entity_index];
			file_system_write_text(text_file, "archetype_id=" + std::to_string(ecs_system_get_entity_archetype(entity)) + "\n");

			std::vector<component_id> components = ecs_system_get_entity_components(entity);
			for (uint component_index = 0; component_index < components.size(); ++component_index) {
				file_system_write_text(text_file, "\tcomponent_id=" + std::to_string(components[component_index]) + "\n");
				uint64 component_size = 0;
				void* component_data = ecs_system_get_component_data(entity, components[component_index], component_size);
				file_system_write_text(text_file, "\tcomponent_size=" + std::to_string(component_size) + "\n");

				uint offset = 0;
				std::vector<component_data_type> component_data_types = ecs_system_get_component_data_types(components[component_index]);
				for (uint component_data_type_index = 0; component_data_type_index < component_data_types.size(); ++component_data_type_index) {
					if (COMPONENT_DATA_TYPE_STRING == component_data_types[component_data_type_index]) {
						char* str = static_cast<char*>((char*)component_data + offset);
						file_system_write_text(text_file, "\t\tstring=" + std::string(str) + "\n");
						offset += MAX_NAME_LENGTH * sizeof(char);
					}
					else if (COMPONENT_DATA_TYPE_VEC4 == component_data_types[component_data_type_index]) {
						glm::vec4* vec4 = (glm::vec4*)((char*)component_data + offset);
						file_system_write_text(text_file, "\t\tvector4=" + std::to_string(vec4->x) + " " + std::to_string(vec4->y) + " " + std::to_string(vec4->z) + " " + std::to_string(vec4->w) + "\n");
						offset += sizeof(glm::vec4);
					}
					else if (COMPONENT_DATA_TYPE_VEC3 == component_data_types[component_data_type_index]) {
						glm::vec3* vec3 = (glm::vec3*)((char*)component_data + offset);
						file_system_write_text(text_file, "\t\tvector3=" + std::to_string(vec3->x) + " " + std::to_string(vec3->y) + " " + std::to_string(vec3->z) + "\n");
						offset += sizeof(glm::vec3);
					}
					else if (COMPONENT_DATA_TYPE_UINT == component_data_types[component_data_type_index]) {
						uint* uinteger = (glm::uint*)((char*)component_data + offset);
						file_system_write_text(text_file, "\t\tinteger=" + std::to_string(*uinteger) + "\n");
						offset += sizeof(uint);
					}
					else if (COMPONENT_DATA_TYPE_FLOAT == component_data_types[component_data_type_index]) {
						float* f = (float*)((char*)component_data + offset);
						file_system_write_text(text_file, "\t\tfloat=" + std::to_string(*f) + "\n");
						offset += sizeof(float);
					}
				}


				file_system_write_text(text_file, "\tend_component\n");

				if (component_index != components.size() - 1) {
					file_system_write_text(text_file, "\n");
				}
			}

			file_system_write_text(text_file, "end_archetype\n\n");
		}

		file_system_close(text_file);

		return true;
	}

	resource_parser scene_resource_parser_create() {
		resource_parser parser;

		parser.type = RESOURCE_TYPE_SCENE;
		parser.custom_type = std::string("");
		parser.resource_folder = std::string("scenes/");
		parser.parse = scene_parser_parse;

		return parser;
	}
}