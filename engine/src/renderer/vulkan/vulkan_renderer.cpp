#include "vulkan_renderer.h"
#include "cepch.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "renderer/vulkan/vulkan_command_buffer.h"
#include "renderer/vulkan/vulkan_buffer.h"

#include "core/cememory.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>



#include "platform/platform.h"

#include "math/math_types.inl"


#include "loaders/resources_types.inl"
#include "systems/resource_system.h"



namespace caliope {

	void create_framebuffers();
	void destroy_framebuffers();
	void recreate_swapchain();
	void create_command_buffers();
	VkShaderModule create_shader_module(std::string& file_path);

	

	typedef struct uniform_buffer_object {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 ambient_color;
		glm::vec3 view_position;
	}uniform_buffer_object;

	typedef struct vulkan_backend_state {
		vulkan_context context;
		uint max_number_quads;//TODO: REFACTOR in a internal state
		uint max_textures_per_batch;
	}vulkan_backend_state;

	static std::unique_ptr<vulkan_backend_state> state_ptr;

	bool vulkan_renderer_backend_initialize(const renderer_backend_config& config) {

		state_ptr = std::make_unique<vulkan_backend_state>();

		if (!state_ptr) {
			return false;
		}

		// TODO: PRE-LOAD THE KNOWN SHADERS WHEN INITIALIZE
		state_ptr->max_number_quads = config.max_quads;
		state_ptr->max_textures_per_batch = config.max_textures_per_batch;

		state_ptr->context.image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		state_ptr->context.render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		state_ptr->context.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

		state_ptr->context.batch_image_infos.resize(state_ptr->max_textures_per_batch);

		// Create vulkan instancce
		VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		app_info.pApplicationName = config.application_name.c_str();
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

		VK_CHECK(vkCreateInstance(&create_info, nullptr, &state_ptr->context.instance));


		// Create surface
		if (glfwCreateWindowSurface(state_ptr->context.instance, std::any_cast<GLFWwindow*>(platform_system_get_window()), nullptr, &state_ptr->context.surface)) {
			CE_LOG_FATAL("Could not create a window surface");
			return false;
		}


		// Create device
		if (!vulkan_device_create(state_ptr->context)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create a device");
			return false;
		}

		// Create swapchain
		if (!vulkan_swapchain_create(state_ptr->context, state_ptr->context.swapchain)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the swapchain");
			return false;
		}


		// Create world renderpass
		if (!vulkan_renderpass_create(
			state_ptr->context,
			state_ptr->context.main_renderpass,
			glm::vec4(0.0f, 0.0f, state_ptr->context.swapchain.extent.width, state_ptr->context.swapchain.extent.height),
			glm::vec4(0.0f, 0.2f, 0.6f, 1.0f),
			1.0f,
			0,
			false,
			false
		)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the renderpass");
			return false;
		}

		// Create command buffers
		create_command_buffers();

		// Create framebuffers
		create_framebuffers();

		// Create synchronization objects
		VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VK_CHECK(vkCreateSemaphore(state_ptr->context.device.logical_device, &semaphore_info, nullptr, &state_ptr->context.image_available_semaphores[i]));
			VK_CHECK(vkCreateSemaphore(state_ptr->context.device.logical_device, &semaphore_info, nullptr, &state_ptr->context.render_finished_semaphores[i]));
			VK_CHECK(vkCreateFence(state_ptr->context.device.logical_device, &fence_info, nullptr, &state_ptr->context.in_flight_fences[i]));
		}

		CE_LOG_INFO("Vulkan backend initialized.");
		return true;
	}

	void vulkan_renderer_backend_stop() {
		vkDeviceWaitIdle(state_ptr->context.device.logical_device);
	}

	void vulkan_renderer_backend_shutdown() {

		vkDeviceWaitIdle(state_ptr->context.device.logical_device);

		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroySemaphore(state_ptr->context.device.logical_device, state_ptr->context.image_available_semaphores[i], nullptr);
			vkDestroySemaphore(state_ptr->context.device.logical_device, state_ptr->context.render_finished_semaphores[i], nullptr);
			vkDestroyFence(state_ptr->context.device.logical_device, state_ptr->context.in_flight_fences[i], nullptr);
		}

		destroy_framebuffers();

		for (uint i = 0; i < state_ptr->context.swapchain.image_count; ++i) {
			if (state_ptr->context.command_buffers[i].handle) {
				vulkan_command_buffer_free(state_ptr->context, state_ptr->context.device.command_pool, state_ptr->context.command_buffers[i]);
			}
		}

		vulkan_renderpass_destroy(state_ptr->context, state_ptr->context.main_renderpass);

		vulkan_swapchain_destroy(state_ptr->context, state_ptr->context.swapchain);

		vulkan_device_destroy(state_ptr->context);

		vkDestroySurfaceKHR(state_ptr->context.instance, state_ptr->context.surface, nullptr);

		vkDestroyInstance(state_ptr->context.instance, nullptr);

		state_ptr.reset();
		state_ptr = nullptr;
	}

	void vulkan_renderer_backend_resize(uint16 width, uint16 height) {
		swapchain_support_details new_support_details = vulkan_device_query_swapchain_support(state_ptr->context.device.physical_device, state_ptr->context.surface);
		state_ptr->context.device.swapchain_support_details = new_support_details;
		state_ptr->context.framebuffer_resized = true;
	}

	bool vulkan_renderer_begin_frame(float delta_time) {
		
		vkWaitForFences(state_ptr->context.device.logical_device, 1, &state_ptr->context.in_flight_fences[state_ptr->context.current_frame], VK_TRUE, UINT64_MAX);

		VkResult result = vulkan_swapchain_acquire_next_image_index(state_ptr->context, state_ptr->context.swapchain, UINT64_MAX, state_ptr->context.image_available_semaphores[state_ptr->context.current_frame], VK_NULL_HANDLE, state_ptr->context.image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreate_swapchain();
			return true;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			CE_LOG_FATAL("vulkan_renderer_begin_frame failed to acquire swapchain image");
			return false;
		}

		
		vkResetCommandBuffer(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, 0);

		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;
		VK_CHECK(vkBeginCommandBuffer(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, &begin_info));
		
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(state_ptr->context.swapchain.extent.height);// Invert Y to match with Vulkan;
		viewport.width = static_cast<float>(state_ptr->context.swapchain.extent.width);
		viewport.height = -static_cast<float>(state_ptr->context.swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = state_ptr->context.swapchain.extent;
		vkCmdSetScissor(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, 0, 1, &scissor);

		return true;
	}

	bool vulkan_renderer_end_frame(float delta_time) {

		VK_CHECK(vkEndCommandBuffer(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle));

		vkResetFences(state_ptr->context.device.logical_device, 1, &state_ptr->context.in_flight_fences[state_ptr->context.current_frame]);

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkSemaphore wait_semaphores[] = { state_ptr->context.image_available_semaphores[state_ptr->context.current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &state_ptr->context.command_buffers[state_ptr->context.current_frame].handle;
		VkSemaphore signal_sempahores[] = { state_ptr->context.render_finished_semaphores[state_ptr->context.current_frame] };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_sempahores;
		VK_CHECK(vkQueueSubmit(state_ptr->context.device.graphics_queue, 1, &submit_info, state_ptr->context.in_flight_fences[state_ptr->context.current_frame]));

		VkResult result = vulkan_swapchain_present(state_ptr->context, state_ptr->context.swapchain, state_ptr->context.device.presentation_queue, state_ptr->context.render_finished_semaphores[state_ptr->context.current_frame], state_ptr->context.image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state_ptr->context.framebuffer_resized) {
			recreate_swapchain();
			state_ptr->context.framebuffer_resized = false;
		}
		else if (result != VK_SUCCESS) {
			CE_LOG_FATAL("vulkan_renderer_begin_frame failed to present swapchain image");
		}

		return true;
	}

	void vulkan_renderer_draw_geometry(uint instance_count, geometry& geometry) {
		vulkan_geometry* vk_geometry = std::any_cast<vulkan_geometry>(&geometry.internal_data);

		VkBuffer vertex_buffers[] = { vk_geometry->vertex_buffer.handle };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, vk_geometry->index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);


		vkCmdDrawIndexed(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, geometry.index_count, instance_count, 0, 0, 0);
	}

	bool vulkan_renderer_begin_renderpass() {
		vulkan_renderpass_begin(state_ptr->context.command_buffers[state_ptr->context.current_frame], state_ptr->context.main_renderpass, state_ptr->context.swapchain.framebuffers[state_ptr->context.image_index]);
		return true;
	}

	bool vulkan_renderer_end_renderpass() {
		vulkan_renderpass_end(state_ptr->context.command_buffers[state_ptr->context.current_frame]);
		return true;
	}

	void vulkan_renderer_set_and_apply_uniforms(std::vector<quad_properties>& quads, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint quad_count, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position) {
		uniform_buffer_object ubo;
		ubo.view = view;
		ubo.proj = projection;
		ubo.ambient_color = glm::vec4(0.02f, 0.02f, 0.02f, 1.0f);// TODO: Scene system;
		ubo.view_position = view_position;

		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader_internal_data);
		copy_memory(vk_shader->uniform_buffers_mapped, &ubo, sizeof(ubo));
		copy_memory(vk_shader->ssbo_mapped, quads.data(), sizeof(quad_properties) * quad_count);

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = vk_shader->uniform_buffers.handle;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(uniform_buffer_object);

		VkDescriptorBufferInfo ssbo_info = {};
		ssbo_info.buffer = vk_shader->ssbo.handle;
		ssbo_info.offset = 0;
		ssbo_info.range = sizeof(quad_properties) * state_ptr->max_number_quads;

		std::array<VkWriteDescriptorSet, 3> descriptor_writes = {};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;
		descriptor_writes[0].pImageInfo = nullptr;
		descriptor_writes[0].pTexelBufferView = nullptr;

		uint number_of_texture = 0;
		for (texture* texture : textures_batch_ptr) {
			if (texture == nullptr) {
				break;
			}

			vulkan_texture* vk_texture = std::any_cast<vulkan_texture>(&texture->internal_data);
			state_ptr->context.batch_image_infos[number_of_texture].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			state_ptr->context.batch_image_infos[number_of_texture].imageView = vk_texture->image.view;
			state_ptr->context.batch_image_infos[number_of_texture].sampler = vk_texture->sampler;
			number_of_texture++;
		}

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = number_of_texture;
		descriptor_writes[1].pImageInfo = state_ptr->context.batch_image_infos.data();

		descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[2].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		descriptor_writes[2].dstBinding = 2;
		descriptor_writes[2].dstArrayElement = 0;
		descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[2].descriptorCount = 1;
		descriptor_writes[2].pBufferInfo = &ssbo_info;
		descriptor_writes[2].pImageInfo = nullptr;
		descriptor_writes[2].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(state_ptr->context.device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

		vkCmdBindDescriptorSets(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1, &vk_shader->descriptor_sets[state_ptr->context.current_frame], 0, nullptr);

	}

	void vulkan_renderer_texture_create(texture& t, uchar* pixels) {
		
		t.internal_data = vulkan_texture();
		vulkan_texture* vk_texture = std::any_cast<vulkan_texture>(&t.internal_data);
		VkDeviceSize image_size = t.width * t.height * t.channel_count;

		vulkan_buffer staging_buffer;
		vulkan_buffer_create(
			state_ptr->context,
			image_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true,
			staging_buffer
		);

		vulkan_buffer_load_data(
			state_ptr->context,
			staging_buffer,
			0,
			image_size,
			0,
			pixels
		);


		vulkan_image_create(
			state_ptr->context, 
			VK_IMAGE_TYPE_2D,
			t.width,
			t.height,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true,
			VK_IMAGE_ASPECT_COLOR_BIT,
			vk_texture->image
		);


		vulkan_command_buffer temp_command_buffer;
		vulkan_command_buffer_allocate_and_begin_single_use(
			state_ptr->context,
			state_ptr->context.device.command_pool,
			temp_command_buffer
		);

		vulkan_image_transition_layout(
			state_ptr->context,
			temp_command_buffer,
			vk_texture->image,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);
			
		vulkan_image_copy_buffer_to_image(
			state_ptr->context,
			vk_texture->image,
			staging_buffer.handle,
			temp_command_buffer
		);
		
		vulkan_image_transition_layout(
			state_ptr->context,
			temp_command_buffer,
			vk_texture->image,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		vulkan_command_buffer_end_single_use(
			state_ptr->context,
			state_ptr->context.device.command_pool,
			temp_command_buffer,
			state_ptr->context.device.graphics_queue
		);

		vulkan_buffer_destroy(state_ptr->context, staging_buffer);

		// Create sampler
		VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(state_ptr->context.device.physical_device, &properties);
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

		VK_CHECK(vkCreateSampler(state_ptr->context.device.logical_device, &sampler_info, nullptr, &vk_texture->sampler));

	}

	void vulkan_renderer_texture_destroy(texture& t) {
		vulkan_texture* vk_t = std::any_cast<vulkan_texture>(&t.internal_data);

		vkDestroySampler(state_ptr->context.device.logical_device, vk_t->sampler, nullptr);
		vkDestroyImageView(state_ptr->context.device.logical_device, vk_t->image.view, nullptr);
		vkDestroyImage(state_ptr->context.device.logical_device, vk_t->image.handle, nullptr);
		vkFreeMemory(state_ptr->context.device.logical_device, vk_t->image.memory, nullptr);
	}

	void vulkan_renderer_shader_create(shader& s) {

		VkShaderModule vert_module = create_shader_module(std::string("shaders\\" + s.name + ".vert.spv"));
		VkPipelineShaderStageCreateInfo vert_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_module;
		vert_shader_stage_info.pName = "main";

		VkShaderModule frag_module = create_shader_module(std::string("shaders\\" + s.name + ".frag.spv"));
		VkPipelineShaderStageCreateInfo frag_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = (float)state_ptr->context.swapchain.extent.height;// Invert Y to match with Vulkan;
		viewport.width = (float)state_ptr->context.swapchain.extent.width;
		viewport.height = -(float)state_ptr->context.swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = state_ptr->context.swapchain.extent;

		// Attributes
		#define NUMBER_OF_VERTEX_ATTRIBUTES 3
		std::array < std::pair<VkFormat, uint>, NUMBER_OF_VERTEX_ATTRIBUTES > attributes_definitions{ 
			std::make_pair<VkFormat, uint>(VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)),
			std::make_pair<VkFormat, uint>(VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(glm::vec4)),
			std::make_pair<VkFormat, uint>(VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2))
		};
		std::array<VkVertexInputAttributeDescription, NUMBER_OF_VERTEX_ATTRIBUTES> attribute_descriptions;
		
		uint offset = 0;
		for (uint i = 0; i < NUMBER_OF_VERTEX_ATTRIBUTES; ++i) {
			attribute_descriptions[i].binding = 0;
			attribute_descriptions[i].location = i;
			attribute_descriptions[i].format = attributes_definitions[i].first;
			attribute_descriptions[i].offset = offset;
			offset += attributes_definitions[i].second;

		}

		s.internal_data = vulkan_shader();
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&s.internal_data);

		// Descriptor set layout
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorCount = state_ptr->max_textures_per_batch;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding ssbo_layout_binding = {};
		ssbo_layout_binding.binding = 2;
		ssbo_layout_binding.descriptorCount = 1;// TODO: MORE CONFIGURABLE WITH THE vulkan_renderer_set_and_apply_uniforms
		ssbo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssbo_layout_binding.pImmutableSamplers = nullptr;
		ssbo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = { ubo_layout_binding, sampler_layout_binding, ssbo_layout_binding };
		VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();
		VK_CHECK(vkCreateDescriptorSetLayout(state_ptr->context.device.logical_device, &layout_info, nullptr, &vk_shader->descriptor_set_layout));


		// Create pipeline
		bool result = vulkan_pipeline_create(
			state_ptr->context, 
			state_ptr->context.main_renderpass,//TODO: Choose renderpass depending the shader type
			sizeof(vertex),
			NUMBER_OF_VERTEX_ATTRIBUTES,
			attribute_descriptions.data(),
			1,
			vk_shader->descriptor_set_layout,
			2,
			shader_stages,
			viewport,
			scissor,
			false,
			vk_shader->pipeline
			);

		if (!result) {
			CE_LOG_ERROR("Couldn't create the pipeline for the shader: %s", s.name.c_str());
			return;
		}

		vkDestroyShaderModule(state_ptr->context.device.logical_device, vert_module, nullptr);
		vkDestroyShaderModule(state_ptr->context.device.logical_device, frag_module, nullptr);

		// Uniform buffers
		VkDeviceSize buffer_size = sizeof(uniform_buffer_object);

		vulkan_buffer_create(state_ptr->context, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->uniform_buffers);
		vk_shader->uniform_buffers_mapped = vulkan_buffer_lock_memory(state_ptr->context, vk_shader->uniform_buffers, 0, buffer_size, 0);

		// Vertex SSBO
		vulkan_buffer_create(state_ptr->context, sizeof(quad_properties) * state_ptr->max_number_quads, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->ssbo);
		vk_shader->ssbo_mapped = vulkan_buffer_lock_memory(state_ptr->context, vk_shader->ssbo, 0, sizeof(quad_properties) * state_ptr->max_number_quads, 0);


		// Descriptor pool
		std::array<VkDescriptorPoolSize, 3> pool_sizes;
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1024; // HACK: max number of ubo descriptor sets.
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = 4096; // HACK: max number of image sampler descriptor sets.
		pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[2].descriptorCount = 2;

		VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.poolSizeCount = pool_sizes.size();
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VK_CHECK(vkCreateDescriptorPool(state_ptr->context.device.logical_device, &pool_info, nullptr, &vk_shader->descriptor_pool));

		// Descriptor sets
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vk_shader->descriptor_set_layout);
		VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc_info.descriptorPool = vk_shader->descriptor_pool;
		alloc_info.descriptorSetCount = 2;
		alloc_info.pSetLayouts = layouts.data();

		vk_shader->descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
		VK_CHECK(vkAllocateDescriptorSets(state_ptr->context.device.logical_device, &alloc_info, vk_shader->descriptor_sets.data()));

	}

	void vulkan_renderer_shader_destroy(shader& s) {

		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&s.internal_data);

		//for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->uniform_buffers);
		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->ssbo);
		vk_shader->uniform_buffers_mapped = 0;
		vk_shader->ssbo_mapped = 0;
		vulkan_buffer_destroy(state_ptr->context, vk_shader->uniform_buffers);
		vulkan_buffer_destroy(state_ptr->context, vk_shader->ssbo);
		//}
		vkDestroyDescriptorPool(state_ptr->context.device.logical_device, vk_shader->descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(state_ptr->context.device.logical_device, vk_shader->descriptor_set_layout, nullptr);
		vulkan_pipeline_destroy(state_ptr->context, vk_shader->pipeline);
	}

	void vulkan_renderer_shader_use(shader& s) {
		vulkan_shader* vk_shader = &std::any_cast<vulkan_shader>(s.internal_data);
		vulkan_pipeline_bind(state_ptr->context.command_buffers[state_ptr->context.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline);
	}

	void vulkan_renderer_geometry_create(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices) {

		vulkan_geometry internal_data;


		// Vertex buffer
		VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
		uint memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		vulkan_buffer staging_buffer;
		vulkan_buffer_create(state_ptr->context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memory_property_flags, true, staging_buffer);
		vulkan_buffer_load_data(state_ptr->context, staging_buffer, 0, buffer_size, 0, vertices.data());
		memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		uint usage_bit = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vulkan_buffer_create(state_ptr->context, buffer_size, (VkBufferUsageFlagBits)usage_bit, memory_property_flags, true, internal_data.vertex_buffer);
		vulkan_buffer_copy(state_ptr->context, state_ptr->context.device.command_pool, 0, state_ptr->context.device.graphics_queue, staging_buffer.handle, 0, internal_data.vertex_buffer.handle, 0, buffer_size);
		vulkan_buffer_destroy(state_ptr->context, staging_buffer);


		// Index buffer
		buffer_size = sizeof(indices[0]) * indices.size();
		memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		vulkan_buffer_create(state_ptr->context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memory_property_flags, true, staging_buffer);
		void* data;
		vulkan_buffer_load_data(state_ptr->context, staging_buffer, 0, buffer_size, 0, indices.data());
		memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		usage_bit = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		vulkan_buffer_create(state_ptr->context, buffer_size, (VkBufferUsageFlagBits)usage_bit, memory_property_flags, true, internal_data.index_buffer);
		vulkan_buffer_copy(state_ptr->context, state_ptr->context.device.command_pool, 0, state_ptr->context.device.graphics_queue, staging_buffer.handle, 0, internal_data.index_buffer.handle, 0, buffer_size);
		vulkan_buffer_destroy(state_ptr->context, staging_buffer);


		geometry.internal_data = internal_data;
	}

	void vulkan_renderer_geometry_destroy(geometry& geometry) {
		vulkan_geometry* vk_geometry = std::any_cast<vulkan_geometry>(&geometry.internal_data);

		vkDestroyBuffer(state_ptr->context.device.logical_device, vk_geometry->vertex_buffer.handle, nullptr);
		vkFreeMemory(state_ptr->context.device.logical_device, vk_geometry->vertex_buffer.memory, nullptr);
		vkDestroyBuffer(state_ptr->context.device.logical_device, vk_geometry->index_buffer.handle, nullptr);
		vkFreeMemory(state_ptr->context.device.logical_device, vk_geometry->index_buffer.memory, nullptr);
	}


	void create_framebuffers() {
		state_ptr->context.swapchain.framebuffers.resize(state_ptr->context.swapchain.views.size());
		for (int i = 0; i < state_ptr->context.swapchain.views.size(); ++i) {
			VkImageView attachments[] = {
				state_ptr->context.swapchain.views[i],
				state_ptr->context.swapchain.depth_attachment.view
			};

			VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			framebuffer_info.renderPass = state_ptr->context.main_renderpass.handle;
			framebuffer_info.attachmentCount = 2;
			framebuffer_info.pAttachments = attachments;
			framebuffer_info.width = state_ptr->context.swapchain.extent.width;
			framebuffer_info.height = state_ptr->context.swapchain.extent.height;
			framebuffer_info.layers = 1;

			VK_CHECK(vkCreateFramebuffer(state_ptr->context.device.logical_device, &framebuffer_info, nullptr, &state_ptr->context.swapchain.framebuffers[i]));
		}
	}

	void destroy_framebuffers() {
		for (int i = 0; i < state_ptr->context.swapchain.framebuffers.size(); ++i) {
			vkDestroyFramebuffer(state_ptr->context.device.logical_device, state_ptr->context.swapchain.framebuffers[i], nullptr);
		}
	}

	void recreate_swapchain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(std::any_cast<GLFWwindow*>(platform_system_get_window()), &width, &height);
			glfwWaitEvents();
		}

		state_ptr->context.main_renderpass.render_area.z = width;
		state_ptr->context.main_renderpass.render_area.w = height;

		vkDeviceWaitIdle(state_ptr->context.device.logical_device);
		destroy_framebuffers();
		//vulkan_imageview_destroy(state_ptr->context);

		vulkan_swapchain_recreate(state_ptr->context, state_ptr->context.swapchain);
		create_framebuffers();
		create_command_buffers();
	}

	void create_command_buffers() {
		state_ptr->context.command_buffers.resize(state_ptr->context.swapchain.image_count);

		for (uint i = 0; i < state_ptr->context.swapchain.image_count; ++i) {
			if (state_ptr->context.command_buffers[i].handle) {
				vulkan_command_buffer_free(state_ptr->context, state_ptr->context.device.command_pool, state_ptr->context.command_buffers[i]);
			}

			vulkan_command_buffer_allocate(state_ptr->context, state_ptr->context.device.command_pool, true, state_ptr->context.command_buffers[i]);
		}

		CE_LOG_INFO("Vulkan command buffers created");
	}

	VkShaderModule create_shader_module(std::string& file_path) {
		resource r;
		resource_system_load(file_path, RESOURCE_TYPE_BINARY, r);
		std::vector<uchar> code = std::any_cast<std::vector<uchar>>(r.data);
		
		VkShaderModuleCreateInfo create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		create_info.codeSize = r.data_size;
		create_info.pCode = (uint*)code.data();

		VkShaderModule shader_module;
		if (vkCreateShaderModule(state_ptr->context.device.logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			CE_LOG_ERROR("Could not create the shader");
			return VkShaderModule();
		}
		resource_system_unload(r);
		return shader_module;
	}
}