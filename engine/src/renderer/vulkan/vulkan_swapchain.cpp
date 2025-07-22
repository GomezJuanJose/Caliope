#include "vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_device.h"
#include "platform/platform.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>



namespace caliope {
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, bool force_recalculation);

	void create(vulkan_context* context, vulkan_swapchain* out_swapchain);
	void destroy(vulkan_context* context, vulkan_swapchain* out_swapchain);

	bool vulkan_swapchain_create(vulkan_context& context, vulkan_swapchain& out_swapchain) {
		create(&context, &out_swapchain);
		return true;
	}

	void vulkan_swapchain_recreate(vulkan_context& context, vulkan_swapchain& swapchain) {
		destroy(&context, &swapchain);
		create(&context, &swapchain);
	}

	void vulkan_swapchain_destroy(vulkan_context& context, vulkan_swapchain& swapchain) {
		destroy(&context, &swapchain);
	}

	VkResult vulkan_swapchain_acquire_next_image_index(vulkan_context& context, vulkan_swapchain& swapchain, uint64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, uint& out_image_index) {
		VkResult result = vkAcquireNextImageKHR(context.device.logical_device, swapchain.handle, timeout_ns, image_available_semaphore, fence, &out_image_index);

		return result;
	}

	VkResult vulkan_swapchain_present(vulkan_context& context, vulkan_swapchain& swapchain, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint present_image_index) {
		VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &render_complete_semaphore;

		VkSwapchainKHR swapchains[] = { swapchain.handle };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &present_image_index;

		VkResult result = vkQueuePresentKHR(present_queue, &present_info);

		context.current_frame = (context.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}



	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, bool force_recalculation) {
		if (!force_recalculation && capabilities.currentExtent.width != std::numeric_limits<uint>::max()) {
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


	void create(vulkan_context* context, vulkan_swapchain* out_swapchain) {

		// Select a format
		bool found_format = false;
		for (const VkSurfaceFormatKHR& available_format : context->device.swapchain_support_details.formats) {
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				out_swapchain->surface_format = available_format;
				found_format = true;
				break;
			}
		}

		if (!found_format) {
			out_swapchain->surface_format = context->device.swapchain_support_details.formats[0];
		}


		out_swapchain->presentation_mode = VK_PRESENT_MODE_FIFO_KHR;
		// TODO: Make it configurable in the future? is necessary? at the end the games made with this engine will not require much frames and its wanted to export games to mobile
		// which with mailbox its not recomended due to high battery consumtion
		/*for (const VkPresentModeKHR& available_presentation_mode : context->device.swapchain_support_details.present_modes) {
			if (available_presentation_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				out_swapchain->presentation_mode = available_presentation_mode;
				break;
			}
		}*/

		out_swapchain->extent = choose_swap_extent(context->device.swapchain_support_details.capabilities, context->framebuffer_resized);

		out_swapchain->image_count = context->device.swapchain_support_details.capabilities.minImageCount + 1;
		// Avoids to suprass the maximum number of image counts (0 means no maximum)
		if (context->device.swapchain_support_details.capabilities.maxImageCount > 0 && out_swapchain->image_count > context->device.swapchain_support_details.capabilities.maxImageCount) {
			out_swapchain->image_count = context->device.swapchain_support_details.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		create_info.surface = context->surface;
		create_info.minImageCount = out_swapchain->image_count;
		create_info.imageFormat = out_swapchain->surface_format.format;
		create_info.imageColorSpace = out_swapchain->surface_format.colorSpace;
		create_info.imageExtent = out_swapchain->extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint queque_family_indices[] = { context->device.graphics_queue_index, context->device.presentation_queue_index };
		if (context->device.graphics_queue_index != context->device.presentation_queue_index) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queque_family_indices;
		}
		else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;		// Optional
			create_info.pQueueFamilyIndices = nullptr;	// Optional
		}

		create_info.preTransform = context->device.swapchain_support_details.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = out_swapchain->presentation_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device, &create_info, nullptr, &out_swapchain->handle));

		// Images
		uint image_count;
		vkGetSwapchainImagesKHR(context->device.logical_device, out_swapchain->handle, &image_count, nullptr);
		out_swapchain->images.resize(image_count);
		vkGetSwapchainImagesKHR(context->device.logical_device, out_swapchain->handle, &image_count, out_swapchain->images.data());


		// Views
		out_swapchain->views.resize(out_swapchain->images.size());
		for (uint i = 0; i < out_swapchain->images.size(); ++i) {
			VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			view_info.image = out_swapchain->images[i];
			view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_info.format = out_swapchain->surface_format.format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_info.subresourceRange.baseMipLevel= 0;
			view_info.subresourceRange.levelCount = 1;
			view_info.subresourceRange.baseArrayLayer = 0;
			view_info.subresourceRange.layerCount = 1;

			VK_CHECK(vkCreateImageView(context->device.logical_device, &view_info, nullptr, &out_swapchain->views[i]));
		}

		// Depth
		VkFormat depth_format = vulkan_device_check_format_support(*context,
				{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);

		context->device.depth_format = depth_format;
		vulkan_image_create(
			*context,
			VK_IMAGE_TYPE_2D,
			out_swapchain->extent.width,
			out_swapchain->extent.height,
			depth_format, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			true,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			out_swapchain->depth_attachment
		);
		
		/*vulkan_image_transition_layout(
			*context,
			
			context->depth_image, 
			depth_format, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);*/

	}


	void destroy(vulkan_context* context, vulkan_swapchain* swapchain) {
		vkDeviceWaitIdle(context->device.logical_device);

		vulkan_image_destroy(*context, swapchain->depth_attachment);

		for (uint i = 0; i < swapchain->image_count; ++i) {
			vkDestroyImageView(context->device.logical_device, swapchain->views[i], nullptr);
		}

		vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle, nullptr);
	}
}