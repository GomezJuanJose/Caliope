#include "image_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendors/stb_image/stb_image.h"

namespace caliope {
	bool image_loader_load(std::string* file, resource* out_resource) {

		//TODO : try with differents formats
		#define IMAGE_FORMATS_COUNT 4
		std::string formats[IMAGE_FORMATS_COUNT] = {".png", ".jpg", ".tga", ".bmp"};
		std::string selected_format = "";
		for (int i = 0; i < IMAGE_FORMATS_COUNT; ++i) {
			if (file_system_exists(*file + formats[i])) {
				selected_format = formats[i];
				break;
			}
		}

		if (selected_format.empty()) {
			CE_LOG_ERROR("File %s not found", file->c_str());
			return false;
		}
		
		const int required_channel_count = 4;
		stbi_set_flip_vertically_on_load(true);

		int tex_width, tex_height, tex_channels;
		stbi_uc* pixels = stbi_load(file->append(selected_format).c_str(), &tex_width, &tex_height, &tex_channels, required_channel_count);

		image_resource_data image_data;
		image_data.channel_count = tex_channels;
		image_data.width = tex_width;
		image_data.height = tex_height;
		image_data.pixels = pixels;

		out_resource->data = image_data;
		out_resource->data_size = tex_width * tex_height * tex_channels;

		return true;
	}

	void image_loader_unload(resource* resource) {
		image_resource_data image = std::any_cast<image_resource_data>(resource->data);
		uchar* pixels = image.pixels;
		stbi_image_free(pixels);
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.empty();
	}

	resource_loader image_resource_loader_create() {
		resource_loader resource;
		resource.type = RESOURCE_TYPE_TEXTURE;
		resource.custom_type = "";
		resource.resource_folder = std::string("textures\\");
		resource.load = image_loader_load;
		resource.unload = image_loader_unload;

		return resource;
	}
}