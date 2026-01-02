#include "text_style_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

namespace caliope {

	bool text_style_loader_load(std::string* file, resource* out_resource) {
		file_handle text_file;
		if (!file_system_open(*file + ".cetxst", FILE_MODE_READ, text_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		text_style_resource_data text_style_config = {};
		std::string text;
		uint64 read_bytes;
		file_system_read_all_text(text_file, text, read_bytes);

		std::istringstream stream(text.c_str());
		std::string line;

		uint style_index = 0;
		uint image_index = 0;

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
				copy_memory(&text_style_config.name, value.c_str(), sizeof(char) * MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("style_tag")))
			{
				uint archetype_id;
				text_style_config.style_tag_names.push_back(std::array<char, MAX_NAME_LENGTH>());
				copy_memory(&text_style_config.style_tag_names[style_index], value.c_str(), sizeof(char) * MAX_NAME_LENGTH);

				text_style_config.text_fonts.push_back(std::array<char, MAX_NAME_LENGTH>());
				text_style_config.font_sizes.push_back(20);
				text_style_config.text_colors.push_back(glm::vec4(1.0f));
				text_style_config.additional_interline_spaces.push_back(0);

			}
			else if (strings_equali(&field, &std::string("text_color")))
			{
				glm::vec4 color;
				string_to_vec4(&value, &color);
				text_style_config.text_colors[style_index] = color;
			}
			else if (strings_equali(&field, &std::string("font")))
			{
				copy_memory(&text_style_config.text_fonts[style_index], value.c_str(), sizeof(char) * MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("additional_interlinear_space")))
			{
				uint interline;
				string_to_uint(&value, &interline);
				text_style_config.additional_interline_spaces[style_index] = interline;
			}
			else if (strings_equali(&field, &std::string("font_size")))
			{
				uint size;
				string_to_uint(&value, &size);
				text_style_config.font_sizes[style_index] = size;
			}
			else if (strings_equali(&field, &std::string("end_style_tag"))) {
				style_index++;
			}
			else if (strings_equali(&field, &std::string("image_tag")))
			{
				text_style_config.image_tag_names.push_back(std::array<char, MAX_NAME_LENGTH>());
				copy_memory(&text_style_config.image_tag_names[image_index], value.c_str(), sizeof(char) * MAX_NAME_LENGTH);

				text_style_config.image_materials.push_back(std::array<char, MAX_NAME_LENGTH>());
				text_style_config.image_sizes.push_back(glm::vec2({ 20 }));
				text_style_config.texture_coordinates.push_back(glm::vec4({ 0 }));
			}
			else if (strings_equali(&field, &std::string("material")))
			{
				copy_memory(&text_style_config.image_materials[image_index], value.c_str(), sizeof(char) * MAX_NAME_LENGTH);
			}
			else if (strings_equali(&field, &std::string("size"))) {
				glm::vec2 size;
				string_to_vec2(&value, &size);
				text_style_config.image_sizes[image_index] = size;
			}
			else if (strings_equali(&field, &std::string("texture_coordinates"))) {
				glm::vec4 coords;
				string_to_vec4(&value, &coords);
				text_style_config.texture_coordinates[image_index] = coords;
			}
			else if (strings_equali(&field, &std::string("end_style_tag"))) {
				image_index++;
			}
		}

		out_resource->data = text_style_config;
		file_system_close(text_file);

		return true;
	}

	void text_style_loader_unload(resource* resource) {
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.clear();
	}

	resource_loader text_style_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_TEXT_STYLE;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("text_styles/");
		loader.load = text_style_loader_load;
		loader.unload = text_style_loader_unload;

		return loader;
	}
}