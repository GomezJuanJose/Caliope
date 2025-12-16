#pragma once

#include "defines.h"

#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_pipeline_create(
		vulkan_context& context,
		vulkan_renderpass& renderpass, 
		uint vertex_stride, 
		uint attribute_count, 
		VkVertexInputAttributeDescription* attributes,
		uint descriptor_set_layout_count,
		VkDescriptorSetLayout& descriptor_set_layouts,
		uint stage_count,
		VkPipelineShaderStageCreateInfo* stages,
		VkViewport viewport,
		VkRect2D scissor,
		VkFrontFace front_face,
		bool depth_test_enabled,
		vulkan_pipeline& out_pipeline
		);

	void vulkan_pipeline_destroy(vulkan_context& context, vulkan_pipeline& pipeline);

	void vulkan_pipeline_bind(vulkan_command_buffer& command_buffer, VkPipelineBindPoint bind_point, vulkan_pipeline& pipeline);
}