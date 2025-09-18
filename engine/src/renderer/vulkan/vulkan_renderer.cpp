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
	VkFilter get_vulkan_texture_filter(texture_filter filter);

	bool vulkan_renderpass_create(vulkan_context& context, vulkan_renderpass& out_renderpass, glm::vec4 render_area, glm::vec4 clear_color, float depth, uint stencil, bool has_prev_pass, bool has_next_pass);
	void vulkan_renderpass_destroy(vulkan_context& context, vulkan_renderpass& renderpass);

	// TODO: TEMPORAL
	void create_object_pick();
	void destroy_object_pick();
	// TODO: END TEMPORAL

	typedef struct uniform_vertex_buffer_object {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 view_position;
	}uniform_vertex_buffer_object;

	#define MAX_POINT_LIGHTS 10
	typedef struct uniform_fragment_buffer_object {
		glm::vec4 ambient_color;
		point_light_definition point_lights[MAX_POINT_LIGHTS];
		int number_of_lights;
	}uniform_fragment_buffer_object;

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

		// TODO: TEMPORAL
		create_object_pick();
		// TODO: END TEMPORAL


		CE_LOG_INFO("Vulkan backend initialized.");
		return true;
	}

	void vulkan_renderer_backend_stop() {
		vkDeviceWaitIdle(state_ptr->context.device.logical_device);
	}

	void vulkan_renderer_backend_shutdown() {

		vkDeviceWaitIdle(state_ptr->context.device.logical_device);

		// TODO: TEMPORAL
		destroy_object_pick();
		//TODO: REMPORAL


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

		// TODO: Refactor on a view system
		submit_info.commandBufferCount = 2;
		std::array<VkCommandBuffer, 2> cbs = { state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle };
		submit_info.pCommandBuffers = cbs.data();
		// TODO: END TODO

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


	bool vulkan_renderpass_create(vulkan_context& context, vulkan_renderpass& out_renderpass, glm::vec4 render_area, glm::vec4 clear_color, float depth, uint stencil, bool has_prev_pass, bool has_next_pass) {
		out_renderpass.render_area = render_area;
		out_renderpass.clear_color = clear_color;
		out_renderpass.depth = depth;
		out_renderpass.stencil = stencil;
		out_renderpass.has_prev_pass = has_prev_pass;
		out_renderpass.has_next_pass = has_next_pass;

		VkAttachmentDescription color_attachment = {};
		color_attachment.format = context.swapchain.surface_format.format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = context.device.depth_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference color_attachment_reference = {};
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_reference = {};
		depth_attachment_reference.attachment = 1;
		depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;
		subpass.pDepthStencilAttachment = &depth_attachment_reference;

		std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
		VkRenderPassCreateInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderpass_info.attachmentCount = attachments.size();
		renderpass_info.pAttachments = attachments.data();
		renderpass_info.subpassCount = 1;
		renderpass_info.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		renderpass_info.dependencyCount = 1;
		renderpass_info.pDependencies = &dependency;

		VK_CHECK(vkCreateRenderPass(context.device.logical_device, &renderpass_info, nullptr, &out_renderpass.handle));

		return true;
	}

	void vulkan_renderpass_destroy(vulkan_context& context, vulkan_renderpass& renderpass) {
		vkDestroyRenderPass(context.device.logical_device, renderpass.handle, nullptr);
	}

	bool vulkan_renderer_begin_renderpass() {
		VkRenderPassBeginInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderpass_info.renderPass = state_ptr->context.main_renderpass.handle;
		renderpass_info.framebuffer = state_ptr->context.swapchain.framebuffers[state_ptr->context.image_index];
		renderpass_info.renderArea.offset.x = state_ptr->context.main_renderpass.render_area.x;
		renderpass_info.renderArea.offset.y = state_ptr->context.main_renderpass.render_area.y;
		renderpass_info.renderArea.extent.width = state_ptr->context.main_renderpass.render_area.z;
		renderpass_info.renderArea.extent.height = state_ptr->context.main_renderpass.render_area.w;
		std::array<VkClearValue, 2> clear_values;
		clear_values[0].color = { {state_ptr->context.main_renderpass.clear_color.r, state_ptr->context.main_renderpass.clear_color.g, state_ptr->context.main_renderpass.clear_color.b, state_ptr->context.main_renderpass.clear_color.a} };
		clear_values[1].depthStencil = { state_ptr->context.main_renderpass.depth, state_ptr->context.main_renderpass.stencil };
		renderpass_info.clearValueCount = clear_values.size();
		renderpass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
		
		return true;
	}

	bool vulkan_renderer_end_renderpass() {
		vkCmdEndRenderPass(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle);

		return true;
	}

	void vulkan_renderer_set_and_apply_uniforms(std::vector<quad_properties>& quads, std::vector<point_light_definition>& point_lights, glm::vec4 ambient_color, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint quad_count, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position) {
		uniform_vertex_buffer_object ubo_vertex;
		ubo_vertex.view = view;
		ubo_vertex.proj = projection;
		ubo_vertex.view_position = view_position;

		uniform_fragment_buffer_object ubo_frag;
		ubo_frag.ambient_color = ambient_color;
		ubo_frag.number_of_lights = point_lights.size();
		for (uint i = 0; i < ubo_frag.number_of_lights; ++i) {
			ubo_frag.point_lights[i] = point_lights[i];
		}

		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader_internal_data);
		copy_memory(vk_shader->uniform_buffers_vertex_mapped, &ubo_vertex, sizeof(ubo_vertex));
		copy_memory(vk_shader->uniform_buffers_fragment_mapped, &ubo_frag, sizeof(ubo_frag));
		copy_memory(vk_shader->ssbo_mapped, quads.data(), sizeof(quad_properties) * quad_count);

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = vk_shader->uniform_buffers_vertex.handle;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(uniform_vertex_buffer_object);

		VkDescriptorBufferInfo buffer_fragment_info = {};
		buffer_fragment_info.buffer = vk_shader->uniform_buffers_fragment.handle;
		buffer_fragment_info.offset = 0;
		buffer_fragment_info.range = sizeof(uniform_fragment_buffer_object);

		VkDescriptorBufferInfo ssbo_info = {};
		ssbo_info.buffer = vk_shader->ssbo.handle;
		ssbo_info.offset = 0;
		ssbo_info.range = sizeof(quad_properties) * state_ptr->max_number_quads; //((sizeof(quad_properties) + (alignof(quad_properties) - 1)) & ~(alignof(quad_properties) - 1))

		std::array<VkWriteDescriptorSet, 4> descriptor_writes = {};
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

		descriptor_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[3].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		descriptor_writes[3].dstBinding = 3;
		descriptor_writes[3].dstArrayElement = 0;
		descriptor_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[3].descriptorCount = 1;
		descriptor_writes[3].pBufferInfo = &buffer_fragment_info;
		descriptor_writes[3].pImageInfo = nullptr;
		descriptor_writes[3].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(state_ptr->context.device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

		vkCmdBindDescriptorSets(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1, &vk_shader->descriptor_sets[state_ptr->context.current_frame], 0, nullptr);

	}

	VkFilter get_vulkan_texture_filter(texture_filter filter) {
		if (filter == FILTER_LINEAR) {
			return VK_FILTER_LINEAR;
		}

		if (filter == FILTER_NEAREST) {
			return VK_FILTER_NEAREST;
		}
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

		vulkan_renderer_texture_change_filter(t);

	}

	void vulkan_renderer_texture_change_filter(texture& t) {
		vulkan_texture* vk_texture = std::any_cast<vulkan_texture>(&t.internal_data);
		vkDestroySampler(state_ptr->context.device.logical_device, vk_texture->sampler, nullptr);

		// Create sampler
		VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		sampler_info.magFilter = get_vulkan_texture_filter(t.magnification_filter);
		sampler_info.minFilter = get_vulkan_texture_filter(t.minification_filter);
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

		VkShaderModule vert_module = create_shader_module(std::string("shaders/" + s.name + ".vert.spv"));
		VkPipelineShaderStageCreateInfo vert_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_module;
		vert_shader_stage_info.pName = "main";

		VkShaderModule frag_module = create_shader_module(std::string("shaders/" + s.name + ".frag.spv"));
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
		VkDescriptorSetLayoutBinding ubo_vertex_layout_binding = {};
		ubo_vertex_layout_binding.binding = 0;
		ubo_vertex_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_vertex_layout_binding.descriptorCount = 1;
		ubo_vertex_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_vertex_layout_binding.pImmutableSamplers = nullptr;

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

		VkDescriptorSetLayoutBinding ubo_fragment_layout_binding = {};
		ubo_fragment_layout_binding.binding = 3;
		ubo_fragment_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_fragment_layout_binding.descriptorCount = 1;
		ubo_fragment_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ubo_fragment_layout_binding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { ubo_vertex_layout_binding, sampler_layout_binding, ssbo_layout_binding, ubo_fragment_layout_binding };
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
		VkDeviceSize buffer_vertex_size = sizeof(uniform_vertex_buffer_object);
		vulkan_buffer_create(state_ptr->context, buffer_vertex_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->uniform_buffers_vertex);
		vk_shader->uniform_buffers_vertex_mapped = vulkan_buffer_lock_memory(state_ptr->context, vk_shader->uniform_buffers_vertex, 0, buffer_vertex_size, 0);

		VkDeviceSize buffer_fragment_size = sizeof(uniform_fragment_buffer_object);
		vulkan_buffer_create(state_ptr->context, buffer_fragment_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->uniform_buffers_fragment);
		vk_shader->uniform_buffers_fragment_mapped = vulkan_buffer_lock_memory(state_ptr->context, vk_shader->uniform_buffers_fragment, 0, buffer_fragment_size, 0);


		// Vertex SSBO
		vulkan_buffer_create(state_ptr->context, sizeof(quad_properties) * state_ptr->max_number_quads, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->ssbo);
		vk_shader->ssbo_mapped = vulkan_buffer_lock_memory(state_ptr->context, vk_shader->ssbo, 0, sizeof(quad_properties) * state_ptr->max_number_quads, 0);


		// Descriptor pool
		std::array<VkDescriptorPoolSize, 4> pool_sizes;
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1; // HACK: max number of ubo descriptor sets.
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = 4096; // HACK: max number of image sampler descriptor sets.
		pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[2].descriptorCount = 2;
		pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[3].descriptorCount = 1; // HACK: max number of ubo descriptor sets.

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
		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->uniform_buffers_vertex);
		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->uniform_buffers_fragment);
		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->ssbo);
		vk_shader->uniform_buffers_vertex_mapped = 0;
		vk_shader->uniform_buffers_fragment_mapped = 0;
		vk_shader->ssbo_mapped = 0;
		vulkan_buffer_destroy(state_ptr->context, vk_shader->uniform_buffers_vertex);
		vulkan_buffer_destroy(state_ptr->context, vk_shader->uniform_buffers_fragment);
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















	// TODO: Refactor into a view system
	typedef struct ssbo_object_picking {
		uint id;
	}ssbo_object_picking;


	void create_object_pick() {
		vulkan_image_create(
			state_ptr->context,
			VK_IMAGE_TYPE_2D,
			state_ptr->context.swapchain.extent.width,
			state_ptr->context.swapchain.extent.height,
			VK_FORMAT_R32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true,
			VK_IMAGE_ASPECT_COLOR_BIT,
			state_ptr->context.color_object_pick_image
		);


		//helper functions will be provided in the bottom of the page
		state_ptr->context.object_pick_command_buffers.resize(state_ptr->context.swapchain.image_count);

		for (uint i = 0; i < state_ptr->context.swapchain.image_count; ++i) {
			if (state_ptr->context.object_pick_command_buffers[i].handle) {
				vulkan_command_buffer_free(state_ptr->context, state_ptr->context.device.command_pool, state_ptr->context.object_pick_command_buffers[i]);
			}

			vulkan_command_buffer_allocate(state_ptr->context, state_ptr->context.device.command_pool, true, state_ptr->context.object_pick_command_buffers[i]);
		}




		VkAttachmentDescription object_picking_colorAttachment{};
		object_picking_colorAttachment.format = VK_FORMAT_R32_SFLOAT;
		object_picking_colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		object_picking_colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		object_picking_colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		object_picking_colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		object_picking_colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		object_picking_colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		object_picking_colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference object_picking_colorAttachmentRef{};
		object_picking_colorAttachmentRef.attachment = 0;
		object_picking_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkSubpassDescription object_picking_subpass{};
		object_picking_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		object_picking_subpass.colorAttachmentCount = 1;
		object_picking_subpass.pColorAttachments = &object_picking_colorAttachmentRef;
		object_picking_subpass.pDepthStencilAttachment = nullptr;

		VkSubpassDependency object_picking_dependency{};
		object_picking_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		object_picking_dependency.dstSubpass = 0;
		object_picking_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		object_picking_dependency.srcAccessMask = 0;
		object_picking_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		object_picking_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 1 > object_picking_attachments = { object_picking_colorAttachment};
		VkRenderPassCreateInfo object_picking_renderPassInfo{};
		object_picking_renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		object_picking_renderPassInfo.attachmentCount = static_cast<uint32_t>(object_picking_attachments.size());
		object_picking_renderPassInfo.pAttachments = object_picking_attachments.data();
		object_picking_renderPassInfo.subpassCount = 1;
		object_picking_renderPassInfo.pSubpasses = &object_picking_subpass;
		object_picking_renderPassInfo.dependencyCount = 1;
		object_picking_renderPassInfo.pDependencies = &object_picking_dependency;

		if (vkCreateRenderPass(state_ptr->context.device.logical_device, &object_picking_renderPassInfo, nullptr, &state_ptr->context.object_pick_pass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

		std::array<VkImageView, 1> attachments = { state_ptr->context.color_object_pick_image.view};
		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = state_ptr->context.object_pick_pass;
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.width = state_ptr->context.swapchain.extent.width;
		framebuffer_info.height = state_ptr->context.swapchain.extent.height;
		framebuffer_info.layers = 1;
		if (vkCreateFramebuffer(state_ptr->context.device.logical_device, &framebuffer_info, nullptr, &state_ptr->context.object_pick_framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}


		//vert/frag stages
		VkShaderModule vert_module = create_shader_module(std::string("shaders/Builtin.SpritePickShader.vert.spv"));
		VkPipelineShaderStageCreateInfo vert_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_module;
		vert_shader_stage_info.pName = "main";

		VkShaderModule frag_module = create_shader_module(std::string("shaders/Builtin.SpritePickShader.frag.spv"));
		VkPipelineShaderStageCreateInfo frag_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_module;
		frag_shader_stage_info.pName = "main";

		std::vector< VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.push_back(vert_shader_stage_info);
		shaderStages.push_back(frag_shader_stage_info);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		
		// Vertex inputs
		VkVertexInputBindingDescription binding_description = {};
		binding_description.binding = 0;
		binding_description.stride = sizeof(vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = NUMBER_OF_VERTEX_ATTRIBUTES;
		vertexInputInfo.pVertexBindingDescriptions = &binding_description;
		vertexInputInfo.pVertexAttributeDescriptions = attribute_descriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = (float)state_ptr->context.swapchain.extent.height;// Invert Y to match with Vulkan;
		viewport.width = (float)state_ptr->context.swapchain.extent.width;
		viewport.height = -(float)state_ptr->context.swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//We're going to have a dynamic scissor so this doesnt really matter
		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = state_ptr->context.swapchain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		//Only one samples since this is offscreen
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		//blending is set to false since we don't need it here
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 1.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;


		// Descriptor set layout
		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 3;
		sampler_layout_binding.descriptorCount = state_ptr->max_textures_per_batch;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding ssbo_layout_binding = {};
		ssbo_layout_binding.binding = 1;
		ssbo_layout_binding.descriptorCount = 1;// TODO: MORE CONFIGURABLE WITH THE vulkan_renderer_set_and_apply_uniforms
		ssbo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssbo_layout_binding.pImmutableSamplers = nullptr;
		ssbo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding ssbo_out_layout_binding = {};
		ssbo_out_layout_binding.binding = 2;
		ssbo_out_layout_binding.descriptorCount = 1;// TODO: MORE CONFIGURABLE WITH THE vulkan_renderer_set_and_apply_uniforms
		ssbo_out_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssbo_out_layout_binding.pImmutableSamplers = nullptr;
		ssbo_out_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { ubo_layout_binding, ssbo_layout_binding, ssbo_out_layout_binding,sampler_layout_binding };
		VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();
		VK_CHECK(vkCreateDescriptorSetLayout(state_ptr->context.device.logical_device, &layout_info, nullptr, &state_ptr->context.object_pick_shader.descriptor_set_layout));


		//we only need a dynamic state for the scissor, this means whatever we set to the scissor here will be ignored, and we'll need to use vkCmdSetScissor() every frame we use the pipeline
		std::array<VkDynamicState, 2> states = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

		VkPipelineDynamicStateCreateInfo dynamic_state{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = states.size();
		dynamic_state.pDynamicStates = states.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &state_ptr->context.object_pick_shader.descriptor_set_layout;

		if (vkCreatePipelineLayout(state_ptr->context.device.logical_device, &pipelineLayoutInfo, nullptr, &state_ptr->context.object_pick_shader.pipeline.layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = state_ptr->context.object_pick_shader.pipeline.layout;
		pipelineInfo.renderPass = state_ptr->context.object_pick_pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.pDynamicState = &dynamic_state;

		if (vkCreateGraphicsPipelines(state_ptr->context.device.logical_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state_ptr->context.object_pick_shader.pipeline.handle) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(state_ptr->context.device.logical_device, vert_module, nullptr);
		vkDestroyShaderModule(state_ptr->context.device.logical_device, frag_module, nullptr);


		// Descriptor pool
		std::array<VkDescriptorPoolSize, 3> pool_sizes;
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1024; // HACK: max number of ubo descriptor sets.
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[1].descriptorCount = 2;
		pool_sizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[2].descriptorCount = 4096; // HACK: max number of image sampler descriptor sets.

		VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.poolSizeCount = pool_sizes.size();
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VK_CHECK(vkCreateDescriptorPool(state_ptr->context.device.logical_device, &pool_info, nullptr, &state_ptr->context.object_pick_shader.descriptor_pool));

		// Descriptor sets
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, state_ptr->context.object_pick_shader.descriptor_set_layout);
		VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc_info.descriptorPool = state_ptr->context.object_pick_shader.descriptor_pool;
		alloc_info.descriptorSetCount = 2;
		alloc_info.pSetLayouts = layouts.data();

		state_ptr->context.object_pick_shader.descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
		VK_CHECK(vkAllocateDescriptorSets(state_ptr->context.device.logical_device, &alloc_info, state_ptr->context.object_pick_shader.descriptor_sets.data()));

		// UBO
		VkDeviceSize buffer_size = sizeof(uniform_vertex_buffer_object);
		vulkan_buffer_create(state_ptr->context, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, state_ptr->context.object_pick_shader.uniform_buffers_vertex);
		state_ptr->context.object_pick_shader.uniform_buffers_vertex_mapped = vulkan_buffer_lock_memory(state_ptr->context, state_ptr->context.object_pick_shader.uniform_buffers_vertex, 0, buffer_size, 0);

		// Vertex SSBO
		vulkan_buffer_create(state_ptr->context, sizeof(pick_quad_properties) * state_ptr->max_number_quads, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, state_ptr->context.object_pick_shader.ssbo);
		state_ptr->context.object_pick_shader.ssbo_mapped = vulkan_buffer_lock_memory(state_ptr->context, state_ptr->context.object_pick_shader.ssbo, 0, sizeof(pick_quad_properties) * state_ptr->max_number_quads, 0);

		vulkan_buffer_create(state_ptr->context, sizeof(ssbo_object_picking), (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, state_ptr->context.ssbo_pick_out);
	}

	void destroy_object_pick() {
		vulkan_shader* vk_shader = &state_ptr->context.object_pick_shader;

		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->uniform_buffers_vertex);
		vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->ssbo);
		vk_shader->uniform_buffers_vertex_mapped = 0;
		vk_shader->ssbo_mapped = 0;
		vulkan_buffer_destroy(state_ptr->context, vk_shader->uniform_buffers_vertex);
		vulkan_buffer_destroy(state_ptr->context, vk_shader->ssbo);
		vkDestroyDescriptorPool(state_ptr->context.device.logical_device, vk_shader->descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(state_ptr->context.device.logical_device, vk_shader->descriptor_set_layout, nullptr);
		vulkan_pipeline_destroy(state_ptr->context, vk_shader->pipeline);

		vkDestroyRenderPass(state_ptr->context.device.logical_device, state_ptr->context.object_pick_pass, nullptr);
		vulkan_image_destroy(state_ptr->context, state_ptr->context.color_object_pick_image);

		vkDestroyFramebuffer(state_ptr->context.device.logical_device, state_ptr->context.object_pick_framebuffer, nullptr);

		for (uint i = 0; i < state_ptr->context.swapchain.image_count; ++i) {
			if (state_ptr->context.object_pick_command_buffers[i].handle) {
				vulkan_command_buffer_free(state_ptr->context, state_ptr->context.device.command_pool, state_ptr->context.object_pick_command_buffers[i]);
			}
		}

		vulkan_buffer_destroy(state_ptr->context, state_ptr->context.ssbo_pick_out);
	}

	void pick_object(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view) {
		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		vkResetCommandBuffer(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, 0);

		//start recording object picking command buffer
		if (vkBeginCommandBuffer(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
		//begin renderpass
		VkRenderPassBeginInfo object_picking_renderpass_begin_info{};
		object_picking_renderpass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		object_picking_renderpass_begin_info.renderPass = state_ptr->context.object_pick_pass;
		object_picking_renderpass_begin_info.framebuffer = state_ptr->context.object_pick_framebuffer;
		object_picking_renderpass_begin_info.renderArea.offset.x = state_ptr->context.main_renderpass.render_area.x;
		object_picking_renderpass_begin_info.renderArea.offset.y = state_ptr->context.main_renderpass.render_area.y;
		object_picking_renderpass_begin_info.renderArea.extent.width = state_ptr->context.main_renderpass.render_area.z;
		object_picking_renderpass_begin_info.renderArea.extent.height = state_ptr->context.main_renderpass.render_area.w;
		std::array<VkClearValue, 2> clear_values;
		clear_values[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clear_values[1].depthStencil = { 0, 0 };
		object_picking_renderpass_begin_info.clearValueCount = clear_values.size();
		object_picking_renderpass_begin_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, &object_picking_renderpass_begin_info,
			VK_SUBPASS_CONTENTS_INLINE);
		
		vulkan_pipeline_bind(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, state_ptr->context.object_pick_shader.pipeline);
			


			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = (float)state_ptr->context.swapchain.extent.height;// Invert Y to match with Vulkan;
			viewport.width = (float)state_ptr->context.swapchain.extent.width;
			viewport.height = -(float)state_ptr->context.swapchain.extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, 0, 1, &viewport);
		
			//the dynamic pipeline state means we have to set the scissor before each draw
			VkRect2D rect{};
			double cursor_x, cursor_y;
			std::tie(cursor_x, cursor_y) = platform_system_get_cursor_position();
			rect.offset.x = cursor_x > 0 ? cursor_x : 0;
			rect.offset.y =cursor_y > 0 ? cursor_y : 0;
			rect.extent = { 1,1 };

			//can only be used with a dynamic scissor state
			vkCmdSetScissor(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, 0, 1, &rect);

			//bind descriptor sets for current object
			uniform_vertex_buffer_object ubo;
			ubo.view = view;
			ubo.proj = projection;
			ubo.view_position = glm::vec3(0.0f);

			vulkan_shader* vk_shader = &state_ptr->context.object_pick_shader;
			copy_memory(vk_shader->uniform_buffers_vertex_mapped, &ubo, sizeof(ubo));
			copy_memory(vk_shader->ssbo_mapped, quads.data(), sizeof(pick_quad_properties) * instance_count);

			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = vk_shader->uniform_buffers_vertex.handle;
			buffer_info.offset = 0;
			buffer_info.range = sizeof(uniform_vertex_buffer_object);

			VkDescriptorBufferInfo ssbo_info = {};
			ssbo_info.buffer = vk_shader->ssbo.handle;
			ssbo_info.offset = 0;
			ssbo_info.range = sizeof(pick_quad_properties) * state_ptr->max_number_quads; //((sizeof(quad_properties) + (alignof(quad_properties) - 1)) & ~(alignof(quad_properties) - 1))

			VkDescriptorBufferInfo ssbo_out_info = {};
			ssbo_out_info.buffer = state_ptr->context.ssbo_pick_out.handle;
			ssbo_out_info.offset = 0;
			ssbo_out_info.range = sizeof(ssbo_object_picking); //((sizeof(quad_properties) + (alignof(quad_properties) - 1)) & ~(alignof(quad_properties) - 1))


			std::array<VkWriteDescriptorSet, 4> descriptor_writes = {};
			descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[0].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].pBufferInfo = &buffer_info;
			descriptor_writes[0].pImageInfo = nullptr;
			descriptor_writes[0].pTexelBufferView = nullptr;


			descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[1].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
			descriptor_writes[1].dstBinding = 1;
			descriptor_writes[1].dstArrayElement = 0;
			descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptor_writes[1].descriptorCount = 1;
			descriptor_writes[1].pBufferInfo = &ssbo_info;
			descriptor_writes[1].pImageInfo = nullptr;
			descriptor_writes[1].pTexelBufferView = nullptr;

			descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[2].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
			descriptor_writes[2].dstBinding = 2;
			descriptor_writes[2].dstArrayElement = 0;
			descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptor_writes[2].descriptorCount = 1;
			descriptor_writes[2].pBufferInfo = &ssbo_out_info;
			descriptor_writes[2].pImageInfo = nullptr;
			descriptor_writes[2].pTexelBufferView = nullptr;


			uint number_of_texture = 0;
			while (state_ptr->context.batch_image_infos[number_of_texture].imageView != 0 ) {
				number_of_texture++;
			}

			descriptor_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[3].dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
			descriptor_writes[3].dstBinding = 3;
			descriptor_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[3].descriptorCount = number_of_texture;
			descriptor_writes[3].pImageInfo = state_ptr->context.batch_image_infos.data();

			vkUpdateDescriptorSets(state_ptr->context.device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

			vkCmdBindDescriptorSets(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, VK_PIPELINE_BIND_POINT_GRAPHICS, state_ptr->context.object_pick_shader.pipeline.layout, 0, 1, &state_ptr->context.object_pick_shader.descriptor_sets[state_ptr->context.current_frame], 0, nullptr);

			vulkan_geometry* vk_geometry = std::any_cast<vulkan_geometry>(&geometry.internal_data);
			//bind vertex and index buffers for current object
			VkBuffer vertex_buffers[] = { vk_geometry->vertex_buffer.handle };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, vk_geometry->index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);

			vkCmdDrawIndexed(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle, geometry.index_count, instance_count, 0, 0, 0);

		
		vkCmdEndRenderPass(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle);

		if (vkEndCommandBuffer(state_ptr->context.object_pick_command_buffers[state_ptr->context.current_frame].handle) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}



	}

	void show_picked_obj() {
		//Object to map data into
		ssbo_object_picking p;

		//map gpu memory into data
		void* data;
		vkMapMemory(state_ptr->context.device.logical_device, state_ptr->context.ssbo_pick_out.memory, 0, sizeof(ssbo_object_picking), 0, &data);

		//copy from data into p
		memcpy(&p, data, sizeof(ssbo_object_picking));

		//unmap gpu memory
		vkUnmapMemory(state_ptr->context.device.logical_device, state_ptr->context.ssbo_pick_out.memory);

		CE_LOG_INFO("%d", p.id);
	}

	// TODO: End TODO
}