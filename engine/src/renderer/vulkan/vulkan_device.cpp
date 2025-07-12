#include "vulkan_device.h"
#include "cepch.h"

namespace caliope {



	#define DEVICE_EXTENSIONS_COUNT 1
	const char* device_extensions[DEVICE_EXTENSIONS_COUNT] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};


	bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool check_device_extension_support(VkPhysicalDevice device);
	queue_family_indices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);

	bool vulkan_device_create(vulkan_context& context) {

		//Selects a physical device
		context.device.physical_device = VK_NULL_HANDLE;

		uint device_count = 0;
		vkEnumeratePhysicalDevices(context.instance, &device_count, nullptr);

		if (device_count == 0) {
			CE_LOG_ERROR("Failed to find devices with Vulkan support");
			return false;
		}

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(context.instance, &device_count, devices.data());

		for (const VkPhysicalDevice& device : devices) {
			if (is_device_suitable(device, context.surface)) {
				context.device.physical_device = device;
				break;
			}
		}

		if (context.device.physical_device == VK_NULL_HANDLE) {
			CE_LOG_ERROR("Failed to find a device which meets the requirements");
			return false;
		}

		swapchain_support_details swapchain_support = vulkan_device_query_swapchain_support(context.device.physical_device, context.surface);
		context.device.swapchain_support_details = swapchain_support;

		// Creates logical device
		queue_family_indices indices = find_queue_families(context.device.physical_device, context.surface);

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };
		context.device.graphics_queue_index = indices.graphics_family.value();
		context.device.presentation_queue_index = indices.present_family.value();
		float queue_priority = 1.0f;

		for (uint queue_family : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queue_create_info.queueFamilyIndex = queue_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}
		
		VkPhysicalDeviceFeatures device_features = {};
		device_features.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = queue_create_infos.size();
		create_info.pEnabledFeatures = &device_features;
		create_info.enabledExtensionCount = DEVICE_EXTENSIONS_COUNT;
		create_info.ppEnabledExtensionNames = device_extensions;

		VK_CHECK(vkCreateDevice(context.device.physical_device, &create_info, nullptr, &context.device.logical_device));

		vkGetDeviceQueue(context.device.logical_device, indices.graphics_family.value(), 0, &context.device.graphics_queue);
		vkGetDeviceQueue(context.device.logical_device, indices.present_family.value(), 0, &context.device.presentation_queue);

		// Creates the command pool for the device
		VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = context.device.graphics_queue_index;
		VK_CHECK(vkCreateCommandPool(context.device.logical_device, &pool_info, nullptr, &context.device.command_pool));

		return true;
	}

	void vulkan_device_destroy(vulkan_context& context) {
		vkDestroyCommandPool(context.device.logical_device, context.device.command_pool, nullptr);
		vkDestroyDevice(context.device.logical_device, nullptr);
	}


	bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
		// TODO: Make a better score based device pick, and pick a integrated GPU if thats the only available

		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures(device, &device_features);

		queue_family_indices indices = find_queue_families(device, surface);

		bool extensions_supported = check_device_extension_support(device);

		bool swapchain_adequate = false;
		if (extensions_supported) {
			swapchain_support_details swapchain_support = vulkan_device_query_swapchain_support(device, surface);
			swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
		}

		return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader
			&& indices.graphics_family.has_value() && indices.present_family.has_value()
			&& extensions_supported && swapchain_adequate && device_features.samplerAnisotropy;
	}

	bool check_device_extension_support(VkPhysicalDevice device) {

		uint extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions;
		for (uint i = 0; i < DEVICE_EXTENSIONS_COUNT; ++i) {
			required_extensions.insert(device_extensions[i]);
		}

		for (const VkExtensionProperties& extension : available_extensions) {
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	swapchain_support_details vulkan_device_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
		swapchain_support_details details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
		if (format_count != 0) {
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
		}

		uint present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
		if (present_mode_count != 0) {
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
		}

		return details;
	}

	queue_family_indices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
		queue_family_indices indices;

		uint queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		for (int i = 0; i < queue_family_count; ++i) {
			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
			if (present_support) {
				indices.present_family = i;
			}

			if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics_family = i;
				break;
			}
		}

		return indices;
	}

	VkFormat vulkan_device_check_format_support(vulkan_context& context, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(context.device.physical_device, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		return VkFormat();
	}
}