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

#include "math/transform.h"

//TODO: TEMPORAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace caliope{

	typedef struct renderer_system_state {
		renderer_backend backend;

		std::vector<sprite_properties> quads;
		std::vector<texture*> batch_textures;
		float aspect_ratio;

		uint binded_textures_count;
		uint max_textures_per_batch;
	} renderer_system_state;

	static std::unique_ptr<renderer_system_state> state_ptr;
	
	
	bool renderer_system_initialize(renderer_frontend_config& config) {
		state_ptr = std::make_unique<renderer_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->quads.resize(config.max_number_quads);
		state_ptr->batch_textures.resize(config.max_textures_per_batch);
		state_ptr->aspect_ratio = (float)config.window_width / (float)config.window_height;
		
		state_ptr->binded_textures_count = 0;
		state_ptr->max_textures_per_batch = config.max_textures_per_batch;

		renderer_backend_system_create(renderer_backend_type::BACKEND_TYPE_VULKAN, state_ptr->backend);


		renderer_backend_config backend_config;
		backend_config.application_name = config.application_name;
		backend_config.max_quads = config.max_number_quads;
		backend_config.max_textures_per_batch = config.max_textures_per_batch;
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

			// Groups the materials and transforms by shader. This is to batch maximum information in a single drawcall
			//for (auto [shader_name, material_name] : packet.quad_materials) {
			for (auto [shader_name, sprites] : packet.sprite_definitions) {

				std::string sn = shader_name;
				shader* shader = shader_system_adquire(sn);
				renderer_shader_use(*shader);
				
				uint number_of_instances = 0;
				uint texture_id = 0;


				//for (std::string material_name : packet.quad_materials[shader_name]) {
				while (!sprites.empty()){
					sprite_definition sprite = sprites.top();

					material* mat = material_system_adquire(sprite.material_name);
					//std::vector<transform>& transforms = packet.quad_transforms[material_name];
					
					texture* aux_diffuse_texture = mat->diffuse_texture ? mat->diffuse_texture : material_system_get_default()->diffuse_texture;
					texture* aux_specular_texture = mat->specular_texture ? mat->specular_texture : material_system_get_default()->specular_texture;
					texture* aux_normal_texture = mat->normal_texture ? mat->normal_texture : material_system_get_default()->normal_texture;

					// TODO: detect batch and better integration with the algorithm
					// TODO: Probar a hacer que cuando el texture_id(es decir el numero de texturas que lleva en el batch) sobrepase el maximo sutituya los primeros(pero antes de eso tiene que haber hecho el draw)
					// tambien poner a -1 el id de la textura que será sustituida
					/**uint new_textures_count = 0;
					if (!state_ptr->batch_textures[aux_diffuse_texture->id] || state_ptr->batch_textures[aux_diffuse_texture->id]->name != aux_diffuse_texture->name) {
						new_textures_count++;
					}
					if (!state_ptr->batch_textures[aux_specular_texture->id] || state_ptr->batch_textures[aux_specular_texture->id]->name != aux_specular_texture->name) {
						new_textures_count++;
					}
					if (!state_ptr->batch_textures[aux_normal_texture->id] || state_ptr->batch_textures[aux_normal_texture->id]->name != aux_normal_texture->name) {
						new_textures_count++;
					}

					if (state_ptr->binded_textures_count + new_textures_count > state_ptr->max_textures_per_batch) {
						state_ptr->binded_textures_count = 0;
						texture_id = 0;
					
						// TODO: Here goes the batch detection, do a break and store the loops status to make a renderpass with this batch and start again the loop without ending the frame
					}

					state_ptr->binded_textures_count += new_textures_count;*/
					//-----------------

					uint diffuse_id = 0;
					uint specula_id = 0;
					uint normal_id = 0;

					// TODO: INSTEAD OF COMPARE NAMES COMPARE RANDOM NUMBERS OR HASHED NAMES FOR BETTER PERFORMANCE
					// TODO: DRY
					if (state_ptr->batch_textures[aux_diffuse_texture->id] && state_ptr->batch_textures[aux_diffuse_texture->id]->name == aux_diffuse_texture->name) {
						diffuse_id = aux_diffuse_texture->id;
					}
					else {
						if (mat->diffuse_texture) {
							state_ptr->batch_textures[texture_id] = mat->diffuse_texture;
							mat->diffuse_texture->id = texture_id;
						}
						else {
							state_ptr->batch_textures[texture_id] = material_system_get_default()->diffuse_texture;
							material_system_get_default()->diffuse_texture->id = texture_id;
						}
						diffuse_id = texture_id;
						texture_id++;
					}

					if (state_ptr->batch_textures[aux_specular_texture->id] && state_ptr->batch_textures[aux_specular_texture->id]->name == aux_specular_texture->name) {
						specula_id = aux_specular_texture->id;
					}
					else {
						if (mat->specular_texture) {
							state_ptr->batch_textures[texture_id] = mat->specular_texture;
							mat->specular_texture->id = texture_id;
						}
						else {
							state_ptr->batch_textures[texture_id] = material_system_get_default()->specular_texture;
							material_system_get_default()->specular_texture->id = texture_id;
						}
						specula_id = texture_id;
						texture_id++;
					}

					if (state_ptr->batch_textures[aux_normal_texture->id] && state_ptr->batch_textures[aux_normal_texture->id]->name == aux_normal_texture->name) {
						normal_id = aux_normal_texture->id;
					}
					else {
						if (mat->normal_texture) {
							state_ptr->batch_textures[texture_id] = mat->normal_texture;
							mat->normal_texture->id = texture_id;

						}
						else {
							state_ptr->batch_textures[texture_id] = material_system_get_default()->normal_texture;
							material_system_get_default()->normal_texture->id = texture_id;
						}
						normal_id = texture_id;
						texture_id++;
					}

					//for (uint i = 0; i < transforms.size(); ++i) {
					sprite_properties sp;
					sp.model = transform_get_world(sprite.transform/*transforms[i]*/);
					sp.diffuse_color = mat->diffuse_color;
					sp.diffuse_index = diffuse_id;
					sp.specular_index = specula_id;
					sp.normal_index = normal_id;
					sp.shininess_sharpness = mat->shininess_sharpness;
					sp.shininess_intensity = mat->shininess_intensity;
					sp.texture_region = sprite.texture_region;
					state_ptr->quads.at(number_of_instances) = sp;
					number_of_instances++;
					//}
					sprites.pop();
				}

				state_ptr->backend.set_and_apply_uniforms(
					state_ptr->quads,
					shader->internal_data,
					state_ptr->batch_textures,
					number_of_instances,
					camera_view_get(*packet.world_camera),
					camera_projection_get(*packet.world_camera),
					packet.world_camera->position
				);

				state_ptr->backend.draw_geometry(number_of_instances, *geometry_system_get_quad());
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

}