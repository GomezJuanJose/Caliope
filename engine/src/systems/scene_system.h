#pragma once
#include "defines.h"


namespace caliope {

	struct renderer_view_packet;
	struct transform;
	struct camera;
	enum archetype;
	enum component_id;

	typedef struct scene_system_configuration {
		uint max_number_entities;
	} scene_system_configuration;

	bool scene_system_initialize(scene_system_configuration& config);
	void scene_system_shutdown();
	void scene_system_populate_render_packet(std::vector<renderer_view_packet>& packets, camera* world_cam_in_use, float delta_time);

	CE_API bool scene_system_create_empty(std::string& name, bool enable_by_default);
	CE_API bool scene_system_load(std::string& name, bool enable_by_default);
	CE_API void scene_system_unload(std::string& name);
	// Saves the scene passed as argument in a binary file.
	CE_API bool scene_system_save(std::string& name);

	CE_API bool scene_system_instance_entity(std::string& name, archetype archetype, std::vector<component_id>& components, std::vector<void*>& components_data);
	CE_API void scene_system_destroy_entity(std::string& name, uint entity);

	CE_API void scene_system_enable(std::string& name, bool enable);

}