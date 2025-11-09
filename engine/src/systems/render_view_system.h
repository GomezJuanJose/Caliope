#pragma once

#include "defines.h"
#include <any>
#include <string>

namespace caliope {

	struct render_view;
	struct renderer_view_packet;

	typedef enum view_type {
		VIEW_TYPE_WORLD = 0,
		VIEW_TYPE_OBJECT_PICK,
		VIEW_TYPE_UI
	}view_type;


	bool render_view_system_initialize();
	void render_view_system_shutdown();

	void render_view_system_add_view(render_view& view);

	void render_view_system_on_window_resize(uint width, uint height);
	bool render_view_system_on_build_packet(view_type view_type, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data);
	bool render_view_system_on_render(view_type view_type, std::any& packet, uint render_target_index);
}