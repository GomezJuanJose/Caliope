#include "vulkan_image.h"
#include "renderer/vulkan/vulkan_utils.h"


namespace caliope {
	bool has_stencil_component(VkFormat format);

	bool vulkan_image_create(vulkan_context& context,
		VkImageType image_type,
		uint width,
		uint height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		bool create_view,
		VkImageAspectFlags view_aspect_flags,
		vulkan_image& out_image
	) {
		out_image.width = width;
		out_image.height = height;

		VkImageCreateInfo image_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		image_info.imageType = image_type;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateImage(context.device.logical_device, &image_info, nullptr, &out_image.handle));

		VkMemoryRequirements mem_requirements;
		vkGetImageMemoryRequirements(context.device.logical_device, out_image.handle, &mem_requirements);

		VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = find_memory_type(context.device.physical_device, mem_requirements.memoryTypeBits, properties);
		VK_CHECK(vkAllocateMemory(context.device.logical_device, &alloc_info, nullptr, &out_image.memory));

		VK_CHECK(vkBindImageMemory(context.device.logical_device, out_image.handle, out_image.memory, 0));

		if (create_view) {
			vulkan_image_view_create(context, format, out_image, view_aspect_flags);
		}

		return true;
	}

	void vulkan_image_view_create(
		vulkan_context& context,
		VkFormat format,
		vulkan_image& image,
		VkImageAspectFlags aspect_flags
	) {
		VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		view_info.image = image.handle;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = format;
		view_info.subresourceRange.aspectMask = aspect_flags;

		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;


		VK_CHECK(vkCreateImageView(context.device.logical_device, &view_info, nullptr, &image.view));
	}


	void vulkan_image_transition_layout(
		vulkan_context& context,
		vulkan_command_buffer& command_buffer,
		vulkan_image& image,
		VkFormat format,
		VkImageLayout old_layout,
		VkImageLayout new_layout
	) {
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;
		barrier.srcQueueFamilyIndex = context.device.graphics_queue_index;
		barrier.dstQueueFamilyIndex = context.device.graphics_queue_index;
		barrier.image = image.handle;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		VkPipelineStageFlags source_stage;
		VkPipelineStageFlags destrination_stage;

		if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (has_stencil_component(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destrination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		
		}
		else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destrination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		
		}
		else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destrination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			CE_LOG_FATAL("Unsupported layout transition");
		}

		vkCmdPipelineBarrier(
			command_buffer.handle,
			source_stage, destrination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	void vulkan_image_copy_buffer_to_image(
		vulkan_context& context,
		vulkan_image& image,
		VkBuffer buffer,
		vulkan_command_buffer& command_buffer
	) {

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			image.width,
			image.height,
			1
		};

		vkCmdCopyBufferToImage(command_buffer.handle, buffer, image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	}

	void vulkan_image_destroy(
		vulkan_context& context,
		vulkan_image& image
	) {
		vkDestroyImageView(context.device.logical_device, image.view, nullptr);
		vkFreeMemory(context.device.logical_device, image.memory, nullptr);
		vkDestroyImage(context.device.logical_device, image.handle, nullptr);
	}

	bool has_stencil_component(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}