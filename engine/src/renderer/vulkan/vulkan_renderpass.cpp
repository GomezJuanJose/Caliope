#include "vulkan_renderpass.h"


namespace caliope {
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

	void vulkan_renderpass_begin(vulkan_command_buffer& command_buffer, vulkan_renderpass& renderpass, VkFramebuffer frame_buffer) {
		VkRenderPassBeginInfo renderpass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderpass_info.renderPass = renderpass.handle;
		renderpass_info.framebuffer = frame_buffer;
		renderpass_info.renderArea.offset.x = renderpass.render_area.x;
		renderpass_info.renderArea.offset.y = renderpass.render_area.y;
		renderpass_info.renderArea.extent.width = renderpass.render_area.z;
		renderpass_info.renderArea.extent.height = renderpass.render_area.w;
		std::array<VkClearValue, 2> clear_values;
		clear_values[0].color = { {renderpass.clear_color.r, renderpass.clear_color.g, renderpass.clear_color.b, renderpass.clear_color.a} };
		clear_values[1].depthStencil = { renderpass.depth, renderpass.stencil };
		renderpass_info.clearValueCount = clear_values.size();
		renderpass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffer.handle, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void vulkan_renderpass_end(vulkan_command_buffer& command_buffer) {
		vkCmdEndRenderPass(command_buffer.handle);
	}



}