#pragma once

#include <vulkan/vulkan.h>

#include "core/logger.h"

#define VK_CHECK(expr)								\
	{												\
		if (expr != VK_SUCCESS) {					\
			CE_LOG_FATAL("Vulkan check failed");	\
		}											\
	}

namespace caliope {
	typedef struct swapchain_support_details {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	}swapchain_support_details;

	typedef struct queue_family_indices {
		std::optional<uint> graphics_family;
		std::optional<uint> present_family;
	} queue_family_indices;

	typedef struct vulkan_device {
		VkPhysicalDevice physical_device;
		VkDevice logical_device;

		uint graphics_queue_index;
		uint presentation_queue_index;

		VkQueue graphics_queue;
		VkQueue presentation_queue;

	}vulkan_device;

	typedef struct vulkan_swapchain {
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR presentation_mode;
		VkExtent2D extent;

		VkSwapchainKHR handle;

		std::vector<VkImage> swapchain_images;
		std::vector<VkImageView> swapchain_image_views;

		uint image_count;
	} vulkan_swapchain;

	typedef struct vulkan_context {
		VkInstance instance;
		vulkan_device device;
		swapchain_support_details swapchain_details;

		VkSurfaceKHR surface;

		vulkan_swapchain swapchain;

	} vulkan_context;
}