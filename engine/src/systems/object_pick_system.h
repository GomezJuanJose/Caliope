#pragma once
#include "defines.h"
#include <vector>


namespace caliope {

	bool object_pick_system_initialize();
	void object_pick_system_shutdown();

	CE_API uint object_pick_system_get_current_hover_entity();
	CE_API uint object_pick_system_get_world_hover_entity();
	CE_API uint object_pick_system_get_ui_hover_entity();

}