#pragma once
#include "renderer/renderer_types.inl"

#include <string>

namespace caliope {
	struct quad_properties;
	struct pick_quad_properties;

	bool vulkan_renderer_backend_initialize(const renderer_backend_config& config);
	void vulkan_renderer_backend_stop();
	void vulkan_renderer_backend_shutdown();
	void vulkan_renderer_backend_resize(uint16 width, uint16 height);

	bool vulkan_renderer_begin_frame(float delta_time);
	bool vulkan_renderer_end_frame(float delta_time);

	void vulkan_renderer_draw_geometry(uint quad_count, geometry& geometry);

	uint vulkan_renderer_window_images_count_get();
	uint vulkan_renderer_window_image_index_get();

	std::shared_ptr<std::any> vulkan_renderer_window_attachment_get(uint index);
	std::shared_ptr<std::any> vulkan_renderer_depth_attachment_get();

	bool vulkan_renderer_render_target_create(renderpass& pass, render_target& target);
	bool vulkan_renderer_render_target_destroy(render_target& target);

	bool vulkan_renderer_renderpass_create(renderpass& pass, glm::vec4 clear_color, float depth, uint stencil, bool has_prev_pass, bool has_next_pass);
	void vulkan_renderer_renderpass_destroy(renderpass& pass);
	bool vulkan_renderer_renderpass_begin(renderpass& pass, render_target& target);
	bool vulkan_renderer_renderpass_end();

	void vulkan_renderer_set_descriptor_ubo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index);
	void vulkan_renderer_set_descriptor_sampler(std::vector<texture*>& textures_batch_ptr, uint destination_binding, shader& shader);
	void vulkan_renderer_set_descriptor_ssbo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index);
	void vulkan_renderer_apply_descriptors(shader& shader);

	void vulkan_renderer_texture_create(texture& t, uchar* pixels);
	void vulkan_renderer_texture_destroy(texture& t);
	void vulkan_renderer_texture_change_filter(texture& t);

	bool vulkan_renderer_shader_create(shader_resource_data& shader_config, shader& out_shader, renderpass& pass);
	void vulkan_renderer_shader_destroy(shader& s);
	void vulkan_renderer_shader_use(shader& s);

	void vulkan_renderer_geometry_create(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices);
	void vulkan_renderer_geometry_destroy(geometry& geometry);


	//TODO: TEMPORAL
	void pick_object(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view);
	void show_picked_obj();
	//TODO: END TEMPORAL
}