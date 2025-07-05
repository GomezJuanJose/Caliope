#pragma once
#include "defines.h"

namespace caliope {

	struct material;
	struct material_configuration;

	bool material_system_initialize();
	void material_system_shutdown();

	CE_API material* material_system_adquire(std::string& name);
	CE_API material* material_system_adquire_from_config(material_configuration& material_config);

	CE_API void material_system_release(std::string& name);

	CE_API material* material_system_get_default();
}