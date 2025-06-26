#pragma once
#include "defines.h"

namespace caliope {

	struct texture;

	bool texture_system_initialize();
	void texture_system_shutdown();

	CE_API std::shared_ptr<texture> texture_system_adquire(std::string& name);
	CE_API void texture_system_release(std::string& name);

	CE_API std::shared_ptr<texture> texture_system_get_default_diffuse();
	CE_API std::shared_ptr<texture> texture_system_get_default_specular();
	CE_API std::shared_ptr<texture> texture_system_get_default_normal();
}