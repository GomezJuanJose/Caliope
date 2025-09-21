#include "object_pick_render_view.h"

namespace caliope {
	void object_pick_render_view_on_create(render_view& self) {

	}

	void object_pick_render_view_on_destroy(render_view& self) {

	}

	void object_pick_render_view_on_resize_window(render_view& self, uint width, uint heigh) {

	}

	bool object_pick_render_view_on_build_package(render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data) {
		return false;
	}

	bool object_pick_render_view_on_render(render_view& self, std::any& packet, uint render_target_index) {
		return false;
	}
}