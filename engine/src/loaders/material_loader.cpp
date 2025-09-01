#include "binary_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "core/cememory.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

namespace caliope {

	bool material_loader_load(std::string* file, resource* out_resource) {
	
		file_handle binary_file;
		if (!file_system_open(*file + ".cemat", FILE_MODE_READ, binary_file)) {
			CE_LOG_ERROR("Couldnt open %s", file->c_str());
			return false;
		}
		std::vector<uchar> bytes;
		file_system_read_all_bytes(binary_file, bytes, out_resource->data_size);
		material_resource_data* mat_config = reinterpret_cast<material_resource_data*>(bytes.data());
		out_resource->data = *mat_config;
		file_system_close(binary_file);



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
		loader.resource_folder = std::string("materials\\");
		loader.load = material_loader_load;
		loader.unload = material_loader_unload;

		return loader;
	}
}