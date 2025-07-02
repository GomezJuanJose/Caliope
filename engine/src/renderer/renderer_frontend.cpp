#include "renderer_frontend.h"
#include "cepch.h"

#include "core/logger.h"

#include "renderer/renderer_types.inl"
#include "renderer/renderer_backend.h"
#include "renderer/camera.h"

#include "loaders/resources_types.inl"

#include "systems/shader_system.h"
#include "systems/material_system.h"

#include "math/transform.h"

//TODO: TEMPORAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace caliope{

	typedef struct renderer_system_state {
		renderer_backend backend;

		std::vector<quad_properties> quads;

		float aspect_ratio;
	} renderer_system_state;

	static std::unique_ptr<renderer_system_state> state_ptr;
	
	
	bool renderer_system_initialize(renderer_frontend_config& config) {
		state_ptr = std::make_unique<renderer_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->quads.resize(config.max_number_quads);
		state_ptr->aspect_ratio = (float)config.window_width / (float)config.window_height;

		renderer_backend_system_create(renderer_backend_type::BACKEND_TYPE_VULKAN, state_ptr->backend);


		renderer_backend_config backend_config;
		backend_config.application_name = config.application_name;
		backend_config.max_quads = config.max_number_quads;
		if (!state_ptr->backend.initialize(backend_config)) {
			CE_LOG_ERROR("Renderer backend failed to initialized. Shutting down");
			return false;
		}

		CE_LOG_INFO("Renderer system initialized.");
		return true;
	}

	void renderer_system_shutdown() {
		if (state_ptr != nullptr) {
			state_ptr->backend.shutdown();
			renderer_backend_system_destroy(state_ptr->backend);
			state_ptr.reset();
		}
	}

	void renderer_on_resized(uint16 width, uint16 height) {

		state_ptr->aspect_ratio = (float)width / (float)height;

		if (state_ptr->backend.resize != nullptr) {
			state_ptr->backend.resize(width, height);
		}
		else {
			CE_LOG_WARNING("Renderer backend does not support a resize function");
		}
	}

	bool renderer_draw_frame(renderer_packet& packet) {
		
		if (state_ptr->backend.begin_frame(packet.delta_time)) {


			
			if (!state_ptr->backend.begin_renderpass()) {
				CE_LOG_ERROR("renderer_draw_frame failed renderpass begin. Application shutting down");
				return false;
			}

			camera_aspect_ratio_set(*packet.world_camera, state_ptr->aspect_ratio);

			// Groups the materials by shader and all the transforms by material. This is to batch maximum information in a single drawcall
			for (auto [shader_name, material_name] : packet.quad_materials) {

				uint number_of_instances = 0;
				std::string sn = shader_name;
				std::shared_ptr<shader> shader = shader_system_adquire(sn);
				renderer_shader_use(*shader);

				
				for (std::string material_name : packet.quad_materials[shader_name]) {
					std::shared_ptr<material> mat = material_system_adquire(material_name);
					std::vector<transform>& transforms = packet.quad_transforms[material_name];

					for (uint i = 0; i < transforms.size(); ++i) {
						quad_properties qp;
						qp.model = transform_get_world(transforms[i]);
						state_ptr->quads.at(i) = qp;
						number_of_instances++;
					}

				}

				state_ptr->backend.set_and_apply_uniforms(
					state_ptr->quads,
					shader->internal_data,
					number_of_instances,
					camera_view_get(*packet.world_camera),
					camera_projection_get(*packet.world_camera),
					packet.world_camera->position
				);

				state_ptr->backend.draw_geometry(number_of_instances);
			}

			if (!state_ptr->backend.end_renderpass()) {
				CE_LOG_ERROR("renderer_draw_frame failed renderpass end. Application shutting down");
				return false;
			}


			if (!state_ptr->backend.end_frame(packet.delta_time)) {
				CE_LOG_ERROR("renderer_end_frame failed. Application shutting down");
				return false;
			}
		}

		return true;
	}

	void renderer_texture_create(texture& texture, uchar* pixels) {
		state_ptr->backend.texture_create(texture, pixels);
	}

	void renderer_texture_destroy(texture& texture) {
		state_ptr->backend.texture_destroy(texture);
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

}