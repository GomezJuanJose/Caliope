#pragma once

#include "defines.h"

#include "renderer/renderer_types.inl"


namespace caliope {
	bool renderer_backend_system_create(renderer_backend_type type, renderer_backend& out_renderer_backend);
	void renderer_backend_system_destroy(renderer_backend& renderer_backend);
}