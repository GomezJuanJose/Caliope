#include "image_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "vendors/stb_truetype/stb_truetype.h"

namespace caliope {
	bool text_font_loader_load(std::string* file, resource* out_resource) {

		#define FONT_FORMATS_COUNT 1
		std::string formats[FONT_FORMATS_COUNT] = {".ttf"};
		std::string selected_format = "";
		for (int i = 0; i < FONT_FORMATS_COUNT; ++i) {
			if (file_system_exists(*file + formats[i])) {
				selected_format = formats[i];
				break;
			}
		}

		if (selected_format.empty()) {
			CE_LOG_ERROR("File %s not found", file->c_str());
			return false;
		}

		file_handle font_binary_handle;

		if (!file_system_open(file->append(selected_format), FILE_MODE_READ, font_binary_handle)) {
			CE_LOG_ERROR("Unable to open binary font file %s.", file->c_str());
			return false;
		}

		uint64 file_size;
		if (!file_system_size(font_binary_handle, file_size)) {
			CE_LOG_ERROR("Unable to get file size of binary font file. Load process failed.");
			return false;
		}
		out_resource->data_size = file_size;

		std::vector<uchar> bytes;
		file_system_read_all_bytes(font_binary_handle, bytes, out_resource->data_size);
		
		file_system_close(font_binary_handle);

		// Gets the file name
		std::string font_name_format = "", font_name, font_format;
		for (uint i = file->length()-1; i > 0; --i) {
			if (file->at(i) == '\\' || file->at(i) == '/') {
				break;
			}
			font_name_format.insert(font_name_format.begin(), file->at(i));
		}

		string_split(&font_name_format, &font_name, &font_format, '.');
		
		text_font_resource_data font_data;
		copy_memory(&font_data.name, font_name.c_str(), sizeof(char) * MAX_NAME_LENGTH);
		font_data.binary_data = bytes;
		int result = stbtt_InitFont(&font_data.stb_font_info, font_data.binary_data.data(), stbtt_GetFontOffsetForIndex(font_data.binary_data.data(), 0));
		out_resource->data = font_data;

		return true;
	}

	void text_font_loader_unload(resource* resource) {
		text_font_resource_data font = std::any_cast<text_font_resource_data>(resource->data);
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.clear();
	}

	resource_loader text_font_resource_loader_create() {
		resource_loader resource;
		resource.type = RESOURCE_TYPE_TEXT_FONT;
		resource.custom_type = "";
		resource.resource_folder = std::string("text_fonts/");
		resource.load = text_font_loader_load;
		resource.unload = text_font_loader_unload;

		return resource;
	}
}