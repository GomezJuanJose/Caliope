#pragma once

#include <vulkan/vulkan.h>

#include "core/logger.h"
#include "core/asserts.h"

#include <glm/glm.hpp>

#define VK_CHECK(expr)								\
	{												\
		if (expr != VK_SUCCESS) {					\
			CE_LOG_FATAL("Vulkan check failed");	\
			CE_ASSERT(expr);						\
		}											\
	}

#define MAX_FRAMES_IN_FLIGHT 2

namespace caliope {
	typedef struct swapchain_support_details {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	}swapchain_support_details;

	typedef struct queue_family_indices {
		std::optional<uint> graphics_family;
		std::optional<uint> present_family;
	} queue_family_indices;

	typedef struct vulkan_device {
		VkPhysicalDevice physical_device;
		VkDevice logical_device;

		uint graphics_queue_index;
		uint presentation_queue_index;

		VkQueue graphics_queue;
		VkQueue presentation_queue;

		VkCommandPool command_pool;

		swapchain_support_details swapchain_support_details;

		VkFormat depth_format;

	}vulkan_device;


	typedef struct vulkan_image {
		VkImage handle;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		uint width;
		uint height;
	}vulkan_image;


	typedef struct vulkan_swapchain {
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR presentation_mode;
		VkExtent2D extent;

		VkSwapchainKHR handle;

		std::vector<vulkan_image> images;
		//std::vector<VkImage> images;
		//std::vector<VkImageView> views;

		vulkan_image depth_attachment;

		uint image_count;
	} vulkan_swapchain;

	typedef struct vulkan_pipeline {
		VkPipelineLayout layout;
		VkPipeline handle;
	}vulkan_pipeline;

	typedef struct vulkan_renderpass {
		VkRenderPass handle;
		glm::vec4 render_area;
		glm::vec4 clear_color;

		float depth;
		uint stencil;

		bool has_prev_pass;
		bool has_next_pass;

	} vulkan_renderpass;

	typedef struct vulkan_command_buffer{
		VkCommandBuffer handle;
	}vulkan_command_buffer;

	typedef struct vulkan_buffer {

		uint64 total_size;
		VkBuffer handle;
		VkBufferUsageFlagBits usage;
		VkDeviceMemory memory;
		VkMemoryPropertyFlags memory_property_flag;
		// TODO: freelist

	} vulkan_buffer;

	typedef struct vulkan_texture {
		vulkan_image image;
		VkSampler sampler;
	} vulkan_texture;

	typedef struct vulkan_shader {
		vulkan_pipeline pipeline;
		VkDescriptorPool descriptor_pool;

		VkDescriptorSetLayout descriptor_set_layout;

		vulkan_buffer uniform_buffers_vertex;
		void* uniform_buffers_vertex_mapped;

		vulkan_buffer uniform_buffers_fragment;
		void* uniform_buffers_fragment_mapped;

		vulkan_buffer ssbo;
		void* ssbo_mapped;

		vulkan_buffer textures;
		void* textures_mapped;

		std::vector<VkDescriptorSet> descriptor_sets;
		
	} vulkan_shader;

	typedef struct vulkan_geometry {
		vulkan_buffer vertex_buffer;
		vulkan_buffer index_buffer;
	} vulkan_geometry;

	typedef struct vulkan_context {
		uint image_index;
		uint current_frame;
		bool framebuffer_resized;

		VkInstance instance;
		vulkan_device device;

		VkSurfaceKHR surface;

		vulkan_swapchain swapchain;

		std::vector<VkSemaphore> image_available_semaphores;
		std::vector<VkSemaphore> render_finished_semaphores;
		std::vector<VkFence> in_flight_fences;

		std::vector<vulkan_command_buffer> command_buffers;
		
		std::vector<VkDescriptorImageInfo> batch_image_infos;

		// TODO: TEMPORAL
		vulkan_image color_object_pick_image;
		VkRenderPass object_pick_pass;
		VkFramebuffer object_pick_framebuffer;
		glm::vec4 object_pick_render_area;
		vulkan_shader object_pick_shader;
		std::vector<vulkan_command_buffer> object_pick_command_buffers;
		vulkan_buffer ssbo_pick_out;
		void* ssbo_pick_out_mapped;
		// TODO: END TEMPORAL

	} vulkan_context;
}