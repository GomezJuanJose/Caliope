#include "vulkan_renderer.h"
#include "cepch.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_image.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "platform/platform.h"


namespace caliope {

	static vulkan_context context;

	bool vulkan_renderer_backend_initialize(const std::string& application_name) {

		// Create vulkan instancce
		VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		app_info.pApplicationName = application_name.c_str();
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Caliope";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		create_info.pApplicationInfo = &app_info;

		uint glfw_extension_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		create_info.enabledExtensionCount = glfw_extension_count;
		create_info.ppEnabledExtensionNames = glfw_extensions;
		create_info.enabledLayerCount = 0;

#ifdef CE_DEBUG
		const std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		bool found_layers = true;

		uint layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : validation_layers) {
			bool layer_found = false;

			for (VkLayerProperties& layer_properties : available_layers) {
				std::string str(layer_name);
				if (str.compare(layer_properties.layerName) == 0) {
					layer_found = true;
					break;
				}
			}

			if (!layer_found) {
				found_layers = false;
				break;
			}
		}

		if (found_layers) {
			create_info.enabledLayerCount = validation_layers.size();
			create_info.ppEnabledLayerNames = validation_layers.data();
		}

#endif // CE_DEBUG

		VK_CHECK(vkCreateInstance(&create_info, nullptr, &context.instance));


		// Create surface
		if (glfwCreateWindowSurface(context.instance, std::any_cast<GLFWwindow*>(platform_system_get_window()), nullptr, &context.surface)) {
			CE_LOG_FATAL("Could not create a window surface");
			return false;
		}


		// Create device
		if (!vulkan_device_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create a device");
			return false;
		}

		// Create swapchain
		if (!vulkan_swapchain_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the swapchain");
			return false;
		}

		// Create swapchain images views
		if (!vulkan_imageview_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the swapchain images views");
			return false;
		}

		CE_LOG_INFO("Vulkan backend initialized.");
		return true;
	}

	void vulkan_renderer_backend_shutdown() {

		vulkan_imageview_destroy(context);

		vulkan_swapchain_destroy(context);

		vulkan_device_destroy(context);

		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);

		vkDestroyInstance(context.instance, nullptr);
	}

	void vulkan_renderer_backend_resize(uint16 width, uint16 height) {
	}

	bool vulkan_renderer_begin_frame(float delta_time) {
		return true;
	}

	bool vulkan_renderer_end_frame(float delta_time) {
		return true;
	}
}