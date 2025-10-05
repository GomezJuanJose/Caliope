#include "binary_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

#include <sstream>//TODO: REMOVE


namespace caliope {

	bool material_loader_load(std::string* file, resource* out_resource) {
	
		file_handle text_file;
		if (!file_system_open(*file + ".cemat", FILE_MODE_READ, text_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		/*/std::vector<uchar> bytes;
		file_system_read_all_bytes(binary_file, bytes, out_resource->data_size);
		material_resource_data* mat_config = reinterpret_cast<material_resource_data*>(bytes.data());
		out_resource->data = *mat_config;*/
		material_resource_data mat_config = {};
		std::string text;
		uint64 read_bytes;
		file_system_read_all_text(text_file, text, read_bytes);

		std::istringstream stream(text.c_str());
		std::string line;
		while (std::getline(stream, line)) {//TODO: Use the function read file by line instead of store the whole text
			if (line.empty() || line.begin()[0] == '#') {
				continue;
			}
			string_trim_character(&line, '\r');

			std::string field, value;
			string_split(&line, &field, &value, '=');


			if (strings_equali(&field, &std::string("name"))) {
				copy_memory(&mat_config.name, value.c_str(), MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("shader_name"))) {
				copy_memory(&mat_config.shader_name, value.c_str(), MAX_NAME_LENGTH);
			}
			else if(strings_equali(&field, &std::string("diffuse_texture")))
			{
				copy_memory(&mat_config.diffuse_texture_name, value.c_str(), MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("specular_texture")))
			{
				copy_memory(&mat_config.specular_texture_name, value.c_str(), MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("normal_texture")))
			{
				copy_memory(&mat_config.normal_texture_name, value.c_str(), MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("diffuse_color")))
			{
				string_to_vec3(&value, &mat_config.diffuse_color);
			}
			else if (strings_equali(&field, &std::string("shininess_sharpness")))
			{
				string_to_float(&value, &mat_config.shininess_sharpness);
			}
			else if (strings_equali(&field, &std::string("shininess_intensity"))) {
				string_to_float(&value, &mat_config.shininess_intensity);
			}
			else if (strings_equali(&field, &std::string("renderpass_type"))) {
				string_to_uint(&value, &mat_config.renderpass_type);
			}

		}
		out_resource->data = mat_config;
		file_system_close(text_file);

		return true;
	}

	void material_loader_unload(resource* resource) {
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.empty();
	}

	resource_loader material_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_MATERIAL;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("materials/");
		loader.load = material_loader_load;
		loader.unload = material_loader_unload;

		return loader;
	}
}