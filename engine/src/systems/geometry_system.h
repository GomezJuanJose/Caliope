#pragma once
#include "defines.h"

namespace caliope {
	struct geometry;

	bool geometry_system_initialize();
	void geometry_system_shutdown();

	CE_API geometry* geometry_system_get_quad();
}