#pragma once
#include "defines.h"

namespace caliope {
	struct camera;

	bool camera_system_initialize();
	void camera_system_shutdown();

	CE_API camera* camera_system_acquire(const std::string& name);
	CE_API void camera_system_release(const std::string& name);

	CE_API camera* camera_system_get_default();
}