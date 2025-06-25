#pragma once
#include "defines.h"

namespace caliope {

	struct shader;

	bool shader_system_initialize();
	void shader_system_shutdown();

	CE_API std::shared_ptr<shader> shader_system_adquire(std::string& name);
	CE_API void shader_system_release(std::string& name);

	CE_API void shader_system_use(std::string& name);
}