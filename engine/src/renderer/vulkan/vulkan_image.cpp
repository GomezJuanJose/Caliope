#include "vulkan_image.h"

// TODO: Remove it
#include "renderer/vulkan/vulkan_buffer.h"
#include "renderer/vulkan/vulkan_command_buffer.h"

namespace caliope {

	bool vulkan_imageviews_create(vulkan_context& context) {
		context.swapchain.swapchain_image_views.resize(context.swapchain.swapchain_images.size());

		for (uint i = 0; i < context.swapchain.swapchain_images.size(); ++i) {
			context.swapchain.swapchain_image_views[i] = vulkan_image_view_create(context, context.swapchain.swapchain_images[i], context.swapchain.surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		return true;
	}

	void vulkan_imageview_destroy(vulkan_context& context) {
		for (VkImageView image_view : context.swapchain.swapchain_image_views) {
			vkDestroyImageView(context.device.logical_device, image_view, nullptr);
		}
	}

	VkImageView vulkan_image_view_create(vulkan_context& context, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) {
		VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		view_info.image = image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = format;
		view_info.subresourceRange.aspectMask = aspect_flags;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;

		VkImageView image_view;
		VK_CHECK(vkCreateImageView(context.device.logical_device, &view_info, nullptr, &image_view));

		return image_view;
	}

	bool vulkan_image_create(vulkan_context& context, uint width, uint height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory) {
		VkImageCreateInfo image_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		image_info.imageType = VK_IMAGE_TYPE_2D;
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

		VK_CHECK(vkCreateImage(context.device.logical_device, &image_info, nullptr, &image));

		VkMemoryRequirements mem_requirements;
		vkGetImageMemoryRequirements(context.device.logical_device, image, &mem_requirements);

		VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = find_memory_type(context.device.physical_device, mem_requirements.memoryTypeBits, properties);

		VK_CHECK(vkAllocateMemory(context.device.logical_device, &alloc_info, nullptr, &image_memory));

		vkBindImageMemory(context.device.logical_device, image, image_memory, 0);

		return true;
	}

	bool has_stencil_component(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void vulkan_image_transition_layout(vulkan_context& context, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
		VkCommandBuffer command_buffer = vulkan_command_buffer_single_use_begin(context);// TODO: avoid to copy reference

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
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
			command_buffer,
			source_stage, destrination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		vulkan_command_buffer_single_use_end(context, command_buffer);// TODO: avoid to copy reference
	}

	void vulkan_image_copy_buffer_to_image(vulkan_context& context, VkBuffer buffer, VkImage image, uint width, uint height) {
		VkCommandBuffer command_buffer = vulkan_command_buffer_single_use_begin(context);

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
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		vulkan_command_buffer_single_use_end(context, command_buffer);
	}
}