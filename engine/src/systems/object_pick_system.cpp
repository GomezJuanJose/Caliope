#include "object_pick_system.h"
#include "core/logger.h"
#include "core/cememory.h"
#include "core/event.h"
#include "core/input.h"
#include "resources/resources_types.inl"


#include "renderer/renderer_types.inl"


namespace caliope {

	typedef struct object_pick_system_state {
		uint current_hover_entity;
		uint current_world_hover_entity;
		uint current_ui_hover_entity;

		uint current_pressed_entity;
		uint current_world_pressed_entity;
		uint current_ui_pressed_entity;

		bool is_pressing;
	}object_pick_system_state;

	std::unique_ptr<object_pick_system_state> state_ptr;

	// NOTE: Data is the key code pressed
	bool on_entity_pressed(event_system_code code, std::any data) {
		button keycode = std::any_cast<button>(data);

		if (keycode == BUTTON_LEFT) {
			state_ptr->is_pressing = true;
			state_ptr->current_pressed_entity = state_ptr->current_hover_entity;
			state_ptr->current_world_pressed_entity = state_ptr->current_world_hover_entity;
			state_ptr->current_ui_pressed_entity = state_ptr->current_ui_hover_entity;

			event_fire(EVENT_CODE_ON_ENTITY_PRESSED, state_ptr->current_pressed_entity);
		}


		return false;
	}

	// NOTE: Data is the key code released
	bool on_entity_released(event_system_code code, std::any data) {

		button keycode = std::any_cast<button>(data);

		if (keycode == BUTTON_LEFT) {
			state_ptr->is_pressing = false;
			event_fire(EVENT_CODE_ON_ENTITY_RELEASED, state_ptr->current_pressed_entity);

			state_ptr->current_pressed_entity = -1;
			state_ptr->current_world_pressed_entity = -1;
			state_ptr->current_ui_pressed_entity = -1;
		}


		return false;
	}

	bool object_pick_system_initialize()
	{
		state_ptr = std::make_unique<object_pick_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		event_register(EVENT_CODE_BUTTON_PRESSED, on_entity_pressed);
		event_register(EVENT_CODE_BUTTON_RELEASED, on_entity_released);

		CE_LOG_INFO("Object pick system initialized.");

		return true;
	}

	void object_pick_system_shutdown()
	{
		state_ptr.reset();
	}

	void object_pick_system_set_hover_entity(bool world_entity, uint id)
	{
		if (world_entity) {
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

		if (state_ptr->is_pressing == true) {
			return;
		}

		event_fire(EVENT_CODE_ON_ENTITY_HOVER, state_ptr->current_hover_entity);
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

	uint object_pick_system_get_current_pressed_entity()
	{
		return state_ptr->current_pressed_entity;
	}

	uint object_pick_system_get_world_pressed_entity()
	{
		return state_ptr->current_world_pressed_entity;
	}

	uint object_pick_system_get_ui_pressed_entity()
	{
		return state_ptr->current_ui_pressed_entity;
	}
}