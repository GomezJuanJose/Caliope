#include "binary_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"
#include "renderer/renderer_types.inl"
#include <sstream>//TODO: REMOVE


namespace caliope {

	bool shader_loader_load(std::string* file, resource* out_resource) {
	
		file_handle text_file;
		if (!file_system_open(*file + ".ceshaderconfg", FILE_MODE_READ, text_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		shader_resource_data shader_config = {};
		std::string text;
		uint64 read_bytes;
		file_system_read_all_text(text_file, text, read_bytes);

		std::istringstream stream(text.c_str());
		std::string line;
		while (std::getline(stream, line)) {//TODO: Use the function read file by line instead of store the whole text
			if (line == "" || line.begin()[0] == '#') {
				continue;
			}
			string_trim_character(&line, '\r');

			std::string field, value;
			string_split(&line, &field, &value, '=');


			if (strings_equali(&field, &std::string("name"))) {
				shader_config.name = value;
			}
			else if (strings_equali(&field, &std::string("renderpass"))) {

				if (strings_equali(&value, &std::string("world_renderpass"))) {
					shader_config.renderpass_type = RENDERPASS_TYPE_WORLD;

				} else if (strings_equali(&value, &std::string("ui_renderpass"))) {
					shader_config.renderpass_type = RENDERPASS_TYPE_UI;

				} else if (strings_equali(&value, &std::string("world_object_pick_renderpass"))) {
					shader_config.renderpass_type = RENDERPASS_TYPE_WORLD_OBJECT_PICK;

				} else if (strings_equali(&value, &std::string("ui_object_pick_renderpass"))) {
					shader_config.renderpass_type = RENDERPASS_TYPE_UI_OBJECT_PICK;

				}
			}
			else if (strings_equali(&field, &std::string("vertex_shader_name"))) {
				resource r;
				if (!resource_system_load(std::string("shaders/" + value + ".vert.spv"), RESOURCE_TYPE_BINARY, r)) {
					CE_LOG_ERROR("Could not find the shader: %s", value.c_str());
					continue;
				}
				shader_config.vertex_code = std::any_cast<std::vector<uchar>>(r.data);
				shader_config.vertex_code_size = r.data_size;
				resource_system_unload(r);
			}
			else if(strings_equali(&field, &std::string("fragment_shader_name")))
			{
				resource r;
				if (!resource_system_load(std::string("shaders/" + value + ".frag.spv"), RESOURCE_TYPE_BINARY, r)) {
					CE_LOG_ERROR("Could not find the shader: %s", value.c_str());
					continue;
				}
				shader_config.fragment_code = std::any_cast<std::vector<uchar>>(r.data);
				shader_config.fragment_code_size = r.data_size;
				resource_system_unload(r);
			}
			else if (strings_equali(&field, &std::string("vertex_attribute")))
			{
				vertex_attribute_definition vertex_attribute;
				if (strings_equali(&value, &std::string("vector4"))) {
					vertex_attribute.type = VERTEX_ATTRIBUTE_R32G32B32A32;
					vertex_attribute.size = sizeof(glm::vec4);
				
				}else if (strings_equali(&value, &std::string("vector3"))) {
					vertex_attribute.type = VERTEX_ATTRIBUTE_R32G32B32;
					vertex_attribute.size = sizeof(glm::vec3);
				
				}else if (strings_equali(&value, &std::string("vector2"))) {
					vertex_attribute.type = VERTEX_ATTRIBUTE_R32G32;
					vertex_attribute.size = sizeof(glm::vec2);
				}
				shader_config.vertex_attribute_definitions.push_back(vertex_attribute);

			}
			else if (strings_equali(&field, &std::string("descriptor")))
			{
				descriptor_definition descriptor;
				std::string type_str;
				std::string count_str;
				std::string stage_str;

				string_split(&value, &type_str, &count_str, ',');
				value = string_substring(&value, type_str.length()+1, -1);
				string_split(&value, &count_str, &stage_str, ',');

				if (strings_equali(&type_str, &std::string("image"))) {
					descriptor.type = DESCRIPTOR_TYPE_IMAGE_SAMPLER;
				}
				else if (strings_equali(&type_str, &std::string("uniform"))) {
					descriptor.type = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				}
				else if (strings_equali(&type_str, &std::string("storage"))) {
					descriptor.type = DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}

				uint count;
				string_to_uint(&count_str, &count);
				descriptor.count = count;

				if (strings_equali(&stage_str, &std::string("vertex"))) {
					descriptor.stage = DESCRIPTOR_STAGE_VERTEX;
				}
				else if (strings_equali(&stage_str, &std::string("fragment"))) {
					descriptor.stage = DESCRIPTOR_STAGE_FRAGMENT;
				}

				shader_config.descriptor_definitions.push_back(descriptor);
			}
			else if (strings_equali(&field, &std::string("descriptor_buffer")))
			{
				descriptor_buffer_definition buffer_definition;
				std::string usage;
				std::string size;

				string_split(&value, &usage, &size, ',');

				string_to_uint64(&size, &buffer_definition.size);

				if (strings_equali(&usage, &std::string("uniform"))) {
					buffer_definition.usage = DESCRIPTOR_BUFFER_USAGE_UNIFORM;
				}
				else if (strings_equali(&usage, &std::string("storage"))) {
					buffer_definition.usage = DESCRIPTOR_BUFFER_USAGE_STORAGE;
				}
				else if (strings_equali(&usage, &std::string("storage_and_transfer_destination"))) {
					buffer_definition.usage = (DESCRIPTOR_BUFFER_USAGE_STORAGE | DESCRIPTOR_BUFFER_USAGE_TRANSFER_DST);
				}

				shader_config.descriptor_buffer_definitions.push_back(buffer_definition);
			}
		}
		out_resource->data = shader_config;
		file_system_close(text_file);

		return true;
	}

	void shader_loader_unload(resource* resource) {
		/*/vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&out_shader.internal_data);
		shader_resource_data* srd = std::any_cast<shader_resource_data*>(*resource);
		srd->buffer_sizes.clear();
		srd->vertex_code.clear();
		srd->fragment_code.clear();
		srd->descriptor_definitions.clear();
		srd->vertex_attribute_definitions.clear();*/
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.clear();
	}

	resource_loader shader_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_SHADER;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("shaders/");
		loader.load = shader_loader_load;
		loader.unload = shader_loader_unload;

		return loader;
	}
}