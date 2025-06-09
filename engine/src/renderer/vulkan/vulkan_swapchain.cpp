#include "vulkan_swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "platform/platform.h"

namespace caliope {

	VkSurfaceFormatKHR choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
	VkPresentModeKHR choose_swapchain_presentation_mode(const std::vector<VkPresentModeKHR>& available_presentation_modes);
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

	bool vulkan_swapchain_create(vulkan_context& context) {

		context.swapchain.surface_format = choose_swapchain_format(context.swapchain_details.formats);
		context.swapchain.presentation_mode = choose_swapchain_presentation_mode(context.swapchain_details.present_modes);
		context.swapchain.extent = choose_swap_extent(context.swapchain_details.capabilities);
		context.swapchain.image_count = context.swapchain_details.capabilities.minImageCount + 1;
		// Avoids to suprass the maximum number of image counts (0 means no maximum)
		if (context.swapchain_details.capabilities.maxImageCount > 0 && context.swapchain.image_count > context.swapchain_details.capabilities.maxImageCount) {
			context.swapchain.image_count = context.swapchain_details.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		create_info.surface = context.surface;
		create_info.minImageCount = context.swapchain.image_count;
		create_info.imageFormat = context.swapchain.surface_format.format;
		create_info.imageColorSpace = context.swapchain.surface_format.colorSpace;
		create_info.imageExtent = context.swapchain.extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint queque_family_indices[] = {context.device.graphics_queue_index, context.device.presentation_queue_index };
		if (context.device.graphics_queue_index != context.device.presentation_queue_index) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queque_family_indices;
		}
		else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;		// Optional
			create_info.pQueueFamilyIndices = nullptr;	// Optional
		}

		create_info.preTransform = context.swapchain_details.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = context.swapchain.presentation_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(context.device.logical_device, &create_info, nullptr, &context.swapchain.handle));

		uint image_count;
		vkGetSwapchainImagesKHR(context.device.logical_device, context.swapchain.handle, &image_count, nullptr);
		context.swapchain.swapchain_images.resize(image_count);
		vkGetSwapchainImagesKHR(context.device.logical_device, context.swapchain.handle, &image_count, context.swapchain.swapchain_images.data());

		return true;
	}

	void vulkan_swapchain_destroy(vulkan_context& context) {
		vkDestroySwapchainKHR(context.device.logical_device, context.swapchain.handle, nullptr);
	}

	VkSurfaceFormatKHR choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for (const VkSurfaceFormatKHR& available_format : available_formats) {
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return available_format;
			}
		}

		return available_formats[0];
	}

	VkPresentModeKHR choose_swapchain_presentation_mode(const std::vector<VkPresentModeKHR>& available_presentation_modes) {
		for (const VkPresentModeKHR& available_presentation_mode : available_presentation_modes) {
			if (available_presentation_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return available_presentation_mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(std::any_cast<GLFWwindow*>(platform_system_get_window()), &width, &height);

			VkExtent2D actual_extent = {
				width,
				height
			};

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}
}