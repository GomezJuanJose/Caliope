#pragma once

#include <vulkan/vulkan.h>

#include "core/logger.h"

#define VK_CHECK(expr)								\
	{												\
		if (expr != VK_SUCCESS) {					\
			CE_LOG_FATAL("Vulkan check failed");	\
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

	}vulkan_device;

	typedef struct vulkan_swapchain {
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR presentation_mode;
		VkExtent2D extent;

		VkSwapchainKHR handle;

		std::vector<VkImage> swapchain_images;
		std::vector<VkImageView> swapchain_image_views;

		std::vector<VkFramebuffer> framebuffers;

		uint image_count;
	} vulkan_swapchain;

	typedef struct vulkan_pipeline {
		VkPipelineLayout layout;
		VkPipeline handle;
	}vulkan_pipeline;

	typedef struct vulkan_renderpass {
		VkRenderPass handle;
	} vulkan_renderpass;

	typedef struct vulkan_command_buffer{
		VkCommandBuffer handle;
	}vulkan_command_buffer;

	typedef struct vulkan_buffer {

		uint64 total_size;
		VkBuffer handle;
		VkBufferUsageFlagBits usage;
		VkDeviceMemory memory;

		// TODO: freelist

	} vulkan_buffer;

	typedef struct vulkan_context {

		uint current_frame;
		bool framebuffer_resized;

		VkInstance instance;
		vulkan_device device;
		swapchain_support_details swapchain_details;

		VkSurfaceKHR surface;

		
		vulkan_swapchain swapchain;

		vulkan_pipeline pipeline;
		vulkan_renderpass renderpass;

		std::vector<VkSemaphore> image_available_semaphores;
		std::vector<VkSemaphore> render_finished_semaphores;
		std::vector<VkFence> in_flight_fences;

		std::vector<vulkan_command_buffer> command_buffers; // TODO: Make it compatible with triple buffering

		vulkan_buffer vertex_buffer;
		vulkan_buffer index_buffer;

		// TODO: remove and refactor
		VkDescriptorSetLayout descriptor_set_layout;
		std::vector<VkBuffer> uniform_buffers;
		std::vector<VkDeviceMemory> uniform_buffers_memory;
		std::vector<void*> uniform_buffers_mapped;
		VkDescriptorPool descriptor_pool;
		std::vector<VkDescriptorSet> descriptor_sets;

		VkImage texture_image;
		VkDeviceMemory texture_image_memory;
		VkImageView texture_image_view;
		VkSampler texture_sampler;
		VkImage depth_image;
		VkDeviceMemory depth_image_memory;
		VkImageView depth_image_view;

	} vulkan_context;
}