#pragma once

#include "defines.h"
#include <any>

namespace caliope {

	struct render_view;
	struct renderer_view_packet;

	void world_render_view_on_create (render_view& self);
	void world_render_view_on_destroy (render_view& self);
	void world_render_view_on_resize_window (render_view& self, uint width, uint height);
	bool world_render_view_on_build_package (render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data);
	bool world_render_view_on_render (render_view& self, std::any& packet);
}