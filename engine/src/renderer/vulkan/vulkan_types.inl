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

	typedef struct vulkan_device {
		VkPhysicalDevice physical_device;
		VkDevice logical_device;

		VkQueue graphics_queue;

	}vulkan_device;

	typedef struct vulkan_context {
		VkInstance instance;
		vulkan_device device;

	} vulkan_context;
}