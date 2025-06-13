#include "vulkan_command_buffer.h"

namespace caliope {
	bool vulkan_command_buffer_allocate(vulkan_context& context) {
		
		VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.commandPool = context.device.command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VK_CHECK(vkAllocateCommandBuffers(context.device.logical_device, &alloc_info, &context.command_buffers[i].handle));
		}
		return true;
	}

	void vulkan_command_buffer_record(vulkan_context& context, VkCommandBuffer command_buffer, uint image_index) {
		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

		VkRenderPassBeginInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderpass_info.renderPass = context.renderpass.handle;
		renderpass_info.framebuffer = context.swapchain.framebuffers[image_index];
		renderpass_info.renderArea.offset = { 0, 0 };
		renderpass_info.renderArea.extent = context.swapchain.extent;
		VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderpass_info.clearValueCount = 1;
		renderpass_info.pClearValues = &clear_color;

		vkCmdBeginRenderPass(command_buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline.handle);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(context.swapchain.extent.width);
		viewport.height = static_cast<float>(context.swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = context.swapchain.extent;
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		VkBuffer vertex_buffers[] = { context.vertex_buffer.handle };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, context.index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.pipeline.layout, 0, 1, &context.descriptor_sets[context.current_frame], 0, nullptr);

		vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0); // HARDCODED VERTEX NUMBER
		
		vkCmdEndRenderPass(command_buffer);

		VK_CHECK(vkEndCommandBuffer(command_buffer));
	
	}
}