#include "renderer/vulkan/vulkan_buffer.h"

namespace caliope {

	int find_memory_type(VkPhysicalDevice& device, int type_filter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

		for (int i = 0; memory_properties.memoryTypeCount; ++i) {
			if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		CE_LOG_FATAL("find_memory_type failed to find suitable memory type, returning -1.");
		return -1;
	}

	bool vulkan_buffer_create(vulkan_context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(context.device.logical_device, &buffer_info, nullptr, &buffer));

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(context.device.logical_device, buffer, &memory_requirements);

		VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		alloc_info.allocationSize = memory_requirements.size;
		alloc_info.memoryTypeIndex = find_memory_type(context.device.physical_device, memory_requirements.memoryTypeBits, properties);

		VK_CHECK(vkAllocateMemory(context.device.logical_device, &alloc_info, nullptr, &buffer_memory));
		vkBindBufferMemory(context.device.logical_device, buffer, buffer_memory, 0);
		
		return true;
	}

	bool vulkan_buffer_copy(vulkan_context& context, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = context.device.command_pool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(context.device.logical_device, &alloc_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(command_buffer, &begin_info);

		VkBufferCopy copy_region = {};
		copy_region.srcOffset = 0;
		copy_region.dstOffset = 0;
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(context.device.graphics_queue);

		vkFreeCommandBuffers(context.device.logical_device, context.device.command_pool, 1, &command_buffer);
		
		return true;
	}
}