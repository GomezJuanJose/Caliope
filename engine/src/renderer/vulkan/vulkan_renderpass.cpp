#include "vulkan_renderpass.h"

namespace caliope {
	bool vulkan_renderpass_create(vulkan_context& context) {
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = context.swapchain.surface_format.format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_reference = {};
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;

		VkRenderPassCreateInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderpass_info.attachmentCount = 1;
		renderpass_info.pAttachments = &color_attachment;
		renderpass_info.subpassCount = 1;
		renderpass_info.pSubpasses = &subpass;

		VK_CHECK(vkCreateRenderPass(context.device.logical_device, &renderpass_info, nullptr, &context.renderpass.handle));

		return true;
	}

	void vulkan_renderpass_destroy(vulkan_context& context) {
		vkDestroyRenderPass(context.device.logical_device, context.renderpass.handle, nullptr);
	}
}