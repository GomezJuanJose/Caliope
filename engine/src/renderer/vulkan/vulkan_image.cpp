#include "vulkan_image.h"

namespace caliope {

	bool vulkan_imageview_create(vulkan_context& context) {
		context.swapchain.swapchain_image_views.resize(context.swapchain.swapchain_images.size());

		for (uint i = 0; i < context.swapchain.swapchain_images.size(); ++i) {
			VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			create_info.image = context.swapchain.swapchain_images[i];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = context.swapchain.surface_format.format;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(context.device.logical_device, &create_info, nullptr, &context.swapchain.swapchain_image_views[i]));
		}

		return true;
	}

	void vulkan_imageview_destroy(vulkan_context& context) {
		for (VkImageView image_view : context.swapchain.swapchain_image_views) {
			vkDestroyImageView(context.device.logical_device, image_view, nullptr);
		}
	}
}