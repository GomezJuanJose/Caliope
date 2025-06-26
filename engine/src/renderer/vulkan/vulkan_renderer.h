#pragma once
#include "renderer/renderer_types.inl"

#include <string>

namespace caliope {
	bool vulkan_renderer_backend_initialize(const std::string& application_name);
	void create_framebuffers();
	void vulkan_renderer_backend_shutdown();
	void destroy_framebuffers();
	void vulkan_renderer_backend_resize(uint16 width, uint16 height);

	bool vulkan_renderer_begin_frame(float delta_time);
	bool vulkan_renderer_end_frame(float delta_time);

	void vulkan_renderer_draw_geometry();

	bool vulkan_renderer_begin_renderpass();
	bool vulkan_renderer_end_renderpass();

	void vulkan_renderer_set_and_apply_uniforms(std::shared_ptr<material>& m, glm::mat4& model, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position);

	void vulkan_renderer_texture_create(texture& t, uchar* pixels);
	void vulkan_renderer_texture_destroy(texture& t);

	void vulkan_renderer_shader_create(shader& s);
	void vulkan_renderer_shader_destroy(shader& s);
	void vulkan_renderer_shader_use(shader& s);
}