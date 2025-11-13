#include "world_render_view.h"
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

	#define MAX_POINT_LIGHTS 10
	typedef struct uniform_fragment_buffer_object {
		glm::vec4 ambient_color;
		point_light_definition point_lights[MAX_POINT_LIGHTS];
		int number_of_lights;
	} uniform_fragment_buffer_object;

	typedef struct world_view_state {
		std::vector<shader_world_quad_properties> quads;
		std::vector<texture*> batch_textures;

		uint binded_textures_count;
		uint max_textures_per_batch;

		float aspect_ratio;

	} world_view_state;

	static std::unique_ptr<world_view_state> state_ptr;


	void world_render_view_on_create(render_view& self) {

		render_view_world_config& internal_config = std::any_cast<render_view_world_config>(self.internal_config);
		state_ptr = std::make_unique<world_view_state>();

		state_ptr->quads.resize(internal_config.max_number_quads);
		state_ptr->batch_textures.resize(internal_config.max_textures_per_batch);

		state_ptr->binded_textures_count = 0;
		state_ptr->max_textures_per_batch = internal_config.max_textures_per_batch;

		state_ptr->aspect_ratio = (float)internal_config.window_width / (float)internal_config.window_height;
	}

	void world_render_view_on_destroy(render_view& self) {
		state_ptr->quads.clear();
		state_ptr->batch_textures.clear();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	void world_render_view_on_resize_window(render_view& self, uint width, uint height) {
		state_ptr->aspect_ratio = (float)width / (float)height;

		renderer_renderpass_set_render_area(self.renderpass, {0, 0, width, height});
	}

	bool world_render_view_on_build_package(render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data) {

		std::vector<quad_definition>& quads = std::any_cast<std::vector<quad_definition>>(variadic_data[0]);
		std::vector<point_light_definition>& lights = std::any_cast<std::vector<point_light_definition>>(variadic_data[1]);
		camera* cam = std::any_cast<camera*>(variadic_data[2]);
		float delta_time = std::any_cast<float>(variadic_data[3]);

		render_view_world_packet world_packet;

		world_packet.ambient_color = { 0, 0, 0, 1 }; // TODO: Make a project settings configuration
		world_packet.delta_time = delta_time;
		world_packet.world_camera = cam;

		for (uint index = 0; index < quads.size(); ++index) {
			world_packet.sprite_definitions.insert({ material_system_adquire(quads[index].material_name)->shader->name, {}});
			world_packet.sprite_definitions.at(
				material_system_adquire(quads[index].material_name)->shader->name
			).push(quads[index]);
		}

		for (uint index = 0; index < lights.size(); ++index) {
			// TODO: Hardcoded config, move to an project settings config (the maximum number of lights)
			if (world_packet.point_light_definitions.size() < 10) {
				world_packet.point_light_definitions.push_back(lights[index]);
			}
		}

		out_packet.view_packet = world_packet;

		return true;
	}

	bool world_render_view_on_render(render_view& self, std::any& packet, uint render_target_index) {

		render_view_world_packet& world_packet = std::any_cast<render_view_world_packet>(packet);

		if (!renderer_renderpass_begin(self.renderpass, render_target_index, glm::vec2(), glm::vec2())) {
			CE_LOG_ERROR("world_render_view_on_render failed renderpass begin. Application shutting down");
			return false;
		}

		camera_aspect_ratio_set(*world_packet.world_camera, state_ptr->aspect_ratio);

		// Groups the materials and transforms by shader. This is to batch maximum information in a single drawcall
		//for (auto [shader_name, material_name] : packet.quad_materials) {
		for (auto [shader_name, sprites] : world_packet.sprite_definitions) {

			shader* shader = shader_system_adquire(std::string(shader_name));
			renderer_shader_use(*shader);

			uint number_of_instances = 0;
			uint texture_id = 0;


			//for (std::string material_name : packet.quad_materials[shader_name]) {
			while (!sprites.empty()) {
				quad_definition sprite = sprites.top();

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

				if (state_ptr->batch_textures[aux_specular_texture->world_batch_index] && state_ptr->batch_textures[aux_specular_texture->world_batch_index]->name == aux_specular_texture->name) {
					specula_id = aux_specular_texture->world_batch_index;
				}
				else {
					if (mat->specular_texture) {
						state_ptr->batch_textures[texture_id] = mat->specular_texture;
						mat->specular_texture->world_batch_index = texture_id;
					}
					else {
						state_ptr->batch_textures[texture_id] = material_system_get_default()->specular_texture;
						material_system_get_default()->specular_texture->world_batch_index = texture_id;
					}
					specula_id = texture_id;
					texture_id++;
				}

				if (state_ptr->batch_textures[aux_normal_texture->world_batch_index] && state_ptr->batch_textures[aux_normal_texture->world_batch_index]->name == aux_normal_texture->name) {
					normal_id = aux_normal_texture->world_batch_index;
				}
				else {
					if (mat->normal_texture) {
						state_ptr->batch_textures[texture_id] = mat->normal_texture;
						mat->normal_texture->world_batch_index = texture_id;

					}
					else {
						state_ptr->batch_textures[texture_id] = material_system_get_default()->normal_texture;
						material_system_get_default()->normal_texture->world_batch_index = texture_id;
					}
					normal_id = texture_id;
					texture_id++;
				}

				shader_world_quad_properties sp;
				sp.model = transform_get_world(sprite.transform);
				sp.diffuse_color = mat->diffuse_color;
				sp.id = sprite.id;
				sp.diffuse_index = diffuse_id;
				sp.specular_index = specula_id;
				sp.normal_index = normal_id;
				sp.shininess_sharpness = mat->shininess_sharpness;
				sp.shininess_intensity = mat->shininess_intensity;
				sp.texture_region = sprite.texture_region;
				state_ptr->quads.at(number_of_instances) = sp;
				number_of_instances++;

				sprites.pop();
			}

			// Updates the descriptors, NOTE: Order matters!!!
			uniform_vertex_buffer_object ubo_vertex;
			ubo_vertex.view = camera_view_get(*world_packet.world_camera);
			ubo_vertex.proj = camera_projection_get(*world_packet.world_camera);
			ubo_vertex.view_position = world_packet.world_camera->position;
			renderer_set_descriptor_ubo(&ubo_vertex, sizeof(uniform_vertex_buffer_object), 0, *shader, 0);

			renderer_set_descriptor_sampler(state_ptr->batch_textures, 1, *shader);

			renderer_set_descriptor_ssbo(state_ptr->quads.data(), sizeof(shader_world_quad_properties) * number_of_instances, 2, *shader, 2);

			uniform_fragment_buffer_object ubo_frag;
			ubo_frag.ambient_color = world_packet.ambient_color;
			ubo_frag.number_of_lights = world_packet.point_light_definitions.size();
			for (uint i = 0; i < ubo_frag.number_of_lights; ++i) {
				ubo_frag.point_lights[i] = world_packet.point_light_definitions[i];
			}
			renderer_set_descriptor_ubo(&ubo_frag, sizeof(uniform_fragment_buffer_object), 3, *shader, 1);
			std::vector<texture*> batch_textures = state_ptr->batch_textures;
			std::vector<shader_world_quad_properties> d = state_ptr->quads;
			renderer_apply_descriptors(*shader);
			renderer_draw_geometry(number_of_instances, *geometry_system_get_quad());

		}

		if (!renderer_renderpass_end()) {
			CE_LOG_ERROR("world_render_view_on_render failed renderpass end. Application shutting down");
			return false;
		}
	}
}