#pragma once

#include"defines.h"
#include "renderer_types.inl"

#include <string>

namespace caliope {
	struct texture;
	struct shader;

	typedef struct renderer_frontend_config {
		std::string application_name;
		uint window_width;
		uint window_height;

	} renderer_frontend_config;

	bool renderer_system_initialize(renderer_frontend_config& config);

	/*
	 * @brief Stops the renderer and all the resources being used, use this function before shutting down systems and the renderer to avoid destroying resources while being used.
	 */
	void renderer_system_stop();

	void renderer_system_shutdown();

	void renderer_on_resized(uint16 width, uint16 height);

	bool renderer_draw_frame(std::vector<renderer_view_packet>& packets, float delta_time);

	void renderer_texture_create(texture& texture, uchar* pixels);
	void renderer_texture_destroy(texture& texture);
	void renderer_texture_change_filter(texture& texture);

	bool renderer_shader_create(shader_resource_data& shader_config, shader& shader);
	void renderer_shader_destroy(shader& shader);
	void renderer_shader_use(shader& shader);

	void renderer_geometry_create(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices);
	void renderer_geometry_destroy(geometry& geometry);

	bool renderer_renderpass_begin(renderpass_type type, uint render_target_index);
	bool renderer_renderpass_end();
	void renderer_renderpass_set_render_area(renderpass_type type, glm::vec4 render_area);

	void renderer_set_descriptor_ubo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index);
	void renderer_set_descriptor_sampler(std::vector<texture*>& textures_batch_ptr, uint destination_binding, shader& shader);
	void renderer_set_descriptor_ssbo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index);
	void renderer_apply_descriptors(shader& shader);
	
	void renderer_draw_geometry(uint instance_count, geometry& geometry);
	void renderer_draw_object_pick(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view);
}