#pragma once
#include "defines.h"
#include "renderer/vulkan/vulkan_types.inl"

namespace caliope {
	bool vulkan_swapchain_create(vulkan_context& context, vulkan_swapchain& out_swapchain);
	void vulkan_swapchain_recreate(vulkan_context& context, vulkan_swapchain& swapchain);
	void vulkan_swapchain_destroy(vulkan_context& context, vulkan_swapchain& swapchain);

	VkResult vulkan_swapchain_acquire_next_image_index(vulkan_context& context, vulkan_swapchain& swapchain, uint64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, uint& out_image_index);
	VkResult vulkan_swapchain_present(vulkan_context& context, vulkan_swapchain& swapchain, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint present_image_index);
}