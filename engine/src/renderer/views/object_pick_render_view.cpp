#include "object_pick_render_view.h"
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

	typedef struct ssbo_object_picking {
		uint id;
	}ssbo_object_picking;


	typedef struct object_pick_view_state {
		std::vector<shader_pick_quad_properties> pick_objects;
		std::vector<texture*> batch_textures;

		uint binded_textures_count;
		uint max_textures_per_batch;

		float aspect_ratio;


	}object_pick_view_state;

	static std::unique_ptr<object_pick_view_state> state_ptr;

	void object_pick_render_view_on_create(render_view& self) {
		render_view_pick_config& internal_config = std::any_cast<render_view_pick_config>(self.internal_config);
		state_ptr = std::make_unique<object_pick_view_state>();

		state_ptr->pick_objects.resize(internal_config.max_number_quads);
		state_ptr->batch_textures.resize(internal_config.max_textures_per_batch);

		state_ptr->binded_textures_count = 0;
		state_ptr->max_textures_per_batch = internal_config.max_textures_per_batch;

		state_ptr->aspect_ratio = (float)internal_config.window_width / (float)internal_config.window_height;
	}

	void object_pick_render_view_on_destroy(render_view& self) {
		state_ptr->batch_textures.clear();
		state_ptr->pick_objects.clear();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	void object_pick_render_view_on_resize_window(render_view& self, uint width, uint height) {
		state_ptr->aspect_ratio = (float)width / (float)height;

		renderer_renderpass_set_render_area(self.renderpass, { 0, 0, width, height });
	}

	bool object_pick_render_view_on_build_package(render_view& self, renderer_view_packet& out_packet, std::vector<std::any>& variadic_data) {
		std::vector<quad_definition>& quads = std::any_cast<std::vector<quad_definition>>(variadic_data[0]);
		camera* cam = std::any_cast<camera*>(variadic_data[1]);
		float delta_time = std::any_cast<float>(variadic_data[2]);

		render_view_object_pick_packet object_pick_packet;
		object_pick_packet.delta_time = delta_time;
		object_pick_packet.world_camera = cam;

		object_pick_packet.sprite_definitions.insert({ std::string("Builtin.ObjectPickShader"), {} });//TODO: TEMPORAL
		
		for (uint index = 0; index < quads.size(); ++index) {
			object_pick_packet.sprite_definitions.at(std::string("Builtin.ObjectPickShader")).push(quads[index]);//TODO: TEMPORAL
		}

		out_packet.view_packet = object_pick_packet;

		return true;
	}

	bool object_pick_render_view_on_render(render_view& self, std::any& packet, uint render_target_index) {

		//TODO: ADAPT IT TO THE PICK UP SHADER
		render_view_object_pick_packet& object_pick_packet = std::any_cast<render_view_object_pick_packet>(packet);

		double cursor_x, cursor_y;
		glm::vec2 scissor_extent;
		glm::vec2 scissor_offset;

		glm::vec2 cursor_position = platform_system_get_cursor_position();
		scissor_offset.x = cursor_position.x > 0 ? cursor_position.x : 0;
		scissor_offset.y = cursor_position.y > 0 ? cursor_position.y : 0;
		scissor_extent = { 1,1 };

		if (!renderer_renderpass_begin(self.renderpass, render_target_index, scissor_extent, scissor_offset)) {
			CE_LOG_ERROR("world_render_view_on_render failed renderpass begin. Application shutting down");
			return false;
		}

		shader* shader = shader_system_adquire(std::string("Builtin.ObjectPickShader"));//TODO: TEMPORAL
		
		// Groups the materials and transforms by shader. This is to batch maximum information in a single drawcall
		//for (auto [shader_name, material_name] : packet.quad_materials) {
		for (auto [shader_name, sprites] : object_pick_packet.sprite_definitions) {

			renderer_shader_use(*shader);
			uint instance_index = sprites.size() - 1;
			uint number_of_instances = 0;
			uint texture_id = 0;

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

				shader_pick_quad_properties psp;
				psp.model = transform_get_world(sprite.transform);
				psp.id = sprite.id;
				psp.diffuse_index = diffuse_id;
				psp.texture_region = sprite.texture_region;
				state_ptr->pick_objects.at(instance_index) = psp;

				number_of_instances++;
				instance_index--;
				//}
				sprites.pop();
			}

			// Updates the descriptors, NOTE: Order matters!!!
			uniform_vertex_buffer_object ubo_vertex;
			ubo_vertex.view = camera_view_get(*object_pick_packet.world_camera);
			ubo_vertex.proj = camera_projection_get(*object_pick_packet.world_camera);
			ubo_vertex.view_position = object_pick_packet.world_camera->position;
			renderer_set_descriptor_ubo(&ubo_vertex, sizeof(uniform_vertex_buffer_object), 0, *shader, 0);
			
			renderer_set_descriptor_ssbo(state_ptr->pick_objects.data(), sizeof(shader_pick_quad_properties)* number_of_instances, 1, *shader, 1);
			renderer_set_descriptor_ssbo(0, sizeof(ssbo_object_picking), 2, *shader, 2);
			
			renderer_set_descriptor_sampler(state_ptr->batch_textures, 3, *shader);// TODO: Reuse the already existing textures from the world view, to avoid to do the internal for loop of this function


			renderer_apply_descriptors(*shader);
			renderer_draw_geometry(number_of_instances, *geometry_system_get_quad());

		}

		if (!renderer_renderpass_end()) {
			CE_LOG_ERROR("world_render_view_on_render failed renderpass end. Application shutting down");
			return false;
		}

		ssbo_object_picking data;
		data.id = -1;
		renderer_get_descriptor_ssbo(&data.id, sizeof(uint), 2, *shader, 2);
		CE_LOG_INFO("%d", data.id);


		return true;
	}
}