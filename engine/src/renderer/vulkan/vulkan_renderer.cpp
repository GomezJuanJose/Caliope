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






	void create_vertex_buffer();
	void create_index_buffer();


	

	static vulkan_context context;

	//TODO: Refactor for batch rendering and quad system/ geometry system
	std::vector<vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},

		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.0f, 1.5, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		{{1.5, 1.5, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{1.5, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	};

	std::vector<uint16> indices = {
		0, 1, 2, 2, 1, 3,
		4, 5, 6, 6, 7, 4
	};

	// TODO: MOVE INTO GEOMETRY/QUAD SYSTEM
	void calculate_tangents(std::vector<vertex>& vertices, std::vector<uint16>& indices) {
		for (uint i = 0; i < indices.size(); i+=3) {
			uint i0 = indices[i + 0];
			uint i1 = indices[i + 1];
			uint i2 = indices[i + 2];

			glm::vec3 deltaPos1 = vertices[i1].pos - vertices[i0].pos;
			glm::vec3 deltaPos2 = vertices[i2].pos - vertices[i0].pos;
		
			glm::vec2 deltaUV1 = vertices[i1].tex_coord - vertices[i0].tex_coord;
			glm::vec2 deltaUV2 = vertices[i2].tex_coord - vertices[i0].tex_coord;

			float fc = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
			glm::vec3 tangent = glm::vec3(
				(fc * (deltaUV2.y * deltaPos1.x - deltaUV1.y * deltaPos2.x)),
				(fc * (deltaUV2.y * deltaPos1.y - deltaUV1.y * deltaPos2.y)),
				(fc * (deltaUV2.y * deltaPos1.z - deltaUV1.y * deltaPos2.z))
			);

			tangent = glm::normalize(tangent);

			float handedness = ((deltaUV1.y * deltaUV2.x - deltaUV2.y * deltaUV1.x) < 0.0f) ? -1.0 : 1.0f;
			
			glm::vec4 t4 = glm::vec4(tangent, handedness);
			vertices[i0].tangent = t4;
			vertices[i1].tangent = t4;
			vertices[i2].tangent = t4;
		}
	}

	typedef struct uniform_buffer_object {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 ambient_color;
		glm::vec3 view_position;
		float shininess;
	}uniform_buffer_object;

	// TODO: refactor

	bool vulkan_renderer_backend_initialize(const std::string& application_name) {

		// TODO: MOVE INTO GEOMETRY/QUAD SYSTEM
		calculate_tangents(vertices, indices);

		// TODO: PRE-LOAD THE KNOWN SHADERS WHEN INITIALIZE

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
		if (!vulkan_swapchain_create(context, context.swapchain)) {
			CE_LOG_FATAL("vulkan_renderer_backend_initialize could not create the swapchain");
			return false;
		}


		// Create world renderpass
		if (!vulkan_renderpass_create(
			context,
			context.main_renderpass,
			glm::vec4(0.0f, 0.0f, context.swapchain.extent.width, context.swapchain.extent.height),
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


#pragma region REFACTOR
	
		// Create vertex and index buffer
		//TODO: Refactor for batch rendering
		create_vertex_buffer();
		create_index_buffer();

#pragma endregion

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

		//TODO: Refactor for batch rendering
		vkDestroyBuffer(context.device.logical_device, context.vertex_buffer.handle, nullptr);
		vkFreeMemory(context.device.logical_device, context.vertex_buffer.memory, nullptr);
		vkDestroyBuffer(context.device.logical_device, context.index_buffer.handle, nullptr);
		vkFreeMemory(context.device.logical_device, context.index_buffer.memory, nullptr);

		destroy_framebuffers();

		for (uint i = 0; i < context.swapchain.image_count; ++i) {
			if (context.command_buffers[i].handle) {
				vulkan_command_buffer_free(context, context.device.command_pool, context.command_buffers[i]);
			}
		}

		vulkan_renderpass_destroy(context, context.main_renderpass);


		vulkan_swapchain_destroy(context, context.swapchain);

		vulkan_device_destroy(context);

		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);

		vkDestroyInstance(context.instance, nullptr);
	}

	void vulkan_renderer_backend_resize(uint16 width, uint16 height) {
		swapchain_support_details new_support_details = vulkan_device_query_swapchain_support(context.device.physical_device, context.surface);
		context.device.swapchain_support_details = new_support_details;
		context.framebuffer_resized = true;
	}

	bool vulkan_renderer_begin_frame(float delta_time) {
		
		vkWaitForFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame], VK_TRUE, UINT64_MAX);

		VkResult result = vulkan_swapchain_acquire_next_image_index(context, context.swapchain, UINT64_MAX, context.image_available_semaphores[context.current_frame], VK_NULL_HANDLE, context.image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreate_swapchain();
			return true;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			CE_LOG_FATAL("vulkan_renderer_begin_frame failed to acquire swapchain image");
			return false;
		}

		
		vkResetCommandBuffer(context.command_buffers[context.current_frame].handle, 0);

		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;
		VK_CHECK(vkBeginCommandBuffer(context.command_buffers[context.current_frame].handle, &begin_info));
		
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(context.swapchain.extent.height);// Invert Y to match with Vulkan;
		viewport.width = static_cast<float>(context.swapchain.extent.width);
		viewport.height = -static_cast<float>(context.swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(context.command_buffers[context.current_frame].handle, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = context.swapchain.extent;
		vkCmdSetScissor(context.command_buffers[context.current_frame].handle, 0, 1, &scissor);

		return true;
	}

	bool vulkan_renderer_end_frame(float delta_time) {

		VK_CHECK(vkEndCommandBuffer(context.command_buffers[context.current_frame].handle));

		vkResetFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame]);

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

		VkResult result = vulkan_swapchain_present(context, context.swapchain, context.device.presentation_queue, context.render_finished_semaphores[context.current_frame], context.image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context.framebuffer_resized) {
			recreate_swapchain();
			context.framebuffer_resized = false;
		}
		else if (result != VK_SUCCESS) {
			CE_LOG_FATAL("vulkan_renderer_begin_frame failed to present swapchain image");
		}

		return true;
	}

	void vulkan_renderer_draw_geometry() {

		//TODO: Refactor for batch rendering

		VkBuffer vertex_buffers[] = { context.vertex_buffer.handle };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(context.command_buffers[context.current_frame].handle, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(context.command_buffers[context.current_frame].handle, context.index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);


		vkCmdDrawIndexed(context.command_buffers[context.current_frame].handle, 12, 1, 0, 0, 0); // HARDCODED VERTEX NUMBER

	}

	bool vulkan_renderer_begin_renderpass() {
		vulkan_renderpass_begin(context.command_buffers[context.current_frame], context.main_renderpass, context.swapchain.framebuffers[context.image_index]);
		return true;
	}

	bool vulkan_renderer_end_renderpass() {
		vulkan_renderpass_end(context.command_buffers[context.current_frame]);
		return true;
	}

	void vulkan_renderer_set_and_apply_uniforms(std::shared_ptr<material>& m, glm::mat4& model, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position) {
		uniform_buffer_object ubo;
		ubo.model = model;
		ubo.view = view;
		ubo.proj = projection;
		ubo.ambient_color = glm::vec4(0.02f, 0.02f, 0.02f, 1.0f);// TODO: Scene system;
		ubo.shininess = m->shininess;// TODO: batch rendering;
		ubo.view_position = view_position;

		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&m->shader->internal_data);
		vulkan_texture* vk_texture_diff = std::any_cast<vulkan_texture>(&m->diffuse_texture->internal_data);//TODO: Make it escalable with any quantity of textures(for normals and specular)
		vulkan_texture* vk_texture_spec = std::any_cast<vulkan_texture>(&m->specular_texture->internal_data);//TODO: Make it escalable with any quantity of textures(for normals and specular)
		vulkan_texture* vk_texture_norm = std::any_cast<vulkan_texture>(&m->normal_texture->internal_data);//TODO: Make it escalable with any quantity of textures(for normals and specular)
		copy_memory(vk_shader->uniform_buffers_mapped, &ubo, sizeof(ubo));

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = vk_shader->uniform_buffers.handle;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(uniform_buffer_object);


		std::array<VkWriteDescriptorSet, 2> descriptor_writes = {};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = vk_shader->descriptor_sets[context.current_frame];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;
		descriptor_writes[0].pImageInfo = nullptr;
		descriptor_writes[0].pTexelBufferView = nullptr;

		std::array<VkDescriptorImageInfo, 3> image_infos;
		image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infos[0].imageView = vk_texture_diff->image.view;
		image_infos[0].sampler = vk_texture_diff->sampler;

		image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infos[1].imageView = vk_texture_spec->image.view;
		image_infos[1].sampler = vk_texture_spec->sampler;

		image_infos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infos[2].imageView = vk_texture_norm->image.view;
		image_infos[2].sampler = vk_texture_norm->sampler;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = vk_shader->descriptor_sets[context.current_frame];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = image_infos.size();
		descriptor_writes[1].pImageInfo = image_infos.data();

		vkUpdateDescriptorSets(context.device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

		vkCmdBindDescriptorSets(context.command_buffers[context.current_frame].handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1, &vk_shader->descriptor_sets[context.current_frame], 0, nullptr);

	}

	void vulkan_renderer_texture_create(texture& t, uchar* pixels) {
		
		t.internal_data = vulkan_texture();
		vulkan_texture* vk_texture = std::any_cast<vulkan_texture>(&t.internal_data);
		VkDeviceSize image_size = t.width * t.height * t.channel_count;

		vulkan_buffer staging_buffer;
		vulkan_buffer_create(
			context,
			image_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true,
			staging_buffer
		);

		vulkan_buffer_load_data(
			context,
			staging_buffer,
			0,
			image_size,
			0,
			pixels
		);


		vulkan_image_create(
			context, 
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
			context,
			context.device.command_pool,
			temp_command_buffer
		);

			vulkan_image_transition_layout(
				context,
				temp_command_buffer,
				vk_texture->image,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);
			
			vulkan_image_copy_buffer_to_image(
				context,
				vk_texture->image,
				staging_buffer.handle,
				temp_command_buffer
			);

			vulkan_image_transition_layout(
				context,
				temp_command_buffer,
				vk_texture->image,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

		vulkan_command_buffer_end_single_use(
			context,
			context.device.command_pool,
			temp_command_buffer,
			context.device.graphics_queue
		);

		vulkan_buffer_destroy(context, staging_buffer);

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

		VK_CHECK(vkCreateSampler(context.device.logical_device, &sampler_info, nullptr, &vk_texture->sampler));

	}

	void vulkan_renderer_texture_destroy(texture& t) {
		vulkan_texture* vk_t = std::any_cast<vulkan_texture>(&t.internal_data);

		vkDestroySampler(context.device.logical_device, vk_t->sampler, nullptr);
		vkDestroyImageView(context.device.logical_device, vk_t->image.view, nullptr);
		vkDestroyImage(context.device.logical_device, vk_t->image.handle, nullptr);
		vkFreeMemory(context.device.logical_device, vk_t->image.memory, nullptr);
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
		viewport.y = (float)context.swapchain.extent.height;// Invert Y to match with Vulkan;
		viewport.width = (float)context.swapchain.extent.width;
		viewport.height = -(float)context.swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = context.swapchain.extent;

		// Attributes
		#define NUMBER_OF_VERTEX_ATTRIBUTES 4
		std::array < std::pair<VkFormat, uint>, NUMBER_OF_VERTEX_ATTRIBUTES > attributes_definitions{ 
			std::make_pair<VkFormat, uint>(VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)),
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
		sampler_layout_binding.descriptorCount = 3;// TODO: MORE CONFIGURABLE WITH THE vulkan_renderer_set_and_apply_uniforms
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
		VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout_info.bindingCount = bindings.size();
		layout_info.pBindings = bindings.data();
		VK_CHECK(vkCreateDescriptorSetLayout(context.device.logical_device, &layout_info, nullptr, &vk_shader->descriptor_set_layout));


		// Create pipeline
		bool result = vulkan_pipeline_create(
			context, 
			context.main_renderpass,//TODO: Choose renderpass depending the shader type
			sizeof(vertex),
			NUMBER_OF_VERTEX_ATTRIBUTES,
			attribute_descriptions.data(),
			1,
			vk_shader->descriptor_set_layout,
			2,
			shader_stages,
			viewport,
			scissor,
			true,
			vk_shader->pipeline
			);

		if (!result) {
			CE_LOG_ERROR("Couldn't create the pipeline for the shader: %s", s.name.c_str());
			return;
		}

		vkDestroyShaderModule(context.device.logical_device, vert_module, nullptr);
		vkDestroyShaderModule(context.device.logical_device, frag_module, nullptr);

		// Uniform buffers
		VkDeviceSize buffer_size = sizeof(uniform_buffer_object);

		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, vk_shader->uniform_buffers);
		vk_shader->uniform_buffers_mapped = vulkan_buffer_lock_memory(context, vk_shader->uniform_buffers, 0, buffer_size, 0);


		// Descriptor pool
		std::array<VkDescriptorPoolSize, 2> pool_sizes;
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = 1024; // HACK: max number of ubo descriptor sets.
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = 4096; // HACK: max number of image sampler descriptor sets.

		VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.poolSizeCount = pool_sizes.size();
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VK_CHECK(vkCreateDescriptorPool(context.device.logical_device, &pool_info, nullptr, &vk_shader->descriptor_pool));

		// Descriptor sets
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vk_shader->descriptor_set_layout);
		VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc_info.descriptorPool = vk_shader->descriptor_pool;
		alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		alloc_info.pSetLayouts = layouts.data();

		vk_shader->descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
		VK_CHECK(vkAllocateDescriptorSets(context.device.logical_device, &alloc_info, vk_shader->descriptor_sets.data()));

	}

	void vulkan_renderer_shader_destroy(shader& s) {

		vulkan_shader* vk_shader = std::any_cast<vulkan_shader>(&s.internal_data);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vulkan_buffer_unlock_memory(context, vk_shader->uniform_buffers);
			vk_shader->uniform_buffers_mapped = 0;
			vulkan_buffer_destroy(context, vk_shader->uniform_buffers);
		}
		vkDestroyDescriptorPool(context.device.logical_device, vk_shader->descriptor_pool, nullptr);
		vkDestroyDescriptorSetLayout(context.device.logical_device, vk_shader->descriptor_set_layout, nullptr);
		vulkan_pipeline_destroy(context, vk_shader->pipeline);
	}

	void vulkan_renderer_shader_use(shader& s) {
		vulkan_shader* vk_shader = &std::any_cast<vulkan_shader>(s.internal_data);
		vulkan_pipeline_bind(context.command_buffers[context.current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline);
	}


	void create_framebuffers() {
		context.swapchain.framebuffers.resize(context.swapchain.views.size());
		for (int i = 0; i < context.swapchain.views.size(); ++i) {
			VkImageView attachments[] = {
				context.swapchain.views[i],
				context.swapchain.depth_attachment.view
			};

			VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			framebuffer_info.renderPass = context.main_renderpass.handle;
			framebuffer_info.attachmentCount = 2;
			framebuffer_info.pAttachments = attachments;
			framebuffer_info.width = context.swapchain.extent.width;
			framebuffer_info.height = context.swapchain.extent.height;
			framebuffer_info.layers = 1;

			VK_CHECK(vkCreateFramebuffer(context.device.logical_device, &framebuffer_info, nullptr, &context.swapchain.framebuffers[i]));
		}
	}

	void destroy_framebuffers() {
		for (int i = 0; i < context.swapchain.framebuffers.size(); ++i) {
			vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], nullptr);
		}
	}

	void recreate_swapchain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(std::any_cast<GLFWwindow*>(platform_system_get_window()), &width, &height);
			glfwWaitEvents();
		}

		context.main_renderpass.render_area.z = width;
		context.main_renderpass.render_area.w = height;

		vkDeviceWaitIdle(context.device.logical_device);
		destroy_framebuffers();
		//vulkan_imageview_destroy(context);

		vulkan_swapchain_recreate(context, context.swapchain);
		create_framebuffers();
		create_command_buffers();
	}

	void create_command_buffers() {
		context.command_buffers.resize(context.swapchain.image_count);

		for (uint i = 0; i < context.swapchain.image_count; ++i) {
			if (context.command_buffers[i].handle) {
				vulkan_command_buffer_free(context, context.device.command_pool, context.command_buffers[i]);
			}

			vulkan_command_buffer_allocate(context, context.device.command_pool, true, context.command_buffers[i]);
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
		if (vkCreateShaderModule(context.device.logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			CE_LOG_ERROR("Could not create the shader");
			return VkShaderModule();
		}
		resource_system_unload(r);
		return shader_module;
	}


	//TODO: Refactor for batch rendering
	void create_vertex_buffer() {
		VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

		uint memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		
		vulkan_buffer staging_buffer;
		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memory_property_flags, true, staging_buffer);
		vulkan_buffer_load_data(context, staging_buffer, 0, buffer_size, 0, vertices.data());
	
		memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		uint usage_bit = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vulkan_buffer_create(context, buffer_size, (VkBufferUsageFlagBits)usage_bit, memory_property_flags, true, context.vertex_buffer);

		vulkan_buffer_copy(context, context.device.command_pool, 0, context.device.graphics_queue, staging_buffer.handle, 0, context.vertex_buffer.handle, 0, buffer_size);

		vulkan_buffer_destroy(context, staging_buffer);

	}

	void create_index_buffer() {
		VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

		uint memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		vulkan_buffer staging_buffer;
		vulkan_buffer_create(context, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memory_property_flags, true, staging_buffer);
		void* data;
		vulkan_buffer_load_data(context, staging_buffer, 0, buffer_size, 0, indices.data());

		memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		uint usage_bit = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		vulkan_buffer_create(context, buffer_size, (VkBufferUsageFlagBits)usage_bit, memory_property_flags, true, context.index_buffer);

		vulkan_buffer_copy(context, context.device.command_pool, 0, context.device.graphics_queue, staging_buffer.handle, 0, context.index_buffer.handle, 0, buffer_size);

		vulkan_buffer_destroy(context, staging_buffer);
	}

}