#include "vulkan_buffer.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_utils.h"

#include "core/cememory.h"

namespace caliope {



	bool vulkan_buffer_create(vulkan_context& context, uint64 size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, bool bind_on_create, vulkan_buffer& out_buffer) {

		out_buffer.memory_property_flag = properties;
		out_buffer.usage = usage;

		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		VK_CHECK(vkCreateBuffer(context.device.logical_device, &buffer_info, nullptr, &out_buffer.handle));

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(context.device.logical_device, out_buffer.handle, &memory_requirements);

		VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		alloc_info.allocationSize = memory_requirements.size;
		alloc_info.memoryTypeIndex = find_memory_type(context.device.physical_device, memory_requirements.memoryTypeBits, out_buffer.memory_property_flag);

		VK_CHECK(vkAllocateMemory(context.device.logical_device, &alloc_info, nullptr, &out_buffer.memory));
				
		if (bind_on_create) {
			vulkan_buffer_bind(context, out_buffer, 0);
		}

		return true;
	}

	void vulkan_buffer_destroy(vulkan_context& context, vulkan_buffer& buffer) {
		vkFreeMemory(context.device.logical_device, buffer.memory, nullptr);
		vkDestroyBuffer(context.device.logical_device, buffer.handle, 0);
	}

	void vulkan_buffer_bind(vulkan_context& context, vulkan_buffer& buffer, uint64 offset) {
		VK_CHECK(vkBindBufferMemory(context.device.logical_device, buffer.handle, buffer.memory, offset));
	}

	void* vulkan_buffer_lock_memory(vulkan_context& context, vulkan_buffer& buffer, uint64 offset, uint64 size, uint flags) {
		void* data;
		VK_CHECK(vkMapMemory(context.device.logical_device, buffer.memory, offset, size, flags, &data));
		return data;
	}

	void vulkan_buffer_unlock_memory(vulkan_context& context, vulkan_buffer& buffer) {
		vkUnmapMemory(context.device.logical_device, buffer.memory);
	}

	void vulkan_buffer_load_data(vulkan_context& context, vulkan_buffer& buffer, uint64 offset, uint64 size, uint flags, const void* data) {
		void* data_ptr;
		VK_CHECK(vkMapMemory(context.device.logical_device, buffer.memory, offset, size, flags, &data_ptr));
		copy_memory(data_ptr, data, size);
		vkUnmapMemory(context.device.logical_device, buffer.memory);
	}

	void vulkan_buffer_read_data(vulkan_context& context, vulkan_buffer& buffer, uint64 offset, uint64 size, uint flags, void* out_data)
	{
		void* data_ptr;
		VK_CHECK(vkMapMemory(context.device.logical_device, buffer.memory, offset, size, flags, &data_ptr));
		copy_memory(out_data, data_ptr, size);
		vkUnmapMemory(context.device.logical_device, buffer.memory);
	}

	bool vulkan_buffer_copy(vulkan_context& context, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer source, uint source_offset, VkBuffer dest, uint64 dest_offset, uint64 size) {
		
		vkQueueWaitIdle(queue);
		vulkan_command_buffer temp_command_buffer;
		vulkan_command_buffer_allocate_and_begin_single_use(context, pool, temp_command_buffer);
		VkBufferCopy copy_region;
		copy_region.srcOffset = source_offset;
		copy_region.dstOffset = dest_offset;
		copy_region.size = size;

		vkCmdCopyBuffer(temp_command_buffer.handle, source, dest, 1, &copy_region);

		// Submit the buffer for execution and wait for it to complete.
		vulkan_command_buffer_end_single_use(context, pool, temp_command_buffer, queue);
		
		return true;
	}
}