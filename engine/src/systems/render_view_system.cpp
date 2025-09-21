#include "render_view_system.h"

#include "core/logger.h"
#include "renderer/renderer_types.inl"

#include "renderer/views/world_render_view.h"
#include "renderer/views/object_pick_render_view.h"

namespace caliope {

	typedef struct render_view_system_state {
		std::unordered_map<uint,render_view> registered_views;
	}render_view_system_state;

	static std::unique_ptr<render_view_system_state> state_ptr;

	bool render_view_system_initialize() {
		state_ptr = std::make_unique<render_view_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

	
		CE_LOG_INFO("Render view system initialized.");

		return true;
	}

	void render_view_system_shutdown() {
		for (auto view : state_ptr->registered_views) {
			view.second.on_destroy(view.second);
		}

		state_ptr->registered_views.empty();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	void render_view_system_add_view(render_view& view) {
	
		// TODO: Factory pattern (with register, etc. for each type)?
		if (view.type == VIEW_TYPE_WORLD) {
			view.on_create = world_render_view_on_create;
			view.on_destroy = world_render_view_on_destroy;
			view.on_resize_window = world_render_view_on_resize_window;
			view.on_build_package = world_render_view_on_build_package;
			view.on_render = world_render_view_on_render;
		
		}else if (view.type == VIEW_TYPE_OBJECT_PICK) {
			view.on_create = object_pick_render_view_on_create;
			view.on_destroy = object_pick_render_view_on_destroy;
			view.on_resize_window = object_pick_render_view_on_resize_window;
			view.on_build_package = object_pick_render_view_on_build_package;
			view.on_render = object_pick_render_view_on_render;
		}
		state_ptr->registered_views.insert({ view.type, view });

		view.on_create(view);

	}

	void render_view_system_on_window_resize(uint width, uint height) {
		for (auto view : state_ptr->registered_views) {
			view.second.on_resize_window(view.second, width, height);
		}
	}

	bool render_view_system_on_build_packet(view_type view_type, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data) {
		if (state_ptr->registered_views.find(view_type) == state_ptr->registered_views.end()) {
			CE_LOG_WARNING("render_view_system_on_build_packet render view not found.");
			return false;
		}

		return state_ptr->registered_views.at(view_type).on_build_package(state_ptr->registered_views.at(view_type), out_packet, variadic_data);
	}

	bool render_view_system_on_render(view_type view_type, std::any& packet, uint render_target_index) {
		if (state_ptr->registered_views.find(view_type) == state_ptr->registered_views.end()) {
			CE_LOG_WARNING("render_view_system_on_render render view not found.");
			return false;
		}

		return state_ptr->registered_views.at(view_type).on_render(state_ptr->registered_views.at(view_type), packet, render_target_index);
	}
}