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
	} renderer_system_state;

	static std::unique_ptr<renderer_system_state> state_ptr;
	
	
	bool renderer_system_initialize(const std::string& application_name) {
		state_ptr = std::make_unique<renderer_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		renderer_backend_system_create(renderer_backend_type::BACKEND_TYPE_VULKAN, state_ptr->backend);

		if (!state_ptr->backend.initialize(application_name)) {
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

			for (auto [key, value] : packet.quad_definitions) {
				std::string mat_name = key;
				std::shared_ptr<material> mat = material_system_adquire(mat_name);
				renderer_shader_use(*mat->shader);

				glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 5.0f);

				for (uint i = 0; i < value.size(); ++i) {
					// TODO: Move this outside this for loop and here should be the quad generation vertex and index buffer with all data prepared for batch rendering
					state_ptr->backend.set_and_apply_uniforms(
						mat,
						transform_get_world(value[i]),
						camera_view_get(*packet.camera),
						camera_projection_get(*packet.camera),
						packet.camera->position
					);

					state_ptr->backend.draw_geometry();
				}
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