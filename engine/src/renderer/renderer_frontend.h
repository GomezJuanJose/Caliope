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

		uint max_number_quads;
		uint max_textures_per_batch;
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

	void renderer_shader_create(shader& shader);
	void renderer_shader_destroy(shader& shader);
	void renderer_shader_use(shader& shader);

	void renderer_geometry_create(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices);
	void renderer_geometry_destroy(geometry& geometry);

	bool renderer_renderpass_begin(renderpass_type type, uint render_target_index);
	bool renderer_renderpass_end();
	void renderer_renderpass_set_render_area(renderpass_type type, glm::vec4 render_area);


	void renderer_set_and_apply_uniforms(std::vector<quad_properties>& quads, std::vector<point_light_definition>& point_lights, glm::vec4 ambient_color, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint quad_count, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);
	void renderer_draw_geometry(uint instance_count, geometry& geometry);
	void renderer_draw_object_pick(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view);
}