#include "ui_render_view.h"
#include "core/logger.h"

#include "renderer/camera.h"
#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "renderer/renderer_frontend.h"

#include "platform/platform.h"


namespace caliope {

	typedef struct uniform_vertex_buffer_object {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 view_position;
	}uniform_vertex_buffer_object;

	typedef struct ui_view_state {
		std::vector<shader_ui_quad_properties> quads;
		std::vector<texture*> batch_textures;

		uint binded_textures_count;
		uint max_textures_per_batch;

		float aspect_ratio;

	} ui_view_state;

	static std::unique_ptr<ui_view_state> state_ptr;


	void ui_render_view_on_create(render_view& self) {

		render_view_ui_config& internal_config = std::any_cast<render_view_ui_config>(self.internal_config);
		state_ptr = std::make_unique<ui_view_state>();

		state_ptr->quads.resize(internal_config.max_number_quads);
		state_ptr->batch_textures.resize(internal_config.max_textures_per_batch);

		state_ptr->binded_textures_count = 0;
		state_ptr->max_textures_per_batch = internal_config.max_textures_per_batch;

		state_ptr->aspect_ratio = (float)internal_config.window_width / (float)internal_config.window_height;
	}

	void ui_render_view_on_destroy(render_view& self) {
		state_ptr->quads.clear();
		state_ptr->batch_textures.clear();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	void ui_render_view_on_resize_window(render_view& self, uint width, uint height) {
		state_ptr->aspect_ratio = (float)width / (float)height;

		renderer_renderpass_set_render_area(self.renderpass, {0, 0, width, height});
	}

	bool ui_render_view_on_build_package(render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data) {

		std::vector<quad_definition>& quads = std::any_cast<std::vector<quad_definition>>(variadic_data[0]);
		camera* cam = std::any_cast<camera*>(variadic_data[1]);
		float delta_time = std::any_cast<float>(variadic_data[2]);

		render_view_ui_packet ui_packet;

		ui_packet.delta_time = delta_time;
		ui_packet.ui_camera = cam;

		for (uint index = 0; index < quads.size(); ++index) {
			ui_packet.quad_definitions.insert({ material_system_adquire(quads[index].material_name)->shader->name, {}});
			ui_packet.quad_definitions.at(
				material_system_adquire(quads[index].material_name)->shader->name
			).push(quads[index]);
		}
		
		
		out_packet.view_packet = ui_packet;

		return true;
	}

	bool ui_render_view_on_render(render_view& self, std::any& packet, uint render_target_index) {
		render_view_ui_packet& ui_packet = std::any_cast<render_view_ui_packet>(packet);

		if (!renderer_renderpass_begin(self.renderpass, render_target_index, glm::vec2(), glm::vec2())) {
			CE_LOG_ERROR("ui_render_view_on_render failed renderpass begin. Application shutting down");
			return false;
		}

		camera_aspect_ratio_set(*ui_packet.ui_camera, state_ptr->aspect_ratio);

		// Groups the materials and transforms by shader. This is to batch maximum information in a single drawcall
		//for (auto [shader_name, material_name] : packet.quad_materials) {
		for (auto [shader_name, sprites] : ui_packet.quad_definitions) {

			shader* shader = shader_system_adquire(std::string(shader_name));
			renderer_shader_use(*shader);

			uint number_of_instances = 0;
			uint texture_id = 0;


			while (!sprites.empty()) {
				quad_definition sprite = sprites.top();

				material* mat = material_system_adquire(sprite.material_name);

				texture* aux_diffuse_texture = mat->diffuse_texture ? mat->diffuse_texture : material_system_get_default()->diffuse_texture;
				uint diffuse_id = 0;

				if (state_ptr->batch_textures[aux_diffuse_texture->world_batch_index] && state_ptr->batch_textures[aux_diffuse_texture->world_batch_index]->name == aux_diffuse_texture->name) {
					diffuse_id = aux_diffuse_texture->world_batch_index;
				}
				else {
					if (mat->diffuse_texture) {
						state_ptr->batch_textures[texture_id] = mat->diffuse_texture;
						mat->diffuse_texture->world_batch_index = texture_id;
					}
					else {
						state_ptr->batch_textures[texture_id] = material_system_get_default()->diffuse_texture;
						material_system_get_default()->diffuse_texture->world_batch_index = texture_id;
					}
					diffuse_id = texture_id;
					texture_id++;
				}


				shader_ui_quad_properties sp;
				sp.model = transform_get_world(sprite.transform);
				sp.diffuse_color = mat->diffuse_color;
				sp.id = sprite.id;
				sp.diffuse_index = diffuse_id;
				sp.texture_region = sprite.texture_region;
				state_ptr->quads.at(number_of_instances) = sp;
				number_of_instances++;

				sprites.pop();
			}

			// Updates the descriptors, NOTE: Order matters!!!
			uniform_vertex_buffer_object ubo_vertex;
			ubo_vertex.view = camera_view_get(*ui_packet.ui_camera);
			ubo_vertex.proj = camera_projection_get(*ui_packet.ui_camera);
			ubo_vertex.view_position = ui_packet.ui_camera->position;
			renderer_set_descriptor_ubo(&ubo_vertex, sizeof(uniform_vertex_buffer_object), 0, *shader, 0);

			renderer_set_descriptor_sampler(state_ptr->batch_textures, 1, *shader);

			renderer_set_descriptor_ssbo(state_ptr->quads.data(), sizeof(shader_ui_quad_properties) * number_of_instances, 2, *shader, 1);


			renderer_apply_descriptors(*shader);
			renderer_draw_geometry(number_of_instances, *geometry_system_get_quad());

		}

		if (!renderer_renderpass_end()) {
			CE_LOG_ERROR("ui_render_view_on_render failed renderpass end. Application shutting down");
			return false;
		}
	}
}