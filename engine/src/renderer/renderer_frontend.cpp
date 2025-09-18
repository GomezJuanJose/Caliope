#include "renderer_frontend.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"

#include "renderer/renderer_types.inl"
#include "renderer/renderer_backend.h"
#include "renderer/camera.h"

#include "loaders/resources_types.inl"

#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"


#include "systems/render_view_system.h"

#include "math/transform.h"



namespace caliope{

	typedef struct renderer_system_state {
		renderer_backend backend;
	} renderer_system_state;

	static std::unique_ptr<renderer_system_state> state_ptr;
	

	
	bool renderer_system_initialize(renderer_frontend_config& config) {
		state_ptr = std::make_unique<renderer_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		renderer_backend_system_create(renderer_backend_type::BACKEND_TYPE_VULKAN, state_ptr->backend);


		renderer_backend_config backend_config;
		backend_config.application_name = config.application_name;
		backend_config.max_quads = config.max_number_quads;// TODO: Remove
		backend_config.max_textures_per_batch = config.max_textures_per_batch;// TODO: Remove
		if (!state_ptr->backend.initialize(backend_config)) {
			CE_LOG_ERROR("Renderer backend failed to initialized. Shutting down");
			return false;
		}

		CE_LOG_INFO("Renderer system initialized.");
		return true;
	}

	void renderer_system_stop() {
		state_ptr->backend.stop();
	}

	void renderer_system_shutdown() {
		if (state_ptr != nullptr) {
			state_ptr->backend.shutdown();
			renderer_backend_system_destroy(state_ptr->backend);
			state_ptr.reset();
		}
		state_ptr = nullptr;
	}

	void renderer_on_resized(uint16 width, uint16 height) {

		render_view_system_on_window_resize(width, height);

		if (state_ptr->backend.resize != nullptr) {
			state_ptr->backend.resize(width, height);
		}
		else {
			CE_LOG_WARNING("Renderer backend does not support a resize function");
		}
	}

	bool renderer_draw_frame(std::vector<renderer_view_packet>& packets, float delta_time) {
		
		if (state_ptr->backend.begin_frame(delta_time)) {

			for(uint index = 0; index < packets.size(); ++index){
				render_view_system_on_render(packets[index].view_type, packets[index].view_packet);
			}

			if (!state_ptr->backend.end_frame(delta_time)) {
				CE_LOG_ERROR("renderer_end_frame failed. Application shutting down");
				return false;
			}

			state_ptr->backend.show_picked_obj();
		}

		return true;
	}

	void renderer_texture_create(texture& texture, uchar* pixels) {
		state_ptr->backend.texture_create(texture, pixels);
	}

	void renderer_texture_destroy(texture& texture) {
		state_ptr->backend.texture_destroy(texture);
	}

	void renderer_texture_change_filter(texture& texture) {
		state_ptr->backend.texture_change_filter(texture);
	}

	void renderer_shader_create(shader& shader) {
		state_ptr->backend.shader_create(shader);
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

	bool renderer_renderpass_begin() {
		return state_ptr->backend.begin_renderpass();
	}

	bool renderer_renderpass_end() {
		return state_ptr->backend.end_renderpass();
	}


	void renderer_set_and_apply_uniforms(std::vector<quad_properties>& quads, std::vector<point_light_definition>& point_lights, glm::vec4 ambient_color, std::any& shader_internal_data, std::vector<texture*>& textures_batch_ptr, uint quad_count, glm::mat4& view, glm::mat4& projection, glm::vec3& view_position) {
		state_ptr->backend.set_and_apply_uniforms(
			quads,
			point_lights,
			ambient_color,
			shader_internal_data,
			textures_batch_ptr,
			quad_count,
			view,
			projection,
			view_position
		);
	}
	void renderer_draw_geometry(uint instance_count, geometry& geometry) {
		state_ptr->backend.draw_geometry(instance_count, geometry);
	}

	void renderer_draw_object_pick(uint instance_count, std::vector<pick_quad_properties>& quads, geometry& geometry, glm::mat4& projection, glm::mat4& view) {
		state_ptr->backend.draw_object_pick(instance_count, quads, geometry, projection, view);

	}
}