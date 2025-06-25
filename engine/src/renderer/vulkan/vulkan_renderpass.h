#pragma once

#include "defines.h"

#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_renderpass_create(vulkan_context& context, vulkan_renderpass& out_renderpass, glm::vec4 render_area, glm::vec4 clear_color, float depth, uint stencil, bool has_prev_pass, bool has_next_pass);
	void vulkan_renderpass_destroy(vulkan_context& context, vulkan_renderpass& renderpass);

	void vulkan_renderpass_begin(vulkan_command_buffer& command_buffer, vulkan_renderpass& renderpass, VkFramebuffer frame_buffer);
	void vulkan_renderpass_end(vulkan_command_buffer& command_buffer);

}