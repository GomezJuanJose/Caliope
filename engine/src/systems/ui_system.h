#pragma once
#include "defines.h"


namespace caliope {

	struct renderer_view_packet;
	struct transform;
	struct camera;
	struct transform_component;
	struct ui_transform_component;
	struct ui_material_component;
	struct ui_dynamic_material_component;
	struct ui_events_component;
	struct ui_text_component;
	struct ui_behaviour_component;
	struct ui_container_component;
	struct quad_instance_definition;
	enum archetype;
	enum component_id;

	typedef struct ui_system_configuration {
		uint max_number_entities;
		uint16 initial_window_width;
		uint16 initial_window_height;
	} ui_system_configuration;

	bool ui_system_initialize(ui_system_configuration& config);
	void ui_system_shutdown();
	void ui_system_on_resize(uint16 width, uint16 height);

	void ui_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* ui_cam_in_use, float delta_time);

	CE_API bool ui_system_create_empty_layout(std::string& name, bool enable_by_default);
	//CE_API bool ui_system_load_layout(std::string& name, bool enable_by_default);
	//CE_API void ui_system_unload_layout(std::string& name);
	// Saves the scene passed as argument in a binary file.
	//CE_API bool ui_system_save_layout(std::string& name);

	// returns the entity ID, if -1 means error.
	// TODO: Do it like the scene system (just one function)?
	CE_API uint ui_system_instance_image(std::string& name, ui_transform_component& transform, ui_material_component& ui_material, ui_behaviour_component& cursor_behaviour);
	CE_API uint ui_system_instance_button(std::string& name, ui_transform_component& transform, ui_dynamic_material_component& ui_dynamic_material, ui_events_component& ui_mouse_events, ui_behaviour_component& cursor_behaviour);
	CE_API uint ui_system_instance_text_box(std::string& name, ui_transform_component& transform, ui_text_component& ui_text, ui_behaviour_component& cursor_behaviour);
	CE_API uint ui_system_instance_container_box(std::string& name, ui_transform_component& transform, ui_container_component& box);

	CE_API void ui_system_destroy_entity(std::string& name, uint entity);

	CE_API bool ui_system_parent_entities(uint child, uint parent);

	CE_API void ui_system_enable_layout(std::string& name, bool enable);

}