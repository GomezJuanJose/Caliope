#include "vulkan_renderer.h"
#include "cepch.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_command_buffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "platform/platform.h"


namespace caliope {

	void vulkan_renderer_framebuffers_create();
	void vulkan_renderer_framebuffers_destroy();
	void vulkan_renderer_recreate_swapchain();


	static vulkan_context context;

	bool vulkan_renderer_backend_initialize(const std::string& application_name) {

		context.command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
		context.image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		context.render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		context.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

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

		// Create renderpass
		if (!vulkan_renderpass_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the renderpass");
			return false;
		}

		// Create pipeline
		if (!vulkan_pipeline_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the pipeline");
			return false;
		}

		// Create framebuffers
		vulkan_renderer_framebuffers_create();

		// Create command buffers
		if (!vulkan_command_buffer_allocate(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not allocate command buffer");
			return false;
		}

		// Create synchronization objects
		VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VK_CHECK(vkCreateSemaphore(context.device.logical_device, &semaphore_info, nullptr, &context.image_available_semaphores[i]));
			VK_CHECK(vkCreateSemaphore(context.device.logical_device, &semaphore_info, nullptr, &context.render_finished_semaphores[i]));
			VK_CHECK(vkCreateFence(context.device.logical_device, &fence_info, nullptr, &context.in_flight_fences[i]));
		}

		CE_LOG_INFO("Vulkan backend initialized.");
		return true;
	}

	

	void vulkan_renderer_backend_shutdown() {

		vkDeviceWaitIdle(context.device.logical_device);

		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroySemaphore(context.device.logical_device, context.image_available_semaphores[i], nullptr);
			vkDestroySemaphore(context.device.logical_device, context.render_finished_semaphores[i], nullptr);
			vkDestroyFence(context.device.logical_device, context.in_flight_fences[i], nullptr);
		}
		vulkan_renderer_framebuffers_destroy();

		vulkan_pipeline_destroy(context);

		vulkan_renderpass_destroy(context);

		vulkan_imageview_destroy(context);

		vulkan_swapchain_destroy(context);

		vulkan_device_destroy(context);

		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);

		vkDestroyInstance(context.instance, nullptr);
	}

	void vulkan_renderer_backend_resize(uint16 width, uint16 height) {
		swapchain_support_details new_support_details = query_swapchain_support(context.device.physical_device, context.surface);
		context.swapchain_details = new_support_details;
		context.framebuffer_resized = true;
	}

	bool vulkan_renderer_begin_frame(float delta_time) {
		
		vkWaitForFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame], VK_TRUE, UINT64_MAX);
		
		uint image_index;
		VkResult result = vkAcquireNextImageKHR(context.device.logical_device, context.swapchain.handle, UINT64_MAX, context.image_available_semaphores[context.current_frame], VK_NULL_HANDLE, &image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			vulkan_renderer_recreate_swapchain();
			return true;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			CE_LOG_FATAL("vulkan_renderer_begin_frame failed to acquire swapchain image");
			return false;
		}

		vkResetFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame]);

		vkResetCommandBuffer(context.command_buffers[context.current_frame].handle, 0);

		vulkan_command_buffer_record(context, context.command_buffers[context.current_frame].handle, image_index);

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkSemaphore wait_semaphores[] = { context.image_available_semaphores[context.current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &context.command_buffers[context.current_frame].handle;
		VkSemaphore signal_sempahores[] = { context.render_finished_semaphores[context.current_frame] };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_sempahores;
		VK_CHECK(vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, context.in_flight_fences[context.current_frame]));

		VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_sempahores;

		VkSwapchainKHR swapchains[] = { context.swapchain.handle };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &image_index;

		result = vkQueuePresentKHR(context.device.presentation_queue, &present_info);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context.framebuffer_resized) {
			vulkan_renderer_recreate_swapchain();
			context.framebuffer_resized = false;
		}
		else if(result != VK_SUCCESS){
			CE_LOG_FATAL("vulkan_renderer_begin_frame failed to present swapchain image");
			return false;
		}

		context.current_frame = (context.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

		return true;
	}

	bool vulkan_renderer_end_frame(float delta_time) {



		return true;
	}


	void vulkan_renderer_framebuffers_create() {
		context.swapchain.framebuffers.resize(context.swapchain.swapchain_image_views.size());
		for (int i = 0; i < context.swapchain.swapchain_image_views.size(); ++i) {
			VkImageView attachments[] = {
				context.swapchain.swapchain_image_views[i]
			};

			VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			framebuffer_info.renderPass = context.renderpass.handle;
			framebuffer_info.attachmentCount = 1;
			framebuffer_info.pAttachments = attachments;
			framebuffer_info.width = context.swapchain.extent.width;
			framebuffer_info.height = context.swapchain.extent.height;
			framebuffer_info.layers = 1;

			VK_CHECK(vkCreateFramebuffer(context.device.logical_device, &framebuffer_info, nullptr, &context.swapchain.framebuffers[i]));
		}
	}

	void vulkan_renderer_framebuffers_destroy() {

		for (int i = 0; i < context.swapchain.framebuffers.size(); ++i) {
			vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], nullptr);
		}
	}

	void vulkan_renderer_recreate_swapchain() {
		vkDeviceWaitIdle(context.device.logical_device);
		
		vulkan_renderer_framebuffers_destroy();
		vulkan_imageview_destroy(context);
		vulkan_swapchain_destroy(context);

		vulkan_swapchain_create(context);
		vulkan_imageview_create(context);
		vulkan_renderer_framebuffers_create();
	}
}