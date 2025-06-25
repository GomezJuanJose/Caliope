#include "vulkan_command_buffer.h"

namespace caliope {
	bool vulkan_command_buffer_allocate(vulkan_context& context, VkCommandPool pool, bool is_primary, vulkan_command_buffer& out_command_buffer) {
		
		VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.commandPool = pool;
		alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		alloc_info.commandBufferCount = 1;


		VK_CHECK(vkAllocateCommandBuffers(context.device.logical_device, &alloc_info, &out_command_buffer.handle));

		return true;
	}

	void vulkan_command_buffer_free(vulkan_context& context, VkCommandPool pool, vulkan_command_buffer& command_buffer) {
		vkFreeCommandBuffers(context.device.logical_device, pool, 1, &command_buffer.handle);
	}

	void vulkan_command_buffer_begin(vulkan_command_buffer& command_buffer, bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use) {

		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = 0;

		if (is_single_use) {
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		}
		if (is_renderpass_continue) {
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		}
		if (is_simultaneous_use) {
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		}

		vkBeginCommandBuffer(command_buffer.handle, &begin_info);
	}

	void vulkan_command_buffer_end(vulkan_command_buffer& command_buffer) {
		VK_CHECK(vkEndCommandBuffer(command_buffer.handle));
	}

	void vulkan_command_buffer_allocate_and_begin_single_use(vulkan_context& context, VkCommandPool pool, vulkan_command_buffer& out_command_buffer) {
		vulkan_command_buffer_allocate(context, pool, true, out_command_buffer);
		vulkan_command_buffer_begin(out_command_buffer, true, false, false);
	}

	void vulkan_command_buffer_end_single_use(vulkan_context& context, VkCommandPool pool, vulkan_command_buffer& command_buffer, VkQueue queue) {
		vulkan_command_buffer_end(command_buffer);

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer.handle;
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
		VK_CHECK(vkQueueWaitIdle(queue));

		vulkan_command_buffer_free(context, pool, command_buffer);
	}

}