#include "vulkan_device.h"
#include "cepch.h"

namespace caliope {

	struct queue_family_indices {
		std::optional<uint> graphics_family;
	};


	bool is_device_suitable(VkPhysicalDevice device);
	queue_family_indices find_queue_families(VkPhysicalDevice device);

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
			if (is_device_suitable(device)) {
				context.device.physical_device = device;
				break;
			}
		}

		if (context.device.physical_device == VK_NULL_HANDLE) {
			CE_LOG_ERROR("Failed to find a device which meets the requirements");
			return false;
		}
 
		// Creates logical device
		queue_family_indices indices = find_queue_families(context.device.physical_device);

		VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info.queueFamilyIndex = indices.graphics_family.value();
		queue_create_info.queueCount = 1;
		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures device_features = {};

		VkDeviceCreateInfo create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		create_info.pQueueCreateInfos = &queue_create_info;
		create_info.queueCreateInfoCount = 1;
		create_info.pEnabledFeatures = &device_features;
		create_info.enabledExtensionCount = 0;

		VK_CHECK(vkCreateDevice(context.device.physical_device, &create_info, nullptr, &context.device.logical_device));

		vkGetDeviceQueue(context.device.logical_device, indices.graphics_family.value(), 0, &context.device.graphics_queue);

		return true;
	}

	void vulkan_device_destroy(vulkan_context& context) {
		vkDestroyDevice(context.device.logical_device, nullptr);
	}


	bool is_device_suitable(VkPhysicalDevice device) {
		// TODO: Make a better score based device pick, and pick a integrated GPU if thats the only available

		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures(device, &device_features);

		queue_family_indices indices = find_queue_families(device);

		return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader && indices.graphics_family.has_value();
	}

	queue_family_indices find_queue_families(VkPhysicalDevice device) {
		queue_family_indices indices;

		uint queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

		for (int i = 0; i < queue_family_count; ++i) {
			if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics_family = i;
				break;
			}
		}

		return indices;
	}
}