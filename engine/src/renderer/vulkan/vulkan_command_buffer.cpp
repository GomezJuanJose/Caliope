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
		std::array<VkClearValue, 2> clear_values;
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};
		renderpass_info.clearValueCount = clear_values.size();
		renderpass_info.pClearValues = clear_values.data();

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

		vkCmdDrawIndexed(command_buffer, 12, 1, 0, 0, 0); // HARDCODED VERTEX NUMBER
		
		vkCmdEndRenderPass(command_buffer);

		VK_CHECK(vkEndCommandBuffer(command_buffer));
	
	}

	VkCommandBuffer vulkan_command_buffer_single_use_begin(vulkan_context& context) {
		VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = context.device.command_pool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(context.device.logical_device, &alloc_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(command_buffer, &begin_info);

		return command_buffer;
	}

	void vulkan_command_buffer_single_use_end(vulkan_context& context, VkCommandBuffer command_buffer) {

		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(context.device.graphics_queue);

		vkFreeCommandBuffers(context.device.logical_device, context.device.command_pool, 1, &command_buffer);
	}
}