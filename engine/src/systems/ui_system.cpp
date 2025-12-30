#include "ui_system.h"
#include "core/logger.h"
#include "core/cememory.h"
#include "core/cestring.h"
#include "core/event.h"
#include "resources/resources_types.inl"
#include "platform/file_system.h"
#include "platform/platform.h"

#include "components/components.inl"
#include "ecs_system.h"
#include "object_pick_system.h"

#include "sprite_animation_system.h"
#include "material_system.h"
#include "texture_system.h"
#include "resource_system.h"
#include "text_font_system.h"
#include "text_style_system.h"

#include "renderer/renderer_types.inl"
#include "render_view_system.h"


namespace caliope {
	typedef struct ui_system_state {
		std::unordered_map<std::string, scene> loaded_ui_layouts;
		std::unordered_map<uint, uint> entity_index_layout; // Index of the entity that occupies in the layout
		uint layout_count;
		uint max_number_entities;

		uint16 window_width;
		uint16 window_height;
		float aspect_ratio;

		uint previous_hover_entity;
	} ui_system_state;

	static std::unique_ptr<ui_system_state> state_ptr;

	// NOTE: Data is the key current entity pressed
	bool on_ui_pressed(event_system_code code, std::any data) {
		uint hover_entity = std::any_cast<uint>(data);
		uint64 size;

		ui_dynamic_material_component* ui_dynamic_image_comp = (ui_dynamic_material_component*)ecs_system_get_component_data(hover_entity, UI_DYNAMIC_MATERIAL_COMPONENT, size);
		ui_events_component* ui_events_comp = (ui_events_component*)ecs_system_get_component_data(hover_entity, UI_MOUSE_EVENTS_COMPONENT, size);
		if (ui_dynamic_image_comp != nullptr) {
			material_system_adquire(std::string(&ui_dynamic_image_comp->material_name[0]))->diffuse_color = ui_dynamic_image_comp->pressed_color;
			material_system_adquire(std::string(&ui_dynamic_image_comp->material_name[0]))->diffuse_texture = texture_system_adquire(std::string(&ui_dynamic_image_comp->pressed_texture[0]));
		}
		if (ui_events_comp != nullptr) {
			ui_events_comp->on_ui_pressed(EVENT_CODE_ON_UI_BUTTON_PRESSED, 0);
		}

		return false;
	}

	// NOTE: Data is the key current entity released
	bool on_ui_released(event_system_code code, std::any data) {
		uint hover_entity = std::any_cast<uint>(data);
		uint64 size;

		ui_dynamic_material_component* ui_dynamic_image_comp = (ui_dynamic_material_component*)ecs_system_get_component_data(hover_entity, UI_DYNAMIC_MATERIAL_COMPONENT, size);
		ui_events_component* ui_events_comp = (ui_events_component*)ecs_system_get_component_data(hover_entity, UI_MOUSE_EVENTS_COMPONENT, size);
		if (ui_dynamic_image_comp != nullptr) {
			material_system_adquire(std::string(&ui_dynamic_image_comp->material_name[0]))->diffuse_color = ui_dynamic_image_comp->normal_color;
			material_system_adquire(std::string(&ui_dynamic_image_comp->material_name[0]))->diffuse_texture = texture_system_adquire(std::string(&ui_dynamic_image_comp->normal_texture[0]));
		}
		if (ui_events_comp != nullptr) {
			ui_events_comp->on_ui_released(EVENT_CODE_ON_UI_BUTTON_RELEASED, 0);
		}

		return false;
	}

	// NOTE: Data is the hover entity and if is world type or not
	bool on_ui_hover(event_system_code code, std::any data) {
		uint hover_entity = std::any_cast<uint>(data);
		uint previous_hover_entity = state_ptr->previous_hover_entity;
		uint64 size;

		ui_dynamic_material_component* ui_dynamic_image_comp = (ui_dynamic_material_component*)ecs_system_get_component_data(hover_entity, UI_DYNAMIC_MATERIAL_COMPONENT, size);
		ui_events_component* ui_events_comp = (ui_events_component*)ecs_system_get_component_data(previous_hover_entity, UI_MOUSE_EVENTS_COMPONENT, size);
		if (ui_dynamic_image_comp != nullptr) {
			material_system_adquire(std::string(&ui_dynamic_image_comp->material_name[0]))->diffuse_color = ui_dynamic_image_comp->hover_color;
			material_system_adquire(std::string(&ui_dynamic_image_comp->material_name[0]))->diffuse_texture = texture_system_adquire(std::string(&ui_dynamic_image_comp->hover_texture[0]));
		}
		if (ui_events_comp != nullptr) {
			ui_events_comp->on_ui_hover(EVENT_CODE_ON_UI_BUTTON_HOVER, 0);
		}

		if (hover_entity == previous_hover_entity) {
			return false;
		}

		ui_dynamic_material_component* previous_ui_dynamic_image_comp = (ui_dynamic_material_component*)ecs_system_get_component_data(previous_hover_entity, UI_DYNAMIC_MATERIAL_COMPONENT, size);
		ui_events_component* previous_ui_events_comp = (ui_events_component*)ecs_system_get_component_data(previous_hover_entity, UI_MOUSE_EVENTS_COMPONENT, size);
		if (previous_ui_dynamic_image_comp != nullptr) {
			material_system_adquire(std::string(&previous_ui_dynamic_image_comp->material_name[0]))->diffuse_color = previous_ui_dynamic_image_comp->normal_color;
			material_system_adquire(std::string(&previous_ui_dynamic_image_comp->material_name[0]))->diffuse_texture = texture_system_adquire(std::string(&previous_ui_dynamic_image_comp->normal_texture[0]));
		}
		if (previous_ui_events_comp != nullptr) {
			previous_ui_events_comp->on_ui_unhover(EVENT_CODE_ON_UI_BUTTON_UNHOVER, 0);
		}

		state_ptr->previous_hover_entity = hover_entity;

		return false;
	}

	bool ui_system_initialize(ui_system_configuration& config) {
		state_ptr = std::make_unique<ui_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->max_number_entities = config.max_number_entities;
		state_ptr->window_width = config.initial_window_width;
		state_ptr->window_height = config.initial_window_height;
		state_ptr->aspect_ratio = (float)config.initial_window_width / (float)config.initial_window_height;


		event_register(EVENT_CODE_ON_ENTITY_PRESSED, on_ui_pressed);
		event_register(EVENT_CODE_ON_ENTITY_RELEASED, on_ui_released);
		event_register(EVENT_CODE_ON_ENTITY_HOVER, on_ui_hover);

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

	void ui_system_on_resize(uint16 width, uint16 height)
	{
		state_ptr->window_width = width;
		state_ptr->window_height = height;
		state_ptr->aspect_ratio = (float)width / (float)height;
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
	uint ui_system_instance_image(std::string& name, ui_transform_component& transform, ui_material_component& ui_material, ui_behaviour_component& cursor_behaviour) {
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("ui_system_instance_image scene %s not found", name.c_str());
			return -1;
		}

		parent_component parent_comp;
		parent_comp.parent = -1;


		std::vector<component_id> components = { PARENT_COMPONENT, UI_TRANSFORM_COMPONENT, UI_MATERIAL_COMPONENT, UI_BEHAVIOUR_COMPONENT };
		std::vector<void*> data = { &parent_comp, &transform , &ui_material, &cursor_behaviour };

		uint entity = ecs_system_add_entity(ARCHETYPE_UI_IMAGE);
		for (uint i = 0; i < components.size(); ++i) {
			ecs_system_insert_data(entity, components[i], data[i]);
		}

		
		state_ptr->entity_index_layout.insert({ entity, state_ptr->loaded_ui_layouts.at(name).entities.size() });
		state_ptr->loaded_ui_layouts.at(name).entities.push_back(entity);

		return entity;
	}

	uint ui_system_instance_button(std::string& name, ui_transform_component& transform, ui_dynamic_material_component& ui_dynamic_material, ui_events_component& ui_mouse_events, ui_behaviour_component& cursor_behaviour)
	{
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("ui_system_instance_button scene %s not found", name.c_str());
			return -1;
		}

		parent_component parent_comp;
		parent_comp.parent = -1;

		std::vector<component_id> components = { PARENT_COMPONENT, UI_TRANSFORM_COMPONENT, UI_DYNAMIC_MATERIAL_COMPONENT, UI_MOUSE_EVENTS_COMPONENT, UI_BEHAVIOUR_COMPONENT };
		std::vector<void*> data = { &parent_comp, &transform , &ui_dynamic_material, &ui_mouse_events, &cursor_behaviour };

		uint entity = ecs_system_add_entity(ARCHETYPE_UI_BUTTON);
		for (uint i = 0; i < components.size(); ++i) {
			ecs_system_insert_data(entity, components[i], data[i]);
		}


		state_ptr->entity_index_layout.insert({ entity, state_ptr->loaded_ui_layouts.at(name).entities.size() });
		state_ptr->loaded_ui_layouts.at(name).entities.push_back(entity);

		event_register(EVENT_CODE_ON_UI_BUTTON_PRESSED, ui_mouse_events.on_ui_pressed);
		event_register(EVENT_CODE_ON_UI_BUTTON_RELEASED, ui_mouse_events.on_ui_released);
		event_register(EVENT_CODE_ON_UI_BUTTON_HOVER, ui_mouse_events.on_ui_hover);
		event_register(EVENT_CODE_ON_UI_BUTTON_UNHOVER, ui_mouse_events.on_ui_unhover);

		return entity;
	}

	uint ui_system_instance_text_box(std::string& name, ui_transform_component& transform, ui_text_component& ui_text, ui_behaviour_component& cursor_behaviour)
	{
		if (state_ptr->loaded_ui_layouts.find(name) == state_ptr->loaded_ui_layouts.end()) {
			CE_LOG_WARNING("ui_system_instance_text_box scene %s not found", name.c_str());
			return -1;
		}

		parent_component parent_comp;
		parent_comp.parent = -1;

		std::vector<component_id> components = { PARENT_COMPONENT, UI_TRANSFORM_COMPONENT, UI_TEXT_COMPONENT, UI_BEHAVIOUR_COMPONENT };
		std::vector<void*> data = { &parent_comp, &transform , &ui_text, &cursor_behaviour };

		uint entity = ecs_system_add_entity(ARCHETYPE_UI_TEXT_BOX);
		for (uint i = 0; i < components.size(); ++i) {
			ecs_system_insert_data(entity, components[i], data[i]);
		}


		state_ptr->entity_index_layout.insert({ entity, state_ptr->loaded_ui_layouts.at(name).entities.size() });
		state_ptr->loaded_ui_layouts.at(name).entities.push_back(entity);


		return entity;
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

	bool ui_system_parent_entities(uint child, uint parent)
	{
		uint64 size;
		parent_component* parent_child_comp = (parent_component*)ecs_system_get_component_data(child, UI_TRANSFORM_COMPONENT, size);
		if (parent_child_comp) {
			parent_component new_parent;
			new_parent.parent = parent;
			ecs_system_insert_data(child, PARENT_COMPONENT, &new_parent);
			return true;
		}

		return false;
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

	glm::vec3 calculate_position_based_on_anchor_bounds_and_parent(ui_transform_component* transform, ui_anchor_position anchor, uint parent) {
		uint64 size;
		ui_transform_component* parent_transform = (ui_transform_component*)ecs_system_get_component_data(parent, UI_TRANSFORM_COMPONENT, size);

		glm::vec2 top_left_corner_ui_element = glm::vec2( 
			transform->position.x + (transform->bounds_max_point.x / 2) - (transform->bounds_max_point.x * transform->bounds_offset.x), 
			transform->position.y + (transform->bounds_max_point.y / 2) - (transform->bounds_max_point.y * transform->bounds_offset.y)
		);

		glm::vec2 parent_bounds = { state_ptr->window_width, state_ptr->window_height };
		parent_bounds *= state_ptr->aspect_ratio;

		glm::vec2 anchor_position = glm::vec2(0.0f);
		glm::vec2 parent_position = { 0,0 };

		if (parent_transform) {
			parent_bounds = { parent_transform->bounds_max_point.x, parent_transform->bounds_max_point.y };
			parent_position = calculate_position_based_on_anchor_bounds_and_parent(parent_transform, parent_transform->anchor, -1);

			// This is to revert the calculation of the left_corner, due its not the real position of the quad.
			parent_position.x -= (parent_transform->bounds_max_point.x / 2);
			parent_position.y -= (parent_transform->bounds_max_point.y / 2);
		}


		if (anchor == UI_ANCHOR_CENTER_LEFT || anchor == UI_ANCHOR_CENTER || anchor == UI_ANCHOR_CENTER_RIGHT) {
			anchor_position.y = parent_bounds.y / 2;
		}
		else if (anchor == UI_ANCHOR_BOTTOM_LEFT || anchor == UI_ANCHOR_BOTTOM_CENTER || anchor == UI_ANCHOR_BOTTOM_RIGHT) {
			anchor_position.y = parent_bounds.y;
		}

		if (anchor == UI_ANCHOR_TOP_CENTER || anchor == UI_ANCHOR_CENTER || anchor == UI_ANCHOR_BOTTOM_CENTER) {
			anchor_position.x = parent_bounds.x / 2;
		}
		else if (anchor == UI_ANCHOR_TOP_RIGHT || anchor == UI_ANCHOR_CENTER_RIGHT || anchor == UI_ANCHOR_BOTTOM_RIGHT) {
			anchor_position.x = parent_bounds.x;
		}

		return {
			top_left_corner_ui_element.x + anchor_position.x + parent_position.x,
			top_left_corner_ui_element.y + anchor_position.y + parent_position.y,
			transform->position.z
		};
	}

	glm::vec3 calculate_scale_based_on_bounds(ui_transform_component* transform) {
		// NOTE: Here is not applied the aspect ratio because then the sizes do not rescale according to the window size, because applies the aspect ratio which will be applied into the proyection
		// so the final result will end up with the same size in any resolution
		return { transform->bounds_max_point.x, transform->bounds_max_point.y, 0 };
	}

	void populate_package_with_ui_image(std::vector<quad_instance_definition>& quads_data, std::vector<quad_instance_definition>& pick_quads_data) {
		std::vector<uint>& ui_images = ecs_system_get_entities_by_archetype(ARCHETYPE_UI_IMAGE);
		for (uint entity_index = 0; entity_index < ui_images.size(); ++entity_index) {

			uint64 size;
			ui_behaviour_component* behaviour_comp = (ui_behaviour_component*)ecs_system_get_component_data(ui_images[entity_index], UI_BEHAVIOUR_COMPONENT, size);
			if (behaviour_comp->visibility == UI_VISIBILITY_COLLAPSE) {
				// Skip and go for the next one
				continue;
			}
			
			parent_component* parent_comp = (parent_component*)ecs_system_get_component_data(ui_images[entity_index], PARENT_COMPONENT, size);
			

			quad_instance_definition quad_definition;
			quad_definition.id = ui_images[entity_index];

			ui_transform_component* tran_comp = (ui_transform_component*)ecs_system_get_component_data(ui_images[entity_index], UI_TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, calculate_scale_based_on_bounds(tran_comp));
			transform_set_position(transform, calculate_position_based_on_anchor_bounds_and_parent(tran_comp, tran_comp->anchor, parent_comp->parent));
			quad_definition.transform = transform;

			ui_material_component* ui_image_comp = (ui_material_component*)ecs_system_get_component_data(ui_images[entity_index], UI_MATERIAL_COMPONENT, size);

			material* mat = material_system_adquire(std::string(ui_image_comp->material_name.data()));
			quad_definition.diffuse_color = mat->diffuse_color;
			quad_definition.shininess_intensity = mat->shininess_intensity;
			quad_definition.shininess_sharpness = mat->shininess_sharpness;
			quad_definition.shader = mat->shader;
			quad_definition.diffuse_texture = mat->diffuse_texture;
			quad_definition.specular_texture = mat->specular_texture;
			quad_definition.normal_texture = mat->normal_texture;

			quad_definition.z_order = 0;
			quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
				*material_system_adquire(std::string(ui_image_comp->material_name.data()))->diffuse_texture,
				ui_image_comp->texture_region[0],
				ui_image_comp->texture_region[1],
				true
			);

			
			quads_data.push_back(quad_definition);

			quads_data.push_back(quad_definition);
			if (behaviour_comp->visibility == UI_VISIBILITY_VISIBLE) {
				pick_quads_data.push_back(quad_definition);
			}
		}
	}

	void populate_package_with_ui_button(std::vector<quad_instance_definition>& quads_data, std::vector<quad_instance_definition>& pick_quads_data) {
		std::vector<uint> ui_button = ecs_system_get_entities_by_archetype(ARCHETYPE_UI_BUTTON);// TODO: If the vector is a reference and there is no button, calling the function the renderer crash, investigate why!
		for (uint entity_index = 0; entity_index < ui_button.size(); ++entity_index) {

			uint64 size;
			ui_behaviour_component* behaviour_comp = (ui_behaviour_component*)ecs_system_get_component_data(ui_button[entity_index], UI_BEHAVIOUR_COMPONENT, size);
			if (behaviour_comp->visibility == UI_VISIBILITY_COLLAPSE) {
				// Skip and go for the next one
				continue;
			}

			quad_instance_definition quad_definition;
			quad_definition.id = ui_button[entity_index];

			parent_component* parent_comp = (parent_component*)ecs_system_get_component_data(ui_button[entity_index], PARENT_COMPONENT, size);
			
			ui_transform_component* tran_comp = (ui_transform_component*)ecs_system_get_component_data(ui_button[entity_index], UI_TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, calculate_scale_based_on_bounds(tran_comp));
			transform_set_position(transform, calculate_position_based_on_anchor_bounds_and_parent(tran_comp, tran_comp->anchor, parent_comp->parent));
			quad_definition.transform = transform;

			ui_dynamic_material_component* ui_dynamic_image_comp = (ui_dynamic_material_component*)ecs_system_get_component_data(ui_button[entity_index], UI_DYNAMIC_MATERIAL_COMPONENT, size);
			material* mat = material_system_adquire(std::string(ui_dynamic_image_comp->material_name.data())); //TODO: Use a built in material UI?
			quad_definition.diffuse_color = mat->diffuse_color;
			quad_definition.shininess_intensity = mat->shininess_intensity;
			quad_definition.shininess_sharpness = mat->shininess_sharpness;
			quad_definition.shader = mat->shader;
			quad_definition.diffuse_texture = mat->diffuse_texture;
			quad_definition.specular_texture = mat->specular_texture;
			quad_definition.normal_texture = mat->normal_texture;

			quad_definition.z_order = 0;
			quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
				*material_system_adquire(std::string(ui_dynamic_image_comp->material_name.data()))->diffuse_texture,
				ui_dynamic_image_comp->texture_region[0],
				ui_dynamic_image_comp->texture_region[1],
				true
			);

			ui_behaviour_component* cursor_behaviour_comp = (ui_behaviour_component*)ecs_system_get_component_data(ui_button[entity_index], UI_BEHAVIOUR_COMPONENT, size);

			quads_data.push_back(quad_definition);

			if (cursor_behaviour_comp->visibility == UI_VISIBILITY_VISIBLE) {
				pick_quads_data.push_back(quad_definition);
			}
		}
	}


	void get_metrics_applying_style_to_text(text_style_table* style_table, std::string& text,
		std::vector<text_style>& styles, std::vector<uint>& style_starting_indices, std::vector<uint>& style_ending_indices,
		std::vector<text_image_style>& insterted_images, std::vector<uint>& inserted_image_starting_indices, std::vector<uint>& inserted_image_ending_indices,
		uint max_width_line, std::vector<uint>& break_line_indices, std::vector<int>& line_heighs)
	{

		bool start_tag_retrieving = false;
		bool tag_matches = false;
		std::string found_tag_name = "";

		uint found_tag_starting_index = 0;
		uint found_tag_ending_index = 0;

		text_style default_style = text_style_system_adquire_text_style(style_table, std::string("default"));
		text_style* in_use_style = &default_style;
		uint x_advance = 0;
		uint char_to_break_index = 0;
		std::string word_to_recheck = "";
		bool is_new_line = true;

		uint max_line_height = 0;
		int temporal_max_line_height = 0;

		// If there is no end character (" \n")then add one to get the last break line with the correct lineheight
		if (!text.empty() && text.substr(std::clamp<int>(0,2,text.size()-2),2) != " \n") {
			//First remove the breakline char if exists just to insert the correct end text characters
			if (text.back() == '\n') {
				text.pop_back();
			}

			text.push_back(' ');
			text.push_back('\n');
		}

		for (uint char_index = 0; char_index < text.size(); ++char_index) {
			char_to_break_index++;

			// Inserted image retrieving
			if (start_tag_retrieving && text[char_index] == '#') {
				start_tag_retrieving = false;

				for (auto [tag_image, id] : style_table->tag_image_indexes) {
					bool has_close_symbol = text.at(char_index + tag_image.size() + 1) == '}'; // Offset the substraction by one to get the '}' char

					std::string found_tag_image_name = string_substring(&text, char_index + 1, tag_image.size()); // Offset the substraction by one to avoid the '#' and get the last character
					bool image_matches = style_table->tag_image_indexes.find(found_tag_image_name) != style_table->tag_image_indexes.end();

					if (has_close_symbol && image_matches) {
						text_image_style* image = &text_style_system_adquire_text_image_style(style_table, found_tag_image_name);
						insterted_images.push_back(*image);
						if(inserted_image_starting_indices.empty() || inserted_image_starting_indices.back() != char_index - 1)
						inserted_image_starting_indices.push_back(char_index - 1); // Minus 1 due to  previous iteration has the special character '{'
						if (inserted_image_ending_indices.empty() || inserted_image_ending_indices.back() != char_index + tag_image.size() + 1)
						inserted_image_ending_indices.push_back(char_index + tag_image.size() + 1); // Plus 1 to get the character '{'
						char_index += tag_image.size();
						char_to_break_index += tag_image.size();
						x_advance += image->image_size.x;

						if (temporal_max_line_height < image->image_size.y + (in_use_style->font->line_height/2)) {
							temporal_max_line_height = image->image_size.y + (in_use_style->font->line_height/2);
						}

						break;
					}
				}

				continue;
			}

			// Text style retrieving
			if (start_tag_retrieving == true) {
				start_tag_retrieving = false;

				if (tag_matches == false) {
					// Iterates the style table to get each tag and first check if it has a > at the end, 
					// second substring the tag found in the text and check if the name exists in the table
					for (auto [tag, id] : style_table->tag_style_indexes) {
						bool has_close_symbol = char_index + tag.size() < text.size() && text.at(char_index + tag.size()) == '|';

						found_tag_name = string_substring(&text, char_index, tag.size());
						tag_matches = style_table->tag_style_indexes.find(found_tag_name) != style_table->tag_style_indexes.end();

						if (has_close_symbol && tag_matches) {
							found_tag_starting_index = char_index - 1; // Minus 1 due to  previous iteration has the special character '{'

							// Inserts the style right away, this means that the style will be applied even if it not has a close character '{', in that case the style will be applied until the end
							styles.push_back(text_style_system_adquire_text_style(style_table, found_tag_name));
							in_use_style = &styles.back();
							style_starting_indices.push_back(found_tag_starting_index);
							style_ending_indices.push_back(-1);

							char_index += tag.size();
							char_to_break_index += tag.size();
							break;
						}
					}
				}
			}

			// If is the end format tag (}), reset
			if (tag_matches && text[char_index] == '}') {
				found_tag_ending_index = char_index;

				style_ending_indices[style_ending_indices.size()-1] = found_tag_ending_index; // Updates the last index to have a end format

				found_tag_name = "";
				tag_matches = false;
				found_tag_starting_index = 0;
				found_tag_ending_index = 0;
				in_use_style = &default_style;
				continue;
			}

			// If is start format, starts to the search for the tag name
			if (text.at(char_index) == '{') {
				start_tag_retrieving = true;
				continue;
			}

			if (text[char_index] == '\n') {
				if (x_advance > max_width_line) {
					break_line_indices.push_back(char_index - char_to_break_index);
				}
				break_line_indices.push_back(char_index);
				line_heighs.push_back(max_line_height);

				is_new_line = true;
				max_line_height = 0;
				temporal_max_line_height = 0;
				x_advance = 0;
				char_to_break_index = 0;
				word_to_recheck = "";
				continue;
			}

			// Auto wrapping detection by words
			if (text[char_index] == ' ' || text[char_index] == '\t') {

				// If there is a blank space skip to next char
				x_advance += text[char_index] == ' ' ? in_use_style->font->x_advance_space : in_use_style->font->x_advance_tab;

				if (x_advance > max_width_line && !is_new_line) {
					x_advance = 0;
					break_line_indices.push_back(char_index - char_to_break_index);
					line_heighs.push_back(max_line_height);
					max_line_height = 0;
					temporal_max_line_height = 0;

					// This is to avoid cases where there is a long word and breaks into a new line and the same word is long enough to occupie the entirety of the line
					char_index = (char_index - char_to_break_index);
					is_new_line = true;
				}
				else {
					max_line_height = temporal_max_line_height;
					is_new_line = false;
				}
				char_to_break_index = 0;
				word_to_recheck = "";
				continue;
			}

			text_font_glyph* g = text_font_system_get_glyph(in_use_style->font, text[char_index]);
			x_advance += g->x_advance; // Kerning is ignored because adds compleixty to the code and are very few pixels in specific situations
			word_to_recheck += text[char_index];

			if (temporal_max_line_height < in_use_style->font->line_height) {
				temporal_max_line_height = in_use_style->font->line_height;
			}
		}
	}

	void populate_package_with_ui_text(std::vector<quad_instance_definition>& quads_data, std::vector<quad_instance_definition>& pick_quads_data) {

		// TODO: Optimization, for the pick, calculate the bounds based on the text and just create a quad that big

		std::vector<uint> ui_text = ecs_system_get_entities_by_archetype(ARCHETYPE_UI_TEXT_BOX);// TODO: If the vector is a reference and there is no text, calling the function the renderer crash?, investigate why!
		for (uint entity_index = 0; entity_index < ui_text.size(); ++entity_index) {

			uint64 size;
			ui_behaviour_component* behaviour_comp = (ui_behaviour_component*)ecs_system_get_component_data(ui_text[entity_index], UI_BEHAVIOUR_COMPONENT, size);
			if (behaviour_comp->visibility == UI_VISIBILITY_COLLAPSE) {
				// Skip and go for the next one
				continue;
			}

			quad_instance_definition quad_definition;
			quad_definition.id = ui_text[entity_index];// TODO: If wants to make each character a unique id then move this inside the for loop

			parent_component* parent_comp = (parent_component*)ecs_system_get_component_data(ui_text[entity_index], PARENT_COMPONENT, size);
			
			ui_transform_component* tran_comp = (ui_transform_component*)ecs_system_get_component_data(ui_text[entity_index], UI_TRANSFORM_COMPONENT, size);
			transform transform = transform_create();
			transform_set_rotation(transform, glm::angleAxis(glm::radians(tran_comp->roll_rotation), glm::vec3(0.f, 0.f, 1.f)));
			transform_set_scale(transform, calculate_scale_based_on_bounds(tran_comp));
			transform_set_position(transform, calculate_position_based_on_anchor_bounds_and_parent(tran_comp, tran_comp->anchor, parent_comp->parent));
			quad_definition.transform = transform;

			ui_text_component* ui_text_comp = (ui_text_component*)ecs_system_get_component_data(ui_text[entity_index], UI_TEXT_COMPONENT, size);
			// Found the glyph. generate points.
			text_style_table* style_table = text_style_system_adquire_text_style_table(std::string(&ui_text_comp->style_table_name[0]));
			
			text_style default_style = text_style_system_adquire_text_style(style_table, std::string("default"));
			
			text_style* in_use_style = &default_style;

			// Gets the style list applied to the text
			// Vectors used as stacks because they are contigous in memory
			uint current_candidate_style_index = 0;
			std::vector<text_style> styles;
			std::vector<uint> style_starting_indices;
			std::vector<uint> style_ending_indices;

			uint current_candidate_image_index = 0;
			std::vector<text_image_style> insterted_images;
			std::vector<uint> inserted_image_starting_indices;
			std::vector<uint> inserted_image_ending_indices;

			uint current_break_line_index = 0;
			std::vector<uint> break_line_indices;
			std::vector<int> line_heights;

			get_metrics_applying_style_to_text(style_table, std::string(&ui_text_comp->text[0]),
				styles, style_starting_indices, style_ending_indices,
				insterted_images, inserted_image_starting_indices, inserted_image_ending_indices,// TODO: Do this operation every time the text change or precalculate it at the level beggining
				tran_comp->bounds_max_point.x, break_line_indices, line_heights);
			

			float x_advance = 0;
			float y_advance = 0;
			// Iterates each string character
			for (uint char_index = 0; char_index < ui_text_comp->text.size(); ++char_index) {
				if (ui_text_comp->text[char_index] == '\0') {
					break;
				}


				caliope::quad_instance_definition qd = quads_data[quads_data.size() - 1];

				// Check if needs to insert a inline image
				if (current_candidate_image_index < inserted_image_starting_indices.size() && char_index == inserted_image_starting_indices[current_candidate_image_index]) {

					// For correct spacing
					quad_definition.transform.position.x = transform.position.x + (x_advance)+(insterted_images[current_candidate_image_index].image_size.x / 2) ;
					quad_definition.transform.position.y = transform.position.y + y_advance - (insterted_images[current_candidate_image_index].image_size.y / 2);

					x_advance += insterted_images[current_candidate_image_index].image_size.x;


					// Quad for the inline image
					quad_definition.transform.scale.x = insterted_images[current_candidate_image_index].image_size.x;
					quad_definition.transform.scale.y = insterted_images[current_candidate_image_index].image_size.y;

					material* mat = insterted_images[current_candidate_image_index].material;
					quad_definition.diffuse_color = mat->diffuse_color;
					quad_definition.shininess_intensity = mat->shininess_intensity;
					quad_definition.shininess_sharpness = mat->shininess_sharpness;
					quad_definition.shader = mat->shader;
					quad_definition.diffuse_texture = mat->diffuse_texture;
					quad_definition.specular_texture = mat->specular_texture;
					quad_definition.normal_texture = mat->normal_texture;

					quad_definition.diffuse_color = insterted_images[current_candidate_image_index].material->diffuse_color;
					quad_definition.z_order = 0;
					quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
						*insterted_images[current_candidate_image_index].material->diffuse_texture,
						insterted_images[current_candidate_image_index].texture_coord[0],
						insterted_images[current_candidate_image_index].texture_coord[1],
						true
					);

					quads_data.push_back(quad_definition);

					char_index = inserted_image_ending_indices[current_candidate_image_index];
					current_candidate_image_index++;
					continue;
				}

				//Checks if it needs to apply a new style 
				if (current_candidate_style_index < style_starting_indices.size() && char_index == style_starting_indices[current_candidate_style_index]) {
					in_use_style = &styles[current_candidate_style_index];
					char_index += styles[current_candidate_style_index].tag_name_length + 1; // The plus 1 is to skip the separator character '|' too
					continue;
				}

				if (current_candidate_style_index < style_starting_indices.size() && char_index == style_ending_indices[current_candidate_style_index]) {
					in_use_style = &default_style;
					current_candidate_style_index++;
					continue;
				}

				// Checks if needs to break a line
				if (current_break_line_index < break_line_indices.size() && break_line_indices[current_break_line_index] == char_index) {
					x_advance = 0;
					current_break_line_index++;
					if (current_break_line_index < line_heights.size()) {
						y_advance += line_heights[current_break_line_index] + in_use_style->additional_interlinial_space;
					}
					continue;
				}

				// Checks if needs to space
				if (ui_text_comp->text[char_index] == ' ') {
					// If there is a blank space skip to next char
					if (x_advance != 0) {
						x_advance += in_use_style->font->x_advance_space;
					}
					continue;
				}

				// Checks if needs to tab
				if (ui_text_comp->text[char_index] == '\t') {
					// If there is a tab space skip to next char
					if (x_advance != 0) {
						x_advance += in_use_style->font->x_advance_tab;
					}
					continue;
				}

				text_font_glyph* g = text_font_system_get_glyph(in_use_style->font, ui_text_comp->text[char_index]);
				// TODO: Do the alignment position calculation, kerning and spacing inside the text font system and return its value with functions?
				quad_definition.transform.position.x = transform.position.x + (x_advance)+(g->width / 2) + g->x_offset;
				float glyph_pos_y = ((g->y_offset2 + g->y_offset) / 2);
				quad_definition.transform.position.y = transform.position.y + y_advance + (glyph_pos_y);// TODO: Change the - with + when the projection is fixed


				int kerning_advance = 0;

				// Try to find kerning, if does, applies it to x_advance 
				if (char_index + 1 < ui_text_comp->text.size()) {
					text_font_glyph* g_next = text_font_system_get_glyph(in_use_style->font, ui_text_comp->text[char_index + 1]);

					//TODO: Make it more efficient!!
					for (uint i = 0; i < in_use_style->font->kernings.size(); ++i) {
						text_font_kerning* k = &in_use_style->font->kernings[i];
						if (g->kerning_index == k->codepoint1 && g_next->kerning_index == k->codepoint2) {
							kerning_advance = -(k->advance);
							break;
						}
					}
				}


				x_advance += (g->x_advance) + (kerning_advance);

				// Quad for the glyph
				quad_definition.transform.scale.x = g->width;
				quad_definition.transform.scale.y = g->height;

				material* mat = in_use_style->font->atlas_material;
				quad_definition.diffuse_color = mat->diffuse_color;
				quad_definition.shininess_intensity = mat->shininess_intensity;
				quad_definition.shininess_sharpness = mat->shininess_sharpness;
				quad_definition.shader = mat->shader;
				quad_definition.diffuse_texture = mat->diffuse_texture;
				quad_definition.specular_texture = mat->specular_texture;
				quad_definition.normal_texture = mat->normal_texture;

				quad_definition.diffuse_color = in_use_style->text_color;
				quad_definition.z_order = 0;
				quad_definition.texture_region = texture_system_calculate_custom_region_coordinates(
					*in_use_style->font->atlas_material->diffuse_texture,
					{ g->x,  g->y },
					{ g->x + g->width, (g->y + g->height) },
					false
				);

				quads_data.push_back(quad_definition);

				if (behaviour_comp->visibility == UI_VISIBILITY_VISIBLE) {
					pick_quads_data.push_back(quad_definition);
				}
			}
		}
	}

	void ui_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* ui_cam_in_use, float delta_time) {

		std::vector<quad_instance_definition> quads_data;
		std::vector<quad_instance_definition> pick_quads_data;

		
		populate_package_with_ui_image(quads_data, pick_quads_data);
		populate_package_with_ui_button(quads_data, pick_quads_data);
		populate_package_with_ui_text(quads_data, pick_quads_data);
		

		renderer_view_packet ui_packet;
		ui_packet.view_type = VIEW_TYPE_UI;
		render_view_system_on_build_packet(VIEW_TYPE_UI, ui_packet, std::vector<std::any>({ quads_data, ui_cam_in_use, delta_time }));
		packets.push_back(ui_packet);

		renderer_view_packet pick_object_packet;
		pick_object_packet.view_type = VIEW_TYPE_UI_OBJECT_PICK;
		render_view_system_on_build_packet(VIEW_TYPE_UI_OBJECT_PICK, pick_object_packet, std::vector<std::any>({ pick_quads_data, ui_cam_in_use, delta_time }));
		packets.push_back(pick_object_packet);
	}
}