#include "ui_system.h"
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
	typedef struct ui_system_state {
		std::unordered_map<std::string, scene> loaded_ui_layouts;
		std::unordered_map<uint, uint> entity_index_layout; // Index of the entity that occupies in the layout
		uint layout_count;
		uint max_number_entities;
	} ui_system_state;

	static std::unique_ptr<ui_system_state> state_ptr;


	bool ui_system_initialize(ui_system_configuration& config) {
		state_ptr = std::make_unique<ui_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->max_number_entities = config.max_number_entities;

		CE_LOG_INFO("UI system initialized.");

		return true;
	}

	void ui_system_shutdown() {
		for (auto [scene_name, scene] : state_ptr->loaded_ui_layouts) {
			//scene_system_unload(std::string(scene_name.c_str()));
		}

		state_ptr->loaded_ui_layouts.clear();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	bool ui_system_create_empty_layout(std::string& name, bool enable_by_default) {
		scene scene;
		strcpy(scene.name.data(), name.data());
		scene.is_enabled = enable_by_default;

		state_ptr->loaded_ui_layouts.insert({ name, scene });

		return true;
	}

	/*bool ui_system_load_layout(std::string& name, bool enable_by_default) {

		if (state_ptr->loaded_ui_layouts.find(name) != state_ptr->loaded_ui_layouts.end()) {
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
	}*/

/*	void scene_system_unload(std::string& name) {
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("scene_system_unload scene %s not found", name.c_str());
			return;
		}
		
		while (!state_ptr->loaded_ui_layouts.at(name).entities.empty()) {
			scene_system_destroy_entity(name, state_ptr->loaded_ui_layouts.at(name).entities.front());
		}

		state_ptr->loaded_ui_layouts.erase(name);
	}

	bool scene_system_save(std::string& name) {
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("scene_system_save scene not found");
			return false;
		}

		if (!resource_system_parse(name, RESOURCE_TYPE_SCENE, &state_ptr->loaded_ui_layouts.at(name))) {
			CE_LOG_ERROR("scene_system_save couldnt save file scene %s", name.c_str());
			return false;
		}

		return true;
	}
*/
	bool ui_system_instance_image(std::string& name, transform_component& transform, ui_material_component& ui_material) {
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("ui_system_instance_image scene %s not found", name.c_str());
			return false;
		}

		std::vector<component_id> components = { TRANSFORM_COMPONENT, UI_MATERIAL_COMPONENT };
		std::vector<void*> data = { &transform , &ui_material };

		uint entity = ecs_system_add_entity(ARCHETYPE_UI_IMAGE);
		for (uint i = 0; i < components.size(); ++i) {
			ecs_system_insert_data(entity, components[i], data[i]);
		}

		
		state_ptr->entity_index_layout.insert({ entity, state_ptr->loaded_ui_layouts.at(name).entities.size() });
		state_ptr->loaded_ui_layouts.at(name).entities.push_back(entity);

		return true;
	}

	void ui_system_destroy_entity(std::string& name, uint entity) {

		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end() || state_ptr->entity_index_layout.find(entity) == state_ptr->entity_index_layout.end()) {
			CE_LOG_WARNING("ui_system_destroy_entity scene %s not found", name.c_str());
			return;
		}

		bool remove_last_entity = state_ptr->loaded_ui_layouts.at(name).entities.back() == entity;

		uint entity_scene_index = state_ptr->entity_index_layout.at(entity);
		ecs_system_delete_entity(entity);


		if (state_ptr->loaded_ui_layouts.at(name).entities.size() > 1 && !remove_last_entity) {
			state_ptr->loaded_ui_layouts.at(name).entities.erase(state_ptr->loaded_ui_layouts.at(name).entities.begin() + entity_scene_index);

			uint last_entity = state_ptr->loaded_ui_layouts.at(name).entities.back();
			state_ptr->loaded_ui_layouts.at(name).entities.pop_back();
			state_ptr->loaded_ui_layouts.at(name).entities.insert(state_ptr->loaded_ui_layouts.at(name).entities.begin() + entity_scene_index, last_entity);
			state_ptr->entity_index_layout[last_entity] = entity_scene_index;
		}
		else if (remove_last_entity) {
			uint last_index = state_ptr->loaded_ui_layouts.at(name).entities.size() - 1;
			state_ptr->loaded_ui_layouts.at(name).entities.erase(state_ptr->loaded_ui_layouts.at(name).entities.begin() + last_index);
		}
		
		state_ptr->entity_index_layout.erase(entity);

	}

	void ui_system_enable_layout(std::string& name, bool enable) {
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("ui_system_enable_layout scene %s not found", name.c_str());
			return;
		}

		state_ptr->loaded_ui_layouts.at(name).is_enabled = enable;
		for (uint entity_index = 0; entity_index < state_ptr->loaded_ui_layouts.at(name).entities.size(); ++entity_index) {
			ecs_system_enable_entity(state_ptr->loaded_ui_layouts.at(name).entities[entity_index], enable);
		}
	}

	void ui_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* ui_cam_in_use, float delta_time) {
			
		std::vector<quad_definition> quads_data;

		// Gets all sprites entities
		std::vector<uint>& ui_images = ecs_system_get_entities_by_archetype(ARCHETYPE_UI_IMAGE);
		for (uint entity_index = 0; entity_index < ui_images.size(); ++entity_index) {

			uint64 size;
			quad_definition quad_definition;
			quad_definition.id = ui_images[entity_index];

			transform_component* tran_comp = (transform_component*)ecs_system_get_component_data(ui_images[entity_index], TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, tran_comp->scale);
			transform_set_position(transform, tran_comp->position);
			quad_definition.transform = transform;

			ui_material_component* ui_image_comp = (ui_material_component*)ecs_system_get_component_data(ui_images[entity_index], UI_MATERIAL_COMPONENT, size);
			quad_definition.material_name = std::string(ui_image_comp->material_name.data()); // TODO: Change to char array
			quad_definition.z_order = 9999999;
			quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
				*material_system_adquire(std::string(ui_image_comp->material_name.data()))->diffuse_texture,
				ui_image_comp->texture_region[0],
				ui_image_comp->texture_region[1]
			);

			quads_data.push_back(quad_definition);
		}

		renderer_view_packet ui_packet;
		ui_packet.view_type = VIEW_TYPE_UI;
		render_view_system_on_build_packet(VIEW_TYPE_UI, ui_packet, std::vector<std::any>({ quads_data, ui_cam_in_use, delta_time }));
		packets.push_back(ui_packet);

		renderer_view_packet pick_object_packet;
		pick_object_packet.view_type = VIEW_TYPE_UI_OBJECT_PICK;
		render_view_system_on_build_packet(VIEW_TYPE_UI_OBJECT_PICK, pick_object_packet, std::vector<std::any>({ quads_data, ui_cam_in_use, delta_time }));
		packets.push_back(pick_object_packet);
	}
}