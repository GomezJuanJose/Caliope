#include "object_pick_system.h"
#include "core/logger.h"
#include "core/cememory.h"
#include "core/event.h"
#include "resources/resources_types.inl"


#include "renderer/renderer_types.inl"


namespace caliope {

	typedef struct object_pick_system_state {
		uint current_hover_entity;
		uint current_world_hover_entity;
		uint current_ui_hover_entity;
	}object_pick_system_state;

	std::unique_ptr<object_pick_system_state> state_ptr;

	// NOTE: The first element is the entity id and the second wich view the entity belongs to
	bool on_entity_hover(caliope::event_system_code code, std::any data) {
		uint* data_array = std::any_cast<uint*>(data);

		uint id = data_array[0];
		view_type type = (view_type)(data_array[1]);

		if (type == 1) {// 1 == VIEW_TYPE_WORLD_OBJECT_PICK
			state_ptr->current_world_hover_entity = id;
		}
		else {
			state_ptr->current_ui_hover_entity = id;
		}

		if (state_ptr->current_ui_hover_entity == -1) {
			state_ptr->current_hover_entity = state_ptr->current_world_hover_entity;
		}
		else
		{
			state_ptr->current_hover_entity = state_ptr->current_ui_hover_entity;
		}

		return true;
	}

	bool object_pick_system_initialize()
	{
		state_ptr = std::make_unique<object_pick_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		event_register(EVENT_CODE_ON_ENTITY_HOVER, on_entity_hover);

		CE_LOG_INFO("Object pick system initialized.");

		return true;
	}

	void object_pick_system_shutdown()
	{
		state_ptr.reset();
	}

	uint object_pick_system_get_current_hover_entity()
	{
		return state_ptr->current_hover_entity;
	}

	uint object_pick_system_get_world_hover_entity()
	{
		return state_ptr->current_world_hover_entity;
	}

	uint object_pick_system_get_ui_hover_entity()
	{
		return state_ptr->current_ui_hover_entity;
	}
}