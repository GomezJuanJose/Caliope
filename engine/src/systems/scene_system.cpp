#include "scene_system.h"
#include "core/logger.h"
#include "core/cememory.h"
#include "loaders/resources_types.inl"
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
		// TODO: Unload all scenes and free resources

		state_ptr->loaded_scenes.empty();
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
		
	/*	resource r;
		if (!resource_system_load(name, RESOURCE_TYPE_SCENE, r)) {
			CE_LOG_ERROR("scene_system_load couldnt load scene");
			return false;
		}
		scene_resource_data* scene_config = &std::any_cast<scene_resource_data>(r.data);
		scene scene;
		scene.name = scene_config->name;
		scene.is_enabled = enable_by_default;
		
		scene_system_create_empty(std::string(scene_config->name.data()), true);



		// Builds each entity stored in the scene, each entity data is stored in 3 differents vectors
		// one for it's archetype, another for a vector wich contains each component data, and one vector
		// with an array of component ids to know to wich component the data belongs to.
		for (uint entity_index = 0; entity_index < scene_config->entities_count; ++entity_index) {
			
			std::vector<void*> component_data;
			std::vector<component_id> components;
				uint64 size;
				// TODO: Refactor to a better parser!!!!
				if (scene_config->entities_archetype[entity_index] == ARCHETYPE_SPRITE || scene_config->entities_archetype[entity_index] == ARCHETYPE_SPRITE_ANIMATION || scene_config->entities_archetype[entity_index] == ARCHETYPE_POINT_LIGHT) {
					component_data.push_back(&scene_config->entities_transform[entity_index]);
					components.push_back(TRANSFORM_COMPONENT);
				}

				if (scene_config->entities_archetype[entity_index] == ARCHETYPE_SPRITE ) {
					component_data.push_back(&scene_config->entities_material[entity_index]);
					components.push_back(MATERIAL_COMPONENT);
				}

				if (scene_config->entities_archetype[entity_index] == ARCHETYPE_SPRITE_ANIMATION) {
					component_data.push_back(&scene_config->entities_animation[entity_index]);
					components.push_back(MATERIAL_ANIMATION_COMPONENT);
				}

				if ( scene_config->entities_archetype[entity_index] == ARCHETYPE_POINT_LIGHT) {
					component_data.push_back(&scene_config->entities_light[entity_index]);
					components.push_back(POINT_LIGHT_COMPONENT);
				}

			scene_system_instance_entity(std::string(scene_config->name.data()), scene_config->entities_archetype[entity_index], components, component_data);
		}

		resource_system_unload(r);*/
		
		return true;
	}

	void scene_system_unload(std::string& name) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_unload scene not found");
			return;
		}
		
		for (uint entity : state_ptr->loaded_scenes.at(name).entities) {
			scene_system_destroy_entity(name, entity);
		}

		state_ptr->loaded_scenes.erase(name);
	}

	bool scene_system_save(std::string& name) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_save scene not found");
			return false;
		}
		
	/*
		uint file_size = sizeof(scene_resource_data);
		scene_resource_data* scn_data = (scene_resource_data*)allocate_memory(MEMORY_TAG_UNKNOWN, file_size);
		zero_memory(scn_data, file_size);
		strcpy(scn_data->name.data(), name.data()); // TODO: String library


		// Iterates throught each entity of each archetype and gets its archetype, component_id and component data
		for (uint entity : state_ptr->loaded_scenes.at(name).entities) {
			archetype archtype = ecs_system_get_entity_archetype(entity);
			scn_data->entities_archetype[scn_data->entities_count] = archtype;

			
			std::vector<component_id>& components = ecs_system_get_entity_components(entity);
			for (uint i = 0; i < components.size(); ++i) {
				uint64 component_size;
				// TODO: Refactor to a better parser!!!!
				switch (components[i]) {
				case TRANSFORM_COMPONENT:
					scn_data->entities_transform[entity] = *(transform_component*)ecs_system_get_component_data(entity, components[i], component_size);
					break;
				case MATERIAL_COMPONENT:
					scn_data->entities_material[entity] = *(material_component*)ecs_system_get_component_data(entity, components[i], component_size);
					break;
				case MATERIAL_ANIMATION_COMPONENT:
					scn_data->entities_animation[entity] = *(material_animation_component*)ecs_system_get_component_data(entity, components[i], component_size);
					break;
				case POINT_LIGHT_COMPONENT:
					scn_data->entities_light[entity] = *(point_light_component*)ecs_system_get_component_data(entity, components[i], component_size);
					break;
				default:
					break;
				}
			}
			scn_data->entities_count += 1;
		}


		file_handle writer;
		file_system_open(std::string("assets\\scenes\\" + name + ".cescene"), FILE_MODE_WRITE, writer);
		file_system_write_bytes(writer, file_size, scn_data);
		file_system_close(writer);
		
		free_memory(MEMORY_TAG_UNKNOWN, scn_data, file_size);*/

		return true;
	}

	bool scene_system_instance_entity(std::string& name, archetype archetype, std::vector<component_id>& components, std::vector<void*>& components_data) {
		uint entity = ecs_system_add_entity(archetype);
		for (uint i = 0; i < components.size(); ++i) {
			ecs_system_insert_data(entity, components[i], components_data[i]);
		}
		
		state_ptr->entity_index_scene.insert({ entity, state_ptr->loaded_scenes.at(name).entities.size() });
		state_ptr->loaded_scenes.at(name).entities.push_back(entity);

		return true;
	}

	void scene_system_destroy_entity(std::string& name, uint entity) {

		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end() || state_ptr->entity_index_scene.find(entity) == state_ptr->entity_index_scene.end()) {
			return;
		}


		uint entity_scene_index = state_ptr->entity_index_scene.at(entity);
		state_ptr->loaded_scenes.at(name).entities.erase(state_ptr->loaded_scenes.at(name).entities.begin() + entity_scene_index);
		ecs_system_delete_entity(entity);
		
		uint last_entity = state_ptr->loaded_scenes.at(name).entities.back();
		state_ptr->loaded_scenes.at(name).entities.insert(state_ptr->loaded_scenes.at(name).entities.begin() + entity_scene_index, last_entity);
		state_ptr->loaded_scenes.at(name).entities.pop_back();
		state_ptr->entity_index_scene[last_entity] = entity_scene_index;
		state_ptr->entity_index_scene.erase(entity);
	}

	void scene_system_enable(std::string& name, bool enable) {
		if (state_ptr->loaded_scenes.find(name) == state_ptr->loaded_scenes.end()) {
			CE_LOG_WARNING("scene_system_enable scene not found");
			return;
		}

		state_ptr->loaded_scenes.at(name).is_enabled = enable;
		for (uint entity_index = 0; entity_index < state_ptr->loaded_scenes.at(name).entities.size(); ++entity_index) {
			ecs_system_enable_entity(state_ptr->loaded_scenes.at(name).entities[entity_index], enable);
		}
	}

	void scene_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* world_cam_in_use, float delta_time) {
			
		std::vector<quad_definition> quads_data;
		std::vector<point_light_definition> lights_data;

		// Gets all sprites entities
		std::vector<uint>& sprites = ecs_system_get_entities_by_archetype(ARCHETYPE_SPRITE);
		for (uint entity_index = 0; entity_index < sprites.size(); ++entity_index) {

			uint64 size;
			quad_definition quad_definition;
			quad_definition.id = sprites[entity_index];

			transform_component* tran_comp = (transform_component*)ecs_system_get_component_data(sprites[entity_index], TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, tran_comp->scale);
			transform_set_position(transform, tran_comp->position);
			quad_definition.transform = transform;

			material_component* sprite_comp = (material_component*)ecs_system_get_component_data(sprites[entity_index], MATERIAL_COMPONENT, size);
			quad_definition.material_name = std::string(sprite_comp->material_name.data()); // TODO: Change to char array
			quad_definition.z_order = sprite_comp->z_order;
			quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
				*material_system_adquire(std::string(sprite_comp->material_name.data()))->diffuse_texture,
				sprite_comp->texture_region[0],
				sprite_comp->texture_region[1]
			);

			quads_data.push_back(quad_definition);
		}
			
		// Gets all animations sprites entities
		std::vector<uint>& sprites_animation = ecs_system_get_entities_by_archetype(ARCHETYPE_SPRITE_ANIMATION);
		for (uint entity_index = 0; entity_index < sprites_animation.size(); ++entity_index) {

			uint64 size;
			quad_definition quad_definition;
			quad_definition.id = sprites_animation[entity_index];

			transform_component* tran_comp = (transform_component*)ecs_system_get_component_data(sprites_animation[entity_index], TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, tran_comp->scale);
			transform_set_position(transform, tran_comp->position);
			quad_definition.transform = transform;	

			material_animation_component* anim_comp = (material_animation_component*)ecs_system_get_component_data(sprites_animation[entity_index], MATERIAL_ANIMATION_COMPONENT, size);
			sprite_frame& frame = sprite_animation_system_acquire_frame(std::string(anim_comp->animation_name.data()), delta_time);
			quad_definition.material_name = frame.material_name;
			quad_definition.z_order = anim_comp->z_order;
			quad_definition.texture_region = frame.texture_region;

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
		pick_object_packet.view_type = VIEW_TYPE_OBJECT_PICK;
		render_view_system_on_build_packet(VIEW_TYPE_OBJECT_PICK, pick_object_packet, std::vector<std::any>({ quads_data, world_cam_in_use, delta_time}));
		packets.push_back(pick_object_packet);
	}
}