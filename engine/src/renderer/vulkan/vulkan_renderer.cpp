#include "vulkan_renderer.h"
#include "cepch.h"
#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_image.h"
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
	// This indicates the number of sets definined, for example if it has a descriptor set formed by an ubo(1), ssbo(1) and a sampler(1024). This variable will reserve two instances of this set, which means its number is multiplied by this value.
	#define NUMBER_OF_DESCRIPTORS_SET 2

	void recreate_swapchain();
	void create_command_buffers();
	VkFilter get_vulkan_texture_filter(texture_filter filter);

	typedef struct vulkan_backend_state {
		vulkan_context context;

		std::vector<VkWriteDescriptorSet> descriptor_writes;
		std::vector<VkDescriptorBufferInfo> descriptor_buffer_infos;
	}vulkan_backend_state;

	static std::unique_ptr<vulkan_backend_state> state_ptr;

	bool vulkan_renderer_backend_initialize(const renderer_backend_config& config) {

		state_ptr = std::make_unique<vulkan_backend_state>();

		if (!state_ptr) {
			return false;
		}
		state_ptr->context.image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		state_ptr->context.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);



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


		// Create command buffers
		create_command_buffers();

		// Create synchronization objects
		VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VK_CHECK(vkCreateSemaphore(state_ptr->context.device.logical_device, &semaphore_info, nullptr, &state_ptr->context.image_available_semaphores[i]));
			VK_CHECK(vkCreateFence(state_ptr->context.device.logical_device, &fence_info, nullptr, &state_ptr->context.in_flight_fences[i]));
		}

		state_ptr->context.swapchain.semaphores.resize(state_ptr->context.swapchain.image_count);
		for (uint i = 0; i < state_ptr->context.swapchain.image_count; ++i) {
			VK_CHECK(vkCreateSemaphore(state_ptr->context.device.logical_device, &semaphore_info, nullptr, &state_ptr->context.swapchain.semaphores[i]));

		}

		CE_LOG_INFO("Vulkan backend initialized.");
		return true;
	}

	void vulkan_renderer_backend_stop() {
		VK_CHECK(vkDeviceWaitIdle(state_ptr->context.device.logical_device));
	}

	void vulkan_renderer_backend_shutdown() {

		VK_CHECK(vkDeviceWaitIdle(state_ptr->context.device.logical_device));

		state_ptr->descriptor_buffer_infos.clear();
		state_ptr->descriptor_writes.clear();

		for (uint i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroySemaphore(state_ptr->context.device.logical_device, state_ptr->context.image_available_semaphores[i], nullptr);
			vkDestroyFence(state_ptr->context.device.logical_device, state_ptr->context.in_flight_fences[i], nullptr);
		}

		for (uint i = 0; i < state_ptr->context.swapchain.image_count; ++i) {
			if (state_ptr->context.command_buffers[i].handle) {
				vulkan_command_buffer_free(state_ptr->context, state_ptr->context.device.command_pool, state_ptr->context.command_buffers[i]);
			}
			vkDestroySemaphore(state_ptr->context.device.logical_device, state_ptr->context.swapchain.semaphores[i], nullptr);

		}

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
		
		VK_CHECK(vkWaitForFences(state_ptr->context.device.logical_device, 1, &state_ptr->context.in_flight_fences[state_ptr->context.current_frame], VK_TRUE, UINT64_MAX));

		if (state_ptr->context.framebuffer_resized) {
			recreate_swapchain();
			state_ptr->context.framebuffer_resized = false;
			return false;
		}

		VkResult result = vulkan_swapchain_acquire_next_image_index(state_ptr->context, state_ptr->context.swapchain, UINT64_MAX, state_ptr->context.image_available_semaphores[state_ptr->context.current_frame], VK_NULL_HANDLE, state_ptr->context.image_index);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
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



		return true;
	}

	bool vulkan_renderer_end_frame(float delta_time) {

		VK_CHECK(vkEndCommandBuffer(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle));

		VK_CHECK(vkResetFences(state_ptr->context.device.logical_device, 1, &state_ptr->context.in_flight_fences[state_ptr->context.current_frame]));

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		VkSemaphore wait_semaphores[] = { state_ptr->context.image_available_semaphores[state_ptr->context.current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;

		submit_info.commandBufferCount = 1;
		std::array<VkCommandBuffer, 1> cbs = { state_ptr->context.command_buffers[state_ptr->context.current_frame].handle};
		submit_info.pCommandBuffers = cbs.data();

		VkSemaphore signal_sempahores[] = { state_ptr->context.swapchain.semaphores[state_ptr->context.image_index] };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_sempahores;
		VK_CHECK(vkQueueSubmit(state_ptr->context.device.graphics_queue, 1, &submit_info, state_ptr->context.in_flight_fences[state_ptr->context.current_frame]));

		VkResult result = vulkan_swapchain_present(state_ptr->context, state_ptr->context.swapchain, state_ptr->context.device.presentation_queue, state_ptr->context.swapchain.semaphores[state_ptr->context.image_index], state_ptr->context.image_index);
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

	uint vulkan_renderer_window_images_count_get() {
		return state_ptr->context.swapchain.image_count;
	}

	uint vulkan_renderer_window_image_index_get() {
		return state_ptr->context.image_index;
	}

	std::shared_ptr<std::any> vulkan_renderer_window_attachment_get(uint index) {
		return std::make_shared<std::any>(state_ptr->context.swapchain.images[index]);
	}

	std::shared_ptr<std::any> vulkan_renderer_depth_attachment_get() {
		return std::make_shared<std::any>(state_ptr->context.swapchain.depth_attachment);;
	}

	std::shared_ptr<std::any> vulkan_renderer_object_pick_attachment_get()
	{
		return std::make_shared<std::any>(state_ptr->context.swapchain.object_pick_attachment);
	}

	bool vulkan_renderer_render_target_create(renderpass& pass, render_target& target) {

		std::vector<VkImageView> attachments;

		for (uint i = 0; i < target.attachments.size(); ++i) {
			vulkan_image* image = std::any_cast<vulkan_image>(&(*target.attachments[i]));
			attachments.push_back(image->view);
		}

		vulkan_renderpass* vk_pass = std::any_cast<vulkan_renderpass>(&pass.internal_data);

		VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebuffer_info.renderPass = vk_pass->handle;
		framebuffer_info.attachmentCount = attachments.size();
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = pass.render_area.z;
		framebuffer_info.height = pass.render_area.w;
		framebuffer_info.layers = 1;

		VkFramebuffer vk_framebuffer;
		VK_CHECK(vkCreateFramebuffer(state_ptr->context.device.logical_device, &framebuffer_info, nullptr, &vk_framebuffer));
		target.internal_framebuffer = vk_framebuffer;

		return true;
	}

	bool vulkan_renderer_render_target_destroy(render_target& target) {
		
		if (!target.internal_framebuffer.has_value()) {
			return false;
		}

		for (uint i = 0; i < target.attachments.size(); ++i) {
			target.attachments[i].reset();
			target.attachments[i] = nullptr;
		}

		VkFramebuffer vk_framebuffer = std::any_cast<VkFramebuffer>(target.internal_framebuffer);
		vkDestroyFramebuffer(state_ptr->context.device.logical_device, vk_framebuffer, nullptr);
		
		target.attachments.clear();
		
		return true;
	}


	

	bool vulkan_renderer_renderpass_create(renderpass& out_pass, renderpass_resource_data& renderpass_data) {
		out_pass.render_area = glm::vec4(0.0f, 0.0f, state_ptr->context.swapchain.extent.width, state_ptr->context.swapchain.extent.height);
		out_pass.clear_color = renderpass_data.clear_color;
		out_pass.depth = renderpass_data.depth;
		out_pass.stencil = renderpass_data.stencil;
		out_pass.has_prev_pass = renderpass_data.has_prev_pass;
		out_pass.has_next_pass = renderpass_data.has_next_pass;
		
		std::vector<VkAttachmentDescription> attachments;

		bool do_clear_color = (out_pass.flags & RENDERPASS_CLEAR_FLAG_COLOR_BUFFER) != 0;
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = renderpass_data.attachment_formats[0] == ATTACHMENT_FORMAT_TYPE_SWAPCHAIN ? state_ptr->context.swapchain.surface_format.format : (VkFormat)renderpass_data.attachment_formats[0];
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = renderpass_data.has_prev_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = renderpass_data.has_next_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_reference = {};
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;

		attachments.push_back(color_attachment);

		bool do_clear_depth = (out_pass.flags & RENDERPASS_CLEAR_FLAG_DEPTH_BUFFER) != 0;
		if (do_clear_depth) {
			VkAttachmentDescription depth_attachment = {};
			depth_attachment.format = state_ptr->context.device.depth_format;
			depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depth_attachment.loadOp = renderpass_data.has_prev_pass ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depth_attachment_reference = {};
			depth_attachment_reference.attachment = 1;
			depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depth_attachment_reference;
			attachments.push_back(depth_attachment);
		}
		else {
			subpass.pDepthStencilAttachment = 0;
		}

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = (VkPipelineStageFlags)renderpass_data.subpass_src_stage_mask;
		dependency.srcAccessMask = (VkAccessFlags)renderpass_data.subpass_src_access_mask;
		dependency.dstStageMask = (VkPipelineStageFlags)renderpass_data.subpass_dst_stage_mask;
		dependency.dstAccessMask = (VkAccessFlags)renderpass_data.subpass_dst_access_mask;

		VkRenderPassCreateInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderpass_info.attachmentCount = attachments.size();
		renderpass_info.pAttachments = attachments.data();
		renderpass_info.subpassCount = 1;
		renderpass_info.pSubpasses = &subpass;
		renderpass_info.dependencyCount = 1;
		renderpass_info.pDependencies = &dependency;

		vulkan_renderpass vk_pass;
		VK_CHECK(vkCreateRenderPass(state_ptr->context.device.logical_device, &renderpass_info, nullptr, &vk_pass.handle));

		out_pass.internal_data = vk_pass;

		attachments.clear();

		return true;
	}

	void vulkan_renderer_renderpass_destroy(renderpass& pass) {
		vulkan_renderpass* vk_pass = std::any_cast<vulkan_renderpass>(&pass.internal_data);

		vkDestroyRenderPass(state_ptr->context.device.logical_device, vk_pass->handle, nullptr);
	}

	bool vulkan_renderer_renderpass_begin(renderpass& pass, render_target& target, glm::vec2 scissor_extent, glm::vec2 scissor_offset) {
		
		vulkan_renderpass* vk_pass = std::any_cast<vulkan_renderpass>(&pass.internal_data);

		VkFramebuffer vk_framebuffer = std::any_cast<VkFramebuffer>(target.internal_framebuffer);

		VkRenderPassBeginInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderpass_info.renderPass = vk_pass->handle;
		renderpass_info.framebuffer = vk_framebuffer;
		renderpass_info.renderArea.offset.x = pass.render_area.x;
		renderpass_info.renderArea.offset.y = pass.render_area.y;
		renderpass_info.renderArea.extent.width = pass.render_area.z;
		renderpass_info.renderArea.extent.height = pass.render_area.w;
		std::array<VkClearValue, 2> clear_values;
		clear_values[0].color = { {pass.clear_color.r, pass.clear_color.g, pass.clear_color.b, pass.clear_color.a} };
		clear_values[1].depthStencil = { pass.depth, pass.stencil };
		renderpass_info.clearValueCount = clear_values.size();
		renderpass_info.pClearValues = clear_values.data();

		VkRect2D scissor = {};
		scissor.offset = { (int)scissor_offset.x, (int)scissor_offset.y };
		scissor.extent = {(uint)scissor_extent.x, (uint)scissor_extent.y };
		
		if (scissor.extent.width == 0 && scissor.extent.height == 0) {
			scissor.extent = state_ptr->context.swapchain.extent;
		}
		
		vkCmdSetScissor(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, 0, 1, &scissor);

		vkCmdBeginRenderPass(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

		return true;
	}

	bool vulkan_renderer_renderpass_end() {
		vkCmdEndRenderPass(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle);
		return true;
	}

	void vulkan_renderer_set_descriptor_ubo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index)
	{
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader.internal_data);
		copy_memory(vk_shader->descriptor_buffer_maps[descriptor_buffer_index], data, data_size);

		state_ptr->descriptor_writes.push_back(VkWriteDescriptorSet());
		state_ptr->descriptor_buffer_infos.push_back(VkDescriptorBufferInfo());

		state_ptr->descriptor_buffer_infos.back().buffer = vk_shader->descriptor_buffers[descriptor_buffer_index].handle;
		state_ptr->descriptor_buffer_infos.back().offset = 0;
		state_ptr->descriptor_buffer_infos.back().range = data_size;

		state_ptr->descriptor_writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		state_ptr->descriptor_writes.back().dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		state_ptr->descriptor_writes.back().dstBinding = destination_binding;
		state_ptr->descriptor_writes.back().dstArrayElement = 0;
		state_ptr->descriptor_writes.back().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		state_ptr->descriptor_writes.back().descriptorCount = 1;
		state_ptr->descriptor_writes.back().pBufferInfo = &state_ptr->descriptor_buffer_infos.back();
		state_ptr->descriptor_writes.back().pImageInfo = nullptr;
		state_ptr->descriptor_writes.back().pTexelBufferView = nullptr;

	}

	void vulkan_renderer_set_descriptor_sampler(std::vector<texture*>& textures_batch_ptr, uint destination_binding, shader& shader)
	{
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader.internal_data);
		state_ptr->descriptor_writes.push_back(VkWriteDescriptorSet());

		uint number_of_texture = 0;
		for (texture* texture : textures_batch_ptr) {
			if (texture == nullptr) {
				break;
			}
			state_ptr->context.batch_image_infos.push_back(VkDescriptorImageInfo());

			vulkan_texture* vk_texture = std::any_cast<vulkan_texture>(&texture->internal_data);
			state_ptr->context.batch_image_infos[number_of_texture].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			state_ptr->context.batch_image_infos[number_of_texture].imageView = vk_texture->image.view;
			state_ptr->context.batch_image_infos[number_of_texture].sampler = vk_texture->sampler;
			number_of_texture++;
		}

		state_ptr->descriptor_writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		state_ptr->descriptor_writes.back().dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		state_ptr->descriptor_writes.back().dstBinding = destination_binding;
		state_ptr->descriptor_writes.back().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		state_ptr->descriptor_writes.back().descriptorCount = number_of_texture;
		state_ptr->descriptor_writes.back().pImageInfo = state_ptr->context.batch_image_infos.data();
	}

	void vulkan_renderer_set_descriptor_ssbo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index)
	{
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader.internal_data);

		if ( data != nullptr) {
			copy_memory(vk_shader->descriptor_buffer_maps[descriptor_buffer_index], data, data_size);
		}

		state_ptr->descriptor_writes.push_back(VkWriteDescriptorSet());
		state_ptr->descriptor_buffer_infos.push_back(VkDescriptorBufferInfo());

		state_ptr->descriptor_buffer_infos.back().buffer = vk_shader->descriptor_buffers[descriptor_buffer_index].handle;
		state_ptr->descriptor_buffer_infos.back().offset = 0;
		state_ptr->descriptor_buffer_infos.back().range = data_size;

		state_ptr->descriptor_writes.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		state_ptr->descriptor_writes.back().dstSet = vk_shader->descriptor_sets[state_ptr->context.current_frame];
		state_ptr->descriptor_writes.back().dstBinding = destination_binding;
		state_ptr->descriptor_writes.back().dstArrayElement = 0;
		state_ptr->descriptor_writes.back().descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		state_ptr->descriptor_writes.back().descriptorCount = 1;
		state_ptr->descriptor_writes.back().pBufferInfo = &state_ptr->descriptor_buffer_infos.back();
		state_ptr->descriptor_writes.back().pImageInfo = nullptr;
		state_ptr->descriptor_writes.back().pTexelBufferView = nullptr;

	}

	void vulkan_renderer_apply_descriptors(shader& shader)
	{
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader.internal_data);
		vkUpdateDescriptorSets(state_ptr->context.device.logical_device, state_ptr->descriptor_writes.size(), state_ptr->descriptor_writes.data(), 0, nullptr);
		vkCmdBindDescriptorSets(state_ptr->context.command_buffers[state_ptr->context.current_frame].handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1, &vk_shader->descriptor_sets[state_ptr->context.current_frame], 0, nullptr);

		state_ptr->descriptor_writes.clear();
		state_ptr->descriptor_buffer_infos.clear();
	}

	void vulkan_renderer_get_descriptor_ssbo(void* out_data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index)
	{
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&shader.internal_data);
		//copy from data into out_data
		memcpy(out_data, vk_shader->descriptor_buffer_maps[descriptor_buffer_index], data_size);

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

	bool vulkan_renderer_shader_create(shader_resource_data& shader_config, shader& out_shader, renderpass& pass) {

		// Shader modules creation
		VkShaderModuleCreateInfo create_vertex_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		create_vertex_info.codeSize = shader_config.vertex_code_size;
		create_vertex_info.pCode = (uint*)shader_config.vertex_code.data();

		VkShaderModule vert_module;
		if (vkCreateShaderModule(state_ptr->context.device.logical_device, &create_vertex_info, nullptr, &vert_module) != VK_SUCCESS) {
			CE_LOG_ERROR("Could not create the shader");
			return false();
		}
		VkPipelineShaderStageCreateInfo vert_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_module;
		vert_shader_stage_info.pName = "main";

		
		VkShaderModuleCreateInfo create_fragment_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		create_fragment_info.codeSize = shader_config.fragment_code_size;
		create_fragment_info.pCode = (uint*)shader_config.fragment_code.data();

		VkShaderModule frag_module;
		if (vkCreateShaderModule(state_ptr->context.device.logical_device, &create_fragment_info, nullptr, &frag_module) != VK_SUCCESS) {
			CE_LOG_ERROR("Could not create the shader");
			return false();
		}
		VkPipelineShaderStageCreateInfo frag_shader_stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

		// Properties
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
		std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
		attribute_descriptions.resize(shader_config.vertex_attribute_definitions.size());
		
		uint64 offset = 0;
		for (uint i = 0; i < attribute_descriptions.size(); ++i) {
			attribute_descriptions[i].binding = 0;
			attribute_descriptions[i].location = i;
			attribute_descriptions[i].format = (VkFormat)shader_config.vertex_attribute_definitions[i].type;
			attribute_descriptions[i].offset = offset;
			offset += shader_config.vertex_attribute_definitions[i].size;

		}

		out_shader.internal_data = vulkan_shader();
		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&out_shader.internal_data);

		// Descriptor set layout
		std::vector<std::pair<VkDescriptorType, uint>> pool_types_definition;
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		for (uint i = 0; i < shader_config.descriptor_definitions.size(); ++i) {
			VkDescriptorSetLayoutBinding binding;
			binding.binding = i;
			binding.descriptorType = (VkDescriptorType)shader_config.descriptor_definitions[i].type;
			binding.descriptorCount = shader_config.descriptor_definitions[i].count;
			binding.stageFlags = (VkShaderStageFlags)shader_config.descriptor_definitions[i].stage;
			binding.pImmutableSamplers = nullptr;

			bindings.push_back(binding);

			// Iterate thought to pool_types_definition to add or increment the number of descriptors that needs to be reserved in the creation of the descriptors pools (creation below)
			// TODO: Consider if change the shader config enumerator to be aligned to vector or to vulkan (maybe in the future to the vector index due to other implementations like opengl and DX?)
			// NOTE: A map its not used to not include another heavy header library to iterare a 3 element vector at most, a map is overkill
			bool found = false;
			uint pool_index = 0;
			for (pool_index = 0; pool_index < pool_types_definition.size(); ++pool_index) {
				if (pool_types_definition[pool_index].first == binding.descriptorType) {
					found = true;
					break;
				}
			}
			if (found) {
				pool_types_definition[pool_index].second += (shader_config.descriptor_definitions[pool_index].count * NUMBER_OF_DESCRIPTORS_SET); // Multiplied by the number of sets that will need the pool
			}
			else {
				pool_types_definition.push_back({ binding.descriptorType, binding.descriptorCount * NUMBER_OF_DESCRIPTORS_SET });
			}
		}

		VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();
		VK_CHECK(vkCreateDescriptorSetLayout(state_ptr->context.device.logical_device, &layout_info, nullptr, &vk_shader->descriptor_set_layout));

		// Create pipeline
		vulkan_renderpass* vk_pass = std::any_cast<vulkan_renderpass>(&pass.internal_data);
		bool result = vulkan_pipeline_create(
			state_ptr->context, 
			*vk_pass,
			sizeof(vertex),
			attribute_descriptions.size(),
			attribute_descriptions.data(),
			1,
			vk_shader->descriptor_set_layout,
			2,
			shader_stages,
			viewport,
			scissor,
			shader_config.color_blend_enabled,
			false,
			vk_shader->pipeline
			);

		if (!result) {
			CE_LOG_ERROR("Couldn't create the pipeline for the shader: %s", out_shader.name.c_str());
			return false;
		}

		vkDestroyShaderModule(state_ptr->context.device.logical_device, vert_module, nullptr);
		vkDestroyShaderModule(state_ptr->context.device.logical_device, frag_module, nullptr);

		// Uniform buffers
		vk_shader->descriptor_buffers.resize(shader_config.descriptor_buffer_definitions.size());
		for (uint i = 0; i < shader_config.descriptor_buffer_definitions.size(); ++i) {
			vulkan_buffer_create(state_ptr->context, shader_config.descriptor_buffer_definitions[i].size, (VkBufferUsageFlagBits)shader_config.descriptor_buffer_definitions[i].usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->descriptor_buffers[i]);
			vk_shader->descriptor_buffer_maps.push_back(vulkan_buffer_lock_memory(state_ptr->context, vk_shader->descriptor_buffers[i], 0, shader_config.descriptor_buffer_definitions[i].size, 0));
		}

		// Descriptor pool
		std::vector<VkDescriptorPoolSize> pool_sizes;
		for (uint i = 0; i < pool_types_definition.size(); ++i) {
			VkDescriptorPoolSize pool_size;
			pool_size.type = pool_types_definition[i].first;
			pool_size.descriptorCount = pool_types_definition[i].second;
			pool_sizes.push_back(pool_size);
		}

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
		alloc_info.descriptorSetCount = NUMBER_OF_DESCRIPTORS_SET;
		alloc_info.pSetLayouts = layouts.data();

		vk_shader->descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
		VK_CHECK(vkAllocateDescriptorSets(state_ptr->context.device.logical_device, &alloc_info, vk_shader->descriptor_sets.data()));

		return true;
	}

	void vulkan_renderer_shader_destroy(shader& s) {

		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&s.internal_data);

		for (uint i = 0; i < vk_shader->descriptor_buffers.size(); ++i) {
			vulkan_buffer_unlock_memory(state_ptr->context, vk_shader->descriptor_buffers[i]);
			vk_shader->descriptor_buffer_maps[i] = 0;
			vulkan_buffer_destroy(state_ptr->context, vk_shader->descriptor_buffers[i]);
		}

		vk_shader->descriptor_buffers.clear();

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


	void recreate_swapchain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(std::any_cast<GLFWwindow*>(platform_system_get_window()), &width, &height);
			glfwWaitEvents();
		}

		VK_CHECK(vkDeviceWaitIdle(state_ptr->context.device.logical_device));

		vulkan_swapchain_recreate(state_ptr->context, state_ptr->context.swapchain);
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
}