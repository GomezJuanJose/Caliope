#include "vulkan_renderer.h"
#include "cepch.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan//vulkan_buffer.h"

#include "core/cememory.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "platform/platform.h"

#include "math/math_types.inl"


// TODO: Remove and use texture, material and geometry system
#include "systems/resource_system.h"
#include "loaders/resources_types.inl"

namespace caliope {

	void vulkan_renderer_framebuffers_create();
	void vulkan_renderer_framebuffers_destroy();
	void vulkan_renderer_recreate_swapchain();


	void create_vertex_buffer();
	void create_index_buffer();
	void create_descriptor_set_layout();
	void create_uniform_buffers();
	void create_descriptor_pool();
	void create_descriptor_sets();
	void create_texture_image();
	void create_depth_resource();
	void update_uniform_buffer(int current_frame);

	

	static vulkan_context context;

	// TODO: refactor
	const std::vector<vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::vector<uint16> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	typedef struct uniform_buffer_object {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	}uniform_buffer_object;

	// TODO: refactor

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
		if (!vulkan_imageviews_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the swapchain images views");
			return false;
		}

		// Create renderpass
		if (!vulkan_renderpass_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the renderpass");
			return false;
		}

		// Create pipeline
		create_descriptor_set_layout();
		if (!vulkan_pipeline_create(context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the pipeline");
			return false;
		}

		// Create vertex and index buffer
		create_vertex_buffer();
		create_index_buffer();
		create_texture_image();
		create_uniform_buffers();
		create_descriptor_pool();
		create_descriptor_sets();

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

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroyBuffer(context.device.logical_device, context.uniform_buffers[i], nullptr);
			vkFreeMemory(context.device.logical_device, context.uniform_buffers_memory[i], nullptr);
		}

		vkDestroySampler(context.device.logical_device, context.texture_sampler, nullptr);
		vkDestroyImageView(context.device.logical_device, context.texture_image_view, nullptr);
		vkDestroyImage(context.device.logical_device, context.texture_image, nullptr);
		vkFreeMemory(context.device.logical_device, context.texture_image_memory, nullptr);

		vkDestroyImageView(context.device.logical_device, context.depth_image_view, nullptr);
		vkDestroyImage(context.device.logical_device, context.depth_image, nullptr);
		vkFreeMemory(context.device.logical_device, context.depth_image_memory, nullptr);

		vkDestroyBuffer(context.device.logical_device, context.vertex_buffer.handle, nullptr);
		vkFreeMemory(context.device.logical_device, context.vertex_buffer.memory, nullptr);
		vkDestroyBuffer(context.device.logical_device, context.index_buffer.handle, nullptr);
		vkFreeMemory(context.device.logical_device, context.index_buffer.memory, nullptr);

		vulkan_renderer_framebuffers_destroy();

		vulkan_pipeline_destroy(context);

		vkDestroyDescriptorPool(context.device.logical_device, context.descriptor_pool, nullptr);

		vkDestroyDescriptorSetLayout(context.device.logical_device, context.descriptor_set_layout, nullptr);

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

		update_uniform_buffer(context.current_frame);

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
				context.swapchain.swapchain_image_views[i],
				context.depth_image_view
			};

			VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			framebuffer_info.renderPass = context.renderpass.handle;
			framebuffer_info.attachmentCount = 2;
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
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(std::any_cast<GLFWwindow*>(platform_system_get_window()), &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(context.device.logical_device);
		
		vulkan_renderer_framebuffers_destroy();
		vulkan_imageview_destroy(context);
		vulkan_swapchain_destroy(context);

		vulkan_swapchain_create(context);
		vulkan_imageviews_create(context);
		create_depth_resource();
		vulkan_renderer_framebuffers_create();
	}



	void create_vertex_buffer() {
		VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

		void* data;
		vkMapMemory(context.device.logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
			copy_memory(data, vertices.data(), buffer_size);
		vkUnmapMemory(context.device.logical_device, staging_buffer_memory);

		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context.vertex_buffer.handle, context.vertex_buffer.memory);
	
		vulkan_buffer_copy(context, staging_buffer, context.vertex_buffer.handle, buffer_size);

		vkDestroyBuffer(context.device.logical_device, staging_buffer, nullptr);
		vkFreeMemory(context.device.logical_device, staging_buffer_memory, nullptr);
	}
	void create_index_buffer() {
		VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

		void* data;
		vkMapMemory(context.device.logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
		copy_memory(data, indices.data(), buffer_size);
		vkUnmapMemory(context.device.logical_device, staging_buffer_memory);

		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context.index_buffer.handle, context.index_buffer.memory);

		vulkan_buffer_copy(context, staging_buffer, context.index_buffer.handle, buffer_size);

		vkDestroyBuffer(context.device.logical_device, staging_buffer, nullptr);
		vkFreeMemory(context.device.logical_device, staging_buffer_memory, nullptr);
	}

	void create_descriptor_set_layout() {
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
		VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();
		VK_CHECK(vkCreateDescriptorSetLayout(context.device.logical_device, &layout_info, nullptr, &context.descriptor_set_layout));
	}
	void create_uniform_buffers() {
		VkDeviceSize buffer_size = sizeof(uniform_buffer_object);

		context.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
		context.uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
		context.uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context.uniform_buffers[i], context.uniform_buffers_memory[i]);
			vkMapMemory(context.device.logical_device, context.uniform_buffers_memory[i], 0, buffer_size, 0, &context.uniform_buffers_mapped[i]);
		}
	}

	void create_descriptor_pool() {
		std::array<VkDescriptorPoolSize, 2> pool_sizes;
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

		VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.poolSizeCount = pool_sizes.size();
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;

		VK_CHECK(vkCreateDescriptorPool(context.device.logical_device, &pool_info, nullptr, &context.descriptor_pool));
	}

	void create_descriptor_sets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, context.descriptor_set_layout);
		VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc_info.descriptorPool = context.descriptor_pool;
		alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		alloc_info.pSetLayouts = layouts.data();

		context.descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
		VK_CHECK(vkAllocateDescriptorSets(context.device.logical_device, &alloc_info, context.descriptor_sets.data()));

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = context.uniform_buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(uniform_buffer_object);

			VkDescriptorImageInfo image_info = {};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = context.texture_image_view;
			image_info.sampler = context.texture_sampler;

			std::array<VkWriteDescriptorSet, 2> descriptor_writes = {};
			descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[0].dstSet = context.descriptor_sets[i];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].pBufferInfo = &buffer_info;
			descriptor_writes[0].pImageInfo = nullptr;
			descriptor_writes[0].pTexelBufferView = nullptr;

			descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[1].dstSet = context.descriptor_sets[i];
			descriptor_writes[1].dstBinding = 1;
			descriptor_writes[1].dstArrayElement = 0;
			descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[1].descriptorCount = 1;
			descriptor_writes[1].pImageInfo = &image_info;

			vkUpdateDescriptorSets(context.device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
		}
	}

	void create_texture_image() {
		//------------------Depth---------------
		create_depth_resource();

		//------------------Color image---------------


		resource image_resource;
		resource_system_load(std::string("dummy_character"), RESOURCE_TYPE_TEXTURE, image_resource);
		image_resource_data image_data = std::any_cast<image_resource_data>(image_resource.data);

		if (!image_data.pixels) {
			CE_LOG_FATAL("Cannot load the texture image");
		}

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		vulkan_buffer_create(context, image_resource.data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
		void* data;
		vkMapMemory(context.device.logical_device, staging_buffer_memory, 0, image_resource.data_size, 0, &data);
		copy_memory(data, image_data.pixels, image_resource.data_size);
		vkUnmapMemory(context.device.logical_device, staging_buffer_memory);
		
		resource_system_unload(image_resource);

		vulkan_image_create(context, image_data.width, image_data.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context.texture_image, context.texture_image_memory);


		vulkan_image_transition_layout(context, context.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vulkan_image_copy_buffer_to_image(context, staging_buffer, context.texture_image, image_data.width, image_data.height);

		vulkan_image_transition_layout(context, context.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(context.device.logical_device, staging_buffer, nullptr);
		vkFreeMemory(context.device.logical_device, staging_buffer_memory, nullptr);

		// Create image view
		context.texture_image_view = vulkan_image_view_create(context, context.texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		// Create sampler
		VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(context.device.physical_device, &properties);
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.mipLodBias = 0.0f;
		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = 0.0f;

		VK_CHECK(vkCreateSampler(context.device.logical_device, &sampler_info, nullptr, &context.texture_sampler));
	}

	void create_depth_resource() {
		VkFormat depth_format = find_depth_format(context);
		vulkan_image_create(context, context.swapchain.extent.width, context.swapchain.extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, context.depth_image, context.depth_image_memory);
		context.depth_image_view = vulkan_image_view_create(context, context.depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
		vulkan_image_transition_layout(context, context.depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void update_uniform_buffer(int current_frame) {
		uniform_buffer_object ubo;
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 1920 / (float)1080, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1; // Invert Y to match with Vulkan;

		copy_memory(context.uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
	}






}