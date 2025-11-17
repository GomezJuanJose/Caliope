#pragma once
#include "defines.h"
#include <vector>


namespace caliope {

	bool object_pick_system_initialize();
	void object_pick_system_shutdown();

	void object_pick_system_set_hover_entity(bool world_entity, uint id);

	CE_API uint object_pick_system_get_current_hover_entity();
	CE_API uint object_pick_system_get_world_hover_entity();
	CE_API uint object_pick_system_get_ui_hover_entity();

	CE_API uint object_pick_system_get_current_pressed_entity();
	CE_API uint object_pick_system_get_world_pressed_entity();
	CE_API uint object_pick_system_get_ui_pressed_entity();
}