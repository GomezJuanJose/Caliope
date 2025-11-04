#include "binary_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

#include <sstream>//TODO: REMOVE when the read line by line is done


namespace caliope {

	bool sprite_animation_loader_load(std::string* file, resource* out_resource) {
	
		file_handle text_file;
		if (!file_system_open(*file + ".cesprtanim", FILE_MODE_READ, text_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		sprite_animation_resource_data sprite_anim_config = {};
		std::string text;
		uint64 read_bytes;
		file_system_read_all_text(text_file, text, read_bytes);

		std::istringstream stream(text.c_str());
		std::string line;

		sprite_frame_resource_data sprite_frame_data;

		while (std::getline(stream, line)) {//TODO: Use the function read file by line instead of store the whole text
			if (line == "" || line.begin()[0] == '#') {
				continue;
			}
			string_trim_character(&line, '\r');

			std::string field, value;
			string_split(&line, &field, &value, '=');


			if (strings_equali(&field, &std::string("name"))) {
				copy_memory(&sprite_anim_config.name, value.c_str(), sizeof(char) * MAX_NAME_LENGTH);
			}
			else if(strings_equali(&field, &std::string("loop")))
			{
				string_to_bool(&value, &sprite_anim_config.is_looping);
			}
			else if (strings_equali(&field, &std::string("play_on_start")))
			{
				string_to_bool(&value, &sprite_anim_config.is_playing);
			}
			else if (strings_equali(&field, &std::string("frames_per_second")))
			{
				string_to_float(&value, &sprite_anim_config.frames_per_second);
			}
			else if (strings_equali(&field, &std::string("number_of_rows")))
			{
				string_to_uint(&value, &sprite_anim_config.number_of_rows);
			}
			else if (strings_equali(&field, &std::string("number_of_columns")))
			{
				string_to_uint(&value, &sprite_anim_config.number_of_columns);
			}
			else if (strings_equali(&field, &std::string("starting_row")))
			{
				string_to_uint(&value, &sprite_anim_config.starting_row);
			}
			else if (strings_equali(&field, &std::string("starting_column")))
			{
				string_to_uint(&value, &sprite_anim_config.starting_column);
			}
			else if (strings_equali(&field, &std::string("material")))
			{
				sprite_frame_data.material_name = value;
			}
			else if (strings_equali(&field, &std::string("grid_region")))
			{
				glm::vec2 region;
				string_to_vec2(&value, &region);
				sprite_frame_data.grid_size = region;

				sprite_anim_config.frames_data.push_back(sprite_frame_data);
			}

		}
		out_resource->data = sprite_anim_config;
		file_system_close(text_file);

		return true;
	}

	void sprite_animation_loader_unload(resource* resource) {
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.clear();
	}

	resource_loader sprite_animation_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_SPRITE_ANIMATION;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("sprite_animations/");
		loader.load = sprite_animation_loader_load;
		loader.unload = sprite_animation_loader_unload;

		return loader;
	}
}