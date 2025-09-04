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

	bool vulkan_renderer_begin_renderpass();
	bool vulkan_renderer_end_renderpass();

	void vulkan_renderer_set_and_apply_uniforms(std::vector<quad_properties>& sprites, std::vector<point_light_definition>& point_lights, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint quad_count, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

	void vulkan_renderer_texture_create(texture& t, uchar* pixels);
	void vulkan_renderer_texture_destroy(texture& t);
	void vulkan_renderer_texture_change_filter(texture& t);

	void vulkan_renderer_shader_create(shader& s);
	void vulkan_renderer_shader_destroy(shader& s);
	void vulkan_renderer_shader_use(shader& s);

	void vulkan_renderer_geometry_create(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices);
	void vulkan_renderer_geometry_destroy(geometry& geometry);


	//TODO: TEMPORAL
	void pick_object(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view);
	void show_picked_obj();
	//TODO: END TEMPORAL
}