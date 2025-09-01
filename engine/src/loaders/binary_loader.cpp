#include "binary_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

namespace caliope {

	bool binary_loader_load(std::string* file, resource* out_resource) {
	
		file_handle binary_file;
		if (!file_system_open(*file, FILE_MODE_READ, binary_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		std::vector<uchar> bytes;
		file_system_read_all_bytes(binary_file, bytes, out_resource->data_size);
		out_resource->data = bytes;
		file_system_close(binary_file);

		return true;
	}

	void binary_loader_unload(resource* resource) {
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.empty();
	}

	resource_loader binary_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_BINARY;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("");
		loader.load = binary_loader_load;
		loader.unload = binary_loader_unload;

		return loader;
	}
}