#include "renderer_frontend.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"

#include "renderer/renderer_types.inl"
#include "renderer/renderer_backend.h"
#include "renderer/camera.h"

#include "resources/resources_types.inl"

#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"


#include "systems/render_view_system.h"

#include "math/transform.h"



namespace caliope{

	typedef struct renderer_system_state {
		renderer_backend backend;
		bool is_resizing;
		std::unordered_map<renderpass_type, renderpass> renderpasses;
	} renderer_system_state;

	static std::unique_ptr<renderer_system_state> state_ptr;
	
	void generate_render_targets();

	bool renderer_system_initialize(renderer_frontend_config& config) {
		state_ptr = std::make_unique<renderer_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		renderer_backend_system_create(renderer_backend_type::BACKEND_TYPE_VULKAN, state_ptr->backend);


		renderer_backend_config backend_config;
		backend_config.application_name = config.application_name;
		if (!state_ptr->backend.initialize(backend_config)) {
			CE_LOG_ERROR("Renderer backend failed to initialized. Shutting down");
			return false;
		}

		// Creates renderpasses
		renderpass pass;

		// World renderpass
		pass = {};
		pass.type = RENDERPASS_TYPE_WORLD;
		pass.flags = (renderpass_clear_flag)(RENDERPASS_CLEAR_FLAG_COLOR_BUFFER | RENDERPASS_CLEAR_FLAG_DEPTH_BUFFER);
		pass.targets.resize(state_ptr->backend.window_images_count_get());
		renderpass_resource_data world_renderpass_data;
		world_renderpass_data.clear_color = glm::vec4(0.0f, 0.2f, 0.6f, 1.0f);
		world_renderpass_data.depth = 1.0f;
		world_renderpass_data.stencil = 0;
		world_renderpass_data.has_next_pass = true;
		world_renderpass_data.has_prev_pass = false;
		world_renderpass_data.attachment_formats = { ATTACHMENT_FORMAT_TYPE_SWAPCHAIN };
		world_renderpass_data.subpass_src_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_LATE_FRAGMENT_TESTS;
		world_renderpass_data.subpass_src_access_mask = RENDERPASS_ACCESS_TYPE_DEPTH_STENCIL_ATTACHMENT_WRITE;
		world_renderpass_data.subpass_dst_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_EARLY_FRAGMENT_TESTS;
		world_renderpass_data.subpass_dst_access_mask = RENDERPASS_ACCESS_TYPE_COLOR_ATTACHMENT_READ | RENDERPASS_ACCESS_TYPE_COLOR_ATTACHMENT_WRITE | RENDERPASS_ACCESS_TYPE_DEPTH_STENCIL_ATTACHMENT_WRITE;
		state_ptr->backend.renderpass_create(pass, world_renderpass_data);
		state_ptr->renderpasses.insert({pass.type, pass});

		// UI renderpass
		pass = {};
		pass.type = RENDERPASS_TYPE_UI;
		pass.flags = (renderpass_clear_flag)(RENDERPASS_CLEAR_FLAG_NONE);
		pass.targets.resize(state_ptr->backend.window_images_count_get());
		renderpass_resource_data ui_renderpass_data;
		ui_renderpass_data.clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		ui_renderpass_data.depth = 1.0f;
		ui_renderpass_data.stencil = 0;
		ui_renderpass_data.has_next_pass = false;
		ui_renderpass_data.has_prev_pass = true;
		ui_renderpass_data.attachment_formats = { ATTACHMENT_FORMAT_TYPE_SWAPCHAIN };
		ui_renderpass_data.subpass_src_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_LATE_FRAGMENT_TESTS;
		ui_renderpass_data.subpass_src_access_mask = RENDERPASS_ACCESS_TYPE_DEPTH_STENCIL_ATTACHMENT_WRITE;
		ui_renderpass_data.subpass_dst_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_EARLY_FRAGMENT_TESTS;
		ui_renderpass_data.subpass_dst_access_mask = RENDERPASS_ACCESS_TYPE_COLOR_ATTACHMENT_READ | RENDERPASS_ACCESS_TYPE_COLOR_ATTACHMENT_WRITE | RENDERPASS_ACCESS_TYPE_DEPTH_STENCIL_ATTACHMENT_WRITE;
		state_ptr->backend.renderpass_create(pass, ui_renderpass_data);
		state_ptr->renderpasses.insert({ pass.type, pass });

		// World pick object renderpass
		pass = {};
		pass.type = RENDERPASS_TYPE_WORLD_OBJECT_PICK;
		pass.flags = (renderpass_clear_flag)(RENDERPASS_CLEAR_FLAG_COLOR_BUFFER);
		pass.targets.resize(state_ptr->backend.window_images_count_get());
		renderpass_resource_data world_pick_object_renderpass_data;
		world_pick_object_renderpass_data.clear_color = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
		world_pick_object_renderpass_data.depth = 1.0f;
		world_pick_object_renderpass_data.stencil = 0;
		world_pick_object_renderpass_data.has_prev_pass = false;
		world_pick_object_renderpass_data.has_next_pass = false;
		world_pick_object_renderpass_data.attachment_formats = { ATTACHMENT_FORMAT_TYPE_R32_SFLOAT };
		world_pick_object_renderpass_data.subpass_src_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_EARLY_FRAGMENT_TESTS;
		world_pick_object_renderpass_data.subpass_src_access_mask = 0;
		world_pick_object_renderpass_data.subpass_dst_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_EARLY_FRAGMENT_TESTS;
		world_pick_object_renderpass_data.subpass_dst_access_mask =  RENDERPASS_ACCESS_TYPE_COLOR_ATTACHMENT_WRITE;
		state_ptr->backend.renderpass_create(pass, world_pick_object_renderpass_data);
		state_ptr->renderpasses.insert({ pass.type, pass });

		// UI pick object renderpass
		pass = {};
		pass.type = RENDERPASS_TYPE_UI_OBJECT_PICK;
		pass.flags = (renderpass_clear_flag)(RENDERPASS_CLEAR_FLAG_COLOR_BUFFER);
		pass.targets.resize(state_ptr->backend.window_images_count_get());
		renderpass_resource_data ui_pick_object_renderpass_data;
		ui_pick_object_renderpass_data.clear_color = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
		ui_pick_object_renderpass_data.depth = 1.0f;
		ui_pick_object_renderpass_data.stencil = 0;
		ui_pick_object_renderpass_data.has_prev_pass = false;
		ui_pick_object_renderpass_data.has_next_pass = false;
		ui_pick_object_renderpass_data.attachment_formats = { ATTACHMENT_FORMAT_TYPE_R32_SFLOAT };
		ui_pick_object_renderpass_data.subpass_src_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_EARLY_FRAGMENT_TESTS;
		ui_pick_object_renderpass_data.subpass_src_access_mask = 0;
		ui_pick_object_renderpass_data.subpass_dst_stage_mask = RENDERPASS_STAGE_TYPE_COLOR_ATTACHMENT_OUTPUT | RENDERPASS_STAGE_TYPE_EARLY_FRAGMENT_TESTS;
		ui_pick_object_renderpass_data.subpass_dst_access_mask = RENDERPASS_ACCESS_TYPE_COLOR_ATTACHMENT_WRITE;
		state_ptr->backend.renderpass_create(pass, ui_pick_object_renderpass_data);
		state_ptr->renderpasses.insert({ pass.type, pass });

		generate_render_targets();
		

		CE_LOG_INFO("Renderer system initialized.");
		return true;
	}

	void renderer_system_stop() {
		state_ptr->backend.stop();
	}

	void renderer_system_shutdown() {
		if (state_ptr != nullptr) {
			for (auto pass : state_ptr->renderpasses) {
				state_ptr->backend.renderpass_destroy(pass.second);
				
				for (uint i = 0; i < pass.second.targets.size(); ++i) {
					state_ptr->backend.render_target_destroy(pass.second.targets[i]);
				}
			
			}

			state_ptr->backend.shutdown();
			renderer_backend_system_destroy(state_ptr->backend);
			state_ptr.reset();
		}
		state_ptr = nullptr;
	}

	void renderer_on_resized(uint16 width, uint16 height) {

		render_view_system_on_window_resize(width, height);
		state_ptr->is_resizing = true;
		if (state_ptr->backend.resize != nullptr) {
			state_ptr->backend.resize(width, height);
		}
		else {
			CE_LOG_WARNING("Renderer backend does not support a resize function");
		}
	}

	bool renderer_draw_frame(std::vector<renderer_view_packet>& packets, float delta_time) {
		
		if (state_ptr->backend.begin_frame(delta_time)) {

			uint render_target_index = state_ptr->backend.window_image_index_get();

			for(uint index = 0; index < packets.size(); ++index){
				render_view_system_on_render(packets[index].view_type, packets[index].view_packet, render_target_index);
			}
	
			if (!state_ptr->backend.end_frame(delta_time)) {
				CE_LOG_ERROR("renderer_end_frame failed. Application shutting down");
				return false;
			}
		}

		if (state_ptr->is_resizing == true) {
			generate_render_targets();
			state_ptr->is_resizing = false;
		}

		return true;
	}

	void renderer_texture_create(texture& texture, uchar* pixels) {
		state_ptr->backend.texture_create(texture, pixels);
	}

	void renderer_texture_create_writeable(texture& texture)
	{
		state_ptr->backend.texture_create_writeable(texture);
	}

	void renderer_texture_destroy(texture& texture) {
		state_ptr->backend.texture_destroy(texture);
	}

	void renderer_texture_write_data(texture& t, uint offset, uint size, uchar* pixels)
	{
		state_ptr->backend.texture_write_data(t, offset, size, pixels);
	}

	void renderer_texture_change_filter(texture& texture) {
		state_ptr->backend.texture_change_filter(texture);
	}

	bool renderer_shader_create(shader_resource_data& shader_config, shader& out_shader) {
		return state_ptr->backend.shader_create(shader_config, out_shader, state_ptr->renderpasses.at(shader_config.renderpass_type));
	}

	void renderer_shader_destroy(shader& shader) {
		state_ptr->backend.shader_destroy(shader);
	}

	void renderer_shader_use(shader& shader) {
		state_ptr->backend.shader_use(shader);
	}

	void renderer_geometry_create(geometry& geometry, std::vector<vertex>& vertices, std::vector<uint16>& indices) {
		state_ptr->backend.geometry_create(geometry, vertices, indices);
	}

	void renderer_geometry_destroy(geometry& geometry) {
		state_ptr->backend.geometry_destroy(geometry);
	}

	bool renderer_renderpass_begin(renderpass_type type, uint render_target_index, glm::vec2 scissor_extent, glm::vec2 scissor_offset) {
		return state_ptr->backend.renderpass_begin(state_ptr->renderpasses.at(type), state_ptr->renderpasses.at(type).targets[render_target_index], scissor_extent, scissor_offset);
	}

	bool renderer_renderpass_end() {
		return state_ptr->backend.renderpass_end();
	}

	void renderer_renderpass_set_render_area(renderpass_type type, glm::vec4 render_area) {
		state_ptr->renderpasses.at(type).render_area = render_area;
	}

	void renderer_set_descriptor_ubo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index)
	{
		state_ptr->backend.set_descriptor_ubo(data, data_size, destination_binding, shader, descriptor_buffer_index);
	}

	void renderer_set_descriptor_sampler(std::vector<texture*>& textures_batch_ptr, uint destination_binding, shader& shader)
	{
		state_ptr->backend.set_descriptor_sampler(textures_batch_ptr, destination_binding, shader);
	}

	void renderer_set_descriptor_ssbo(void* data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index)
	{
		state_ptr->backend.set_descriptor_ssbo(data, data_size, destination_binding, shader, descriptor_buffer_index);
	}

	void renderer_apply_descriptors(shader& shader)
	{
		state_ptr->backend.apply_descriptors(shader);
	}

	void renderer_get_descriptor_ssbo(void* out_data, uint64 data_size, uint destination_binding, shader& shader, uint descriptor_buffer_index)
	{
		state_ptr->backend.get_descriptor_ssbo(out_data, data_size, destination_binding, shader, descriptor_buffer_index);
	}

	void renderer_draw_geometry(uint instance_count, geometry& geometry) {
		state_ptr->backend.draw_geometry(instance_count, geometry);
	}

	void generate_render_targets() {
		uint max_window_images = state_ptr->backend.window_images_count_get();
		for (uint i = 0; i < max_window_images; ++i) {
			renderpass& world_pass = state_ptr->renderpasses.at(RENDERPASS_TYPE_WORLD);

			state_ptr->backend.render_target_destroy(world_pass.targets[i]);
			world_pass.targets[i].attachments = { state_ptr->backend.window_attachment_get(i), state_ptr->backend.depth_attachment_get() };
			state_ptr->backend.render_target_create(world_pass, world_pass.targets[i]);

			renderpass& ui_pass = state_ptr->renderpasses.at(RENDERPASS_TYPE_UI);
			state_ptr->backend.render_target_destroy(ui_pass.targets[i]);
			ui_pass.targets[i].attachments = { state_ptr->backend.window_attachment_get(i)};
			state_ptr->backend.render_target_create(ui_pass, ui_pass.targets[i]);

			renderpass& world_object_pick_pass = state_ptr->renderpasses.at(RENDERPASS_TYPE_WORLD_OBJECT_PICK);
			state_ptr->backend.render_target_destroy(world_object_pick_pass.targets[i]);
			world_object_pick_pass.targets[i].attachments = { state_ptr->backend.object_pick_attachment_get() };
			state_ptr->backend.render_target_create(world_object_pick_pass, world_object_pick_pass.targets[i]);

			renderpass& ui_object_pick_pass = state_ptr->renderpasses.at(RENDERPASS_TYPE_UI_OBJECT_PICK);
			state_ptr->backend.render_target_destroy(ui_object_pick_pass.targets[i]);
			ui_object_pick_pass.targets[i].attachments = { state_ptr->backend.object_pick_attachment_get() };
			state_ptr->backend.render_target_create(ui_object_pick_pass, ui_object_pick_pass.targets[i]);
		}


	}
}