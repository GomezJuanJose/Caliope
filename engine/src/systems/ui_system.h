#pragma once
#include "defines.h"


namespace caliope {

	struct renderer_view_packet;
	struct transform;
	struct camera;
	struct transform_component;
	struct ui_material_component;
	struct quad_definition;
	enum archetype;
	enum component_id;

	typedef struct ui_system_configuration {
		uint max_number_entities;
	} ui_system_configuration;

	bool ui_system_initialize(ui_system_configuration& config);
	void ui_system_shutdown();

	void ui_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* ui_cam_in_use, float delta_time);

	CE_API bool ui_system_create_empty_layout(std::string& name, bool enable_by_default);
	//CE_API bool ui_system_load_layout(std::string& name, bool enable_by_default);
	//CE_API void ui_system_unload_layout(std::string& name);
	// Saves the scene passed as argument in a binary file.
	//CE_API bool ui_system_save_layout(std::string& name);

	CE_API bool ui_system_instance_image(std::string& name, transform_component& transform, ui_material_component& ui_material);
	CE_API void ui_system_destroy_entity(std::string& name, uint entity);

	CE_API void ui_system_enable_layout(std::string& name, bool enable);

}