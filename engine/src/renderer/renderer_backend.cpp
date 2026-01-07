#include "renderer_backend.h"

#include "renderer/vulkan/vulkan_renderer.h"

#include "core/cememory.h"


namespace caliope {
	bool renderer_backend_system_create(renderer_backend_type type, renderer_backend& out_renderer_backend) {
		if (type == renderer_backend_type::BACKEND_TYPE_VULKAN) {
			out_renderer_backend.initialize = vulkan_renderer_backend_initialize;
			out_renderer_backend.stop = vulkan_renderer_backend_stop;
			out_renderer_backend.shutdown = vulkan_renderer_backend_shutdown;
			out_renderer_backend.resize = vulkan_renderer_backend_resize;
			out_renderer_backend.begin_frame = vulkan_renderer_begin_frame;
			out_renderer_backend.end_frame = vulkan_renderer_end_frame;

			out_renderer_backend.window_images_count_get = vulkan_renderer_window_images_count_get;
			out_renderer_backend.window_image_index_get = vulkan_renderer_window_image_index_get;

			out_renderer_backend.window_attachment_get = vulkan_renderer_window_attachment_get;
			out_renderer_backend.depth_attachment_get = vulkan_renderer_depth_attachment_get;
			out_renderer_backend.object_pick_attachment_get = vulkan_renderer_object_pick_attachment_get;

			out_renderer_backend.render_target_create = vulkan_renderer_render_target_create;
			out_renderer_backend.render_target_destroy = vulkan_renderer_render_target_destroy;

			out_renderer_backend.renderpass_create = vulkan_renderer_renderpass_create;
			out_renderer_backend.renderpass_destroy = vulkan_renderer_renderpass_destroy;
			out_renderer_backend.renderpass_begin = vulkan_renderer_renderpass_begin;
			out_renderer_backend.renderpass_end = vulkan_renderer_renderpass_end;
			
			out_renderer_backend.set_descriptor_ubo = vulkan_renderer_set_descriptor_ubo;
			out_renderer_backend.set_descriptor_sampler = vulkan_renderer_set_descriptor_sampler;
			out_renderer_backend.set_descriptor_ssbo = vulkan_renderer_set_descriptor_ssbo;
			out_renderer_backend.apply_descriptors = vulkan_renderer_apply_descriptors;

			out_renderer_backend.get_descriptor_ssbo = vulkan_renderer_get_descriptor_ssbo;

			out_renderer_backend.draw_geometry = vulkan_renderer_draw_geometry;

			out_renderer_backend.texture_create = vulkan_renderer_texture_create;
			out_renderer_backend.texture_create_writeable = vulkan_renderer_texture_create_writeable;
			out_renderer_backend.texture_destroy = vulkan_renderer_texture_destroy;
			out_renderer_backend.texture_write_data = vulkan_renderer_texture_write_data;
			out_renderer_backend.texture_change_filter = vulkan_renderer_texture_change_filter;

			out_renderer_backend.get_picked_id = vulkan_renderer_get_picked_id;

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