#pragma once

#include"defines.h"
#include "renderer_types.inl"

#include <string>

namespace caliope {
	bool renderer_system_initialize(const std::string& application_name);
	void renderer_system_shutdown();

	void renderer_on_resized(uint16 width, uint16 height);

	bool renderer_draw_frame(renderer_packet& packet);
}