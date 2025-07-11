#include "renderer_backend.h"

#include "renderer/vulkan/vulkan_renderer.h"

#include "core/cememory.h"


namespace caliope {
	bool renderer_backend_system_create(renderer_backend_type type, renderer_backend& out_renderer_backend) {
		if (type == renderer_backend_type::BACKEND_TYPE_VULKAN) {
			out_renderer_backend.initialize = vulkan_renderer_backend_initialize;
			out_renderer_backend.shutdown = vulkan_renderer_backend_shutdown;
			out_renderer_backend.resize = vulkan_renderer_backend_resize;
			out_renderer_backend.begin_frame = vulkan_renderer_begin_frame;
			out_renderer_backend.end_frame = vulkan_renderer_end_frame;

			out_renderer_backend.begin_renderpass = vulkan_renderer_begin_renderpass;
			out_renderer_backend.end_renderpass = vulkan_renderer_end_renderpass;

			out_renderer_backend.set_and_apply_uniforms = vulkan_renderer_set_and_apply_uniforms;

			out_renderer_backend.draw_geometry = vulkan_renderer_draw_geometry;

			out_renderer_backend.texture_create = vulkan_renderer_texture_create;
			out_renderer_backend.texture_destroy = vulkan_renderer_texture_destroy;

			out_renderer_backend.shader_create = vulkan_renderer_shader_create;
			out_renderer_backend.shader_destroy = vulkan_renderer_shader_destroy;
			out_renderer_backend.shader_use = vulkan_renderer_shader_use;

			out_renderer_backend.geometry_create = vulkan_renderer_geometry_create;
			out_renderer_backend.geometry_destroy = vulkan_renderer_geometry_destroy;
		
			return true;
		}

		return false;
	}

	void renderer_backend_system_destroy(renderer_backend& renderer_backend) {
		zero_memory(&renderer_backend, sizeof(renderer_backend));
	}
}