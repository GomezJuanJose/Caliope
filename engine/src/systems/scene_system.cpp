#include "scene_system.h"
#include "core/logger.h"
#include "core/cememory.h"
#include "resources/resources_types.inl"
#include "platform/file_system.h"

#include "components/components.inl"
#include "ecs_system.h"

#include "sprite_animation_system.h"
#include "material_system.h"
#include "texture_system.h"
#include "resource_system.h"

#include "renderer/renderer_types.inl"
#include "systems/render_view_system.h"

namespace caliope {
	typedef struct scene_system_state {
		std::unordered_map<std::string, scene> loaded_scenes;
		std::unordered_map<uint, uint> entity_index_scene; // Index of the entity that occupies in the scene
		uint scene_count;
		uint max_number_entities;
	}scene_system_state;

	static std::unique_ptr<scene_system_state> state_ptr;


	bool scene_system_initialize(scene_system_configuration& config) {
		state_ptr = std::make_unique<scene_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->max_number_entities = config.max_number_entities;

		CE_LOG_INFO("Scene system initialized.");

		return true;
	}

	void scene_system_shutdown() {
		for (auto [scene_name, scene] : state_ptr->loaded_scenes) {
			scene_system_unload(std::string(scene_name.c_str()));
		}

		state_ptr->loaded_scenes.clear();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	bool scene_system_create_empty(std::string& name, bool enable_by_default) {
		scene scene;
		strcpy(scene.name.data(), name.data());
		scene.is_enabled = enable_by_default;

		state_ptr->loaded_scenes.insert({ name, scene });

		return true;
	}

	bool scene_system_load(std::string& name, bool enable_by_default) {

		if (state_ptr->loaded_scenes.find(name) != state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_load scene already loaded or created a new one with that name, try to unload it first and load again");
			return true;
		}

		resource r;
		if (!resource_system_load(name, RESOURCE_TYPE_SCENE, r)) {
			CE_LOG_ERROR("scene_system_load couldnt load file scene %s", name.c_str());
			return false;
		}
		scene_resource_data scene_config = std::any_cast<scene_resource_data>(r.data);


		scene_system_create_empty(std::string(scene_config.name.data()), enable_by_default);


		for (uint entity_index = 0; entity_index < scene_config.archetypes.size(); ++entity_index) {

			// Tries to build the archetype if not exists
			std::vector<uint> new_archetype_size;
			std::vector<component_id> new_archetype_id;
			std::vector<std::vector<component_data_type>> sprite_components_data_types;

			for (uint component_index = 0; component_index < scene_config.components[entity_index].size(); ++component_index) {
				component_id comp_id = scene_config.components[entity_index][component_index];
				new_archetype_id.push_back(comp_id);
				new_archetype_size.push_back(scene_config.components_sizes.at(comp_id));

				sprite_components_data_types.push_back(std::vector<component_data_type>());
				for (uint component_data_types_index = 0; component_data_types_index < scene_config.components_data_types[entity_index][component_index].size(); ++component_data_types_index) {
					sprite_components_data_types[component_index].push_back(scene_config.components_data_types[entity_index][component_index][component_data_types_index]);
				}
			}
			ecs_system_build_archetype(scene_config.archetypes[entity_index], new_archetype_id, new_archetype_size, sprite_components_data_types);


			// Creates a new entity
			std::vector<component_id> components;
			std::vector<void*> data;

			for (uint component_index = 0; component_index < scene_config.components[entity_index].size(); ++component_index) {
				components.push_back(scene_config.components[entity_index][component_index]);
				data.push_back(scene_config.components_data[entity_index][component_index]);
			}

			scene_system_instance_entity(std::string(scene_config.name.data()), scene_config.archetypes[entity_index],
				components,
				data
			);
		}

		resource_system_unload(r);

		return true;
	}

	void scene_system_unload(std::string& name) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_unload scene %s not found", name.c_str());
			return;
		}
		
		while (!state_ptr->loaded_scenes.at(name).entities.empty()) {
			scene_system_destroy_entity(name, state_ptr->loaded_scenes.at(name).entities.front());
		}

		state_ptr->loaded_scenes.erase(name);
	}

	bool scene_system_save(std::string& name) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_save scene not found");
			return false;
		}

		if (!resource_system_parse(name, RESOURCE_TYPE_SCENE, &state_ptr->loaded_scenes.at(name))) {
			CE_LOG_ERROR("scene_system_save couldnt save file scene %s", name.c_str());
			return false;
		}

		return true;
	}

	bool scene_system_instance_entity(std::string& name, archetype archetype, std::vector<component_id>& components, std::vector<void*>& components_data) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_instance_entity scene %s not found", name.c_str());
			return -1;
		}

		uint entity = ecs_system_add_entity(archetype);
		for (uint i = 0; i < components.size(); ++i) {
			ecs_system_insert_data(entity, components[i], components_data[i]);
		}
		
		state_ptr->entity_index_scene.insert({ entity, state_ptr->loaded_scenes.at(name).entities.size() });
		state_ptr->loaded_scenes.at(name).entities.push_back(entity);

		return entity;
	}

	void scene_system_destroy_entity(std::string& name, uint entity) {

		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end() || state_ptr->entity_index_scene.find(entity) == state_ptr->entity_index_scene.end()) {
			CE_LOG_WARNING("scene_system_enable scene %s not found", name.c_str());
			return;
		}

		bool remove_last_entity = state_ptr->loaded_scenes.at(name).entities.back() == entity;

		uint entity_scene_index = state_ptr->entity_index_scene.at(entity);
		ecs_system_delete_entity(entity);


		if (state_ptr->loaded_scenes.at(name).entities.size() > 1 && !remove_last_entity) {
			state_ptr->loaded_scenes.at(name).entities.erase(state_ptr->loaded_scenes.at(name).entities.begin() + entity_scene_index);

			uint last_entity = state_ptr->loaded_scenes.at(name).entities.back();
			state_ptr->loaded_scenes.at(name).entities.pop_back();
			state_ptr->loaded_scenes.at(name).entities.insert(state_ptr->loaded_scenes.at(name).entities.begin() + entity_scene_index, last_entity);
			state_ptr->entity_index_scene[last_entity] = entity_scene_index;
		}
		else if (remove_last_entity) {
			uint last_index = state_ptr->loaded_scenes.at(name).entities.size() - 1;
			state_ptr->loaded_scenes.at(name).entities.erase(state_ptr->loaded_scenes.at(name).entities.begin() + last_index);
		}
		
		state_ptr->entity_index_scene.erase(entity);

	}

	void scene_system_enable(std::string& name, bool enable) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_enable scene %s not found", name.c_str());
			return;
		}

		state_ptr->loaded_scenes.at(name).is_enabled = enable;
		for (uint entity_index = 0; entity_index < state_ptr->loaded_scenes.at(name).entities.size(); ++entity_index) {
			ecs_system_enable_entity(state_ptr->loaded_scenes.at(name).entities[entity_index], enable);
		}
	}

	void scene_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* world_cam_in_use, float delta_time) {
			
		std::vector<quad_instance_definition> quads_data;
		std::vector<point_light_definition> lights_data;

		// Gets all sprites entities
		std::vector<uint>& sprites = ecs_system_get_entities_by_archetype(ARCHETYPE_SPRITE);
		for (uint entity_index = 0; entity_index < sprites.size(); ++entity_index) {

			uint64 size;
			quad_instance_definition quad_definition;
			quad_definition.id = sprites[entity_index];

			transform_component* tran_comp = (transform_component*)ecs_system_get_component_data(sprites[entity_index], TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, tran_comp->scale);
			transform_set_position(transform, tran_comp->position);
			quad_definition.transform = transform;

			material_component* sprite_comp = (material_component*)ecs_system_get_component_data(sprites[entity_index], MATERIAL_COMPONENT, size);
			material* mat = material_system_adquire(std::string(sprite_comp->material_name.data()));
			quad_definition.diffuse_color = mat->diffuse_color;
			quad_definition.shininess_intensity = mat->shininess_intensity;
			quad_definition.shininess_sharpness = mat->shininess_sharpness;
			quad_definition.shader = mat->shader;
			quad_definition.diffuse_texture = mat->diffuse_texture;
			quad_definition.specular_texture = mat->specular_texture;
			quad_definition.normal_texture = mat->normal_texture;

			quad_definition.z_order = sprite_comp->z_order;
			quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
				*material_system_adquire(std::string(sprite_comp->material_name.data()))->diffuse_texture,
				sprite_comp->texture_region[0],
				sprite_comp->texture_region[1],
				false
			);

			quads_data.push_back(quad_definition);
		}
			
		// Gets all animations sprites entities
		std::vector<uint>& sprites_animation = ecs_system_get_entities_by_archetype(ARCHETYPE_SPRITE_ANIMATION);
		for (uint entity_index = 0; entity_index < sprites_animation.size(); ++entity_index) {

			uint64 size;
			quad_instance_definition quad_definition;
			quad_definition.id = sprites_animation[entity_index];

			material_animation_component* anim_comp = (material_animation_component*)ecs_system_get_component_data(sprites_animation[entity_index], MATERIAL_ANIMATION_COMPONENT, size);
			sprite_frame* frame = sprite_animation_system_acquire_frame(std::string(anim_comp->animation_name.data()), delta_time);
			if (frame == nullptr) {
				continue;
			}

			material* mat = material_system_adquire(frame->material_name);
			quad_definition.diffuse_color = mat->diffuse_color;
			quad_definition.shininess_intensity = mat->shininess_intensity;
			quad_definition.shininess_sharpness = mat->shininess_sharpness;
			quad_definition.shader = mat->shader;
			quad_definition.diffuse_texture = mat->diffuse_texture;
			quad_definition.specular_texture = mat->specular_texture;
			quad_definition.normal_texture = mat->normal_texture;

			quad_definition.z_order = anim_comp->z_order;
			quad_definition.texture_region = frame->texture_region;

			transform_component* tran_comp = (transform_component*)ecs_system_get_component_data(sprites_animation[entity_index], TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, tran_comp->scale);
			transform_set_position(transform, tran_comp->position);
			quad_definition.transform = transform;	


			quads_data.push_back(quad_definition);
		}
			

		// Gets all point lighst entities
		std::vector<uint>& point_lights = ecs_system_get_entities_by_archetype(ARCHETYPE_POINT_LIGHT);
		for (uint entity_index = 0; entity_index < point_lights.size(); ++entity_index) {
			uint64 size;

			point_light_definition definition;

			transform_component* tran_comp = (transform_component*)ecs_system_get_component_data(point_lights[entity_index], TRANSFORM_COMPONENT, size);
			definition.position = glm::vec4(tran_comp->position, 1.0f);

			point_light_component* light_comp = (point_light_component*)ecs_system_get_component_data(point_lights[entity_index], POINT_LIGHT_COMPONENT, size);
			definition.color = light_comp->color;
			definition.constant = light_comp->constant;
			definition.linear = light_comp->linear;
			definition.quadratic = light_comp->quadratic;
			definition.radius = light_comp->radius;

			lights_data.push_back(definition);
		}


		renderer_view_packet world_packet;
		world_packet.view_type = VIEW_TYPE_WORLD;
		render_view_system_on_build_packet(VIEW_TYPE_WORLD, world_packet, std::vector<std::any>({ quads_data, lights_data, world_cam_in_use, delta_time }));
		packets.push_back(world_packet);


		renderer_view_packet pick_object_packet;
		pick_object_packet.view_type = VIEW_TYPE_WORLD_OBJECT_PICK;
		render_view_system_on_build_packet(VIEW_TYPE_WORLD_OBJECT_PICK, pick_object_packet, std::vector<std::any>({ quads_data, world_cam_in_use, delta_time }));
		packets.push_back(pick_object_packet);
	}
}