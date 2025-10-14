#include "event.h"
#include "cepch.h"
#include "core/logger.h"

namespace caliope {
	typedef struct event_system_state {

		std::vector<function_on_event> registered_events[event_system_code::MAX_EVENT_CODE];

	} event_system_state;

	static std::unique_ptr<event_system_state>state_ptr;

	bool event_system_initialize() {

		state_ptr = std::make_unique<event_system_state>();
		
		if (state_ptr == nullptr) {
			return false;
		}

		CE_LOG_INFO("Event system initialized");

		return true;
	}

	void event_system_shutdown() {
		for (int i = 0; i < event_system_code::MAX_EVENT_CODE; ++i) {
			state_ptr->registered_events[i].clear();
		}
		state_ptr.reset();
	}

	void event_register(event_system_code code, function_on_event on_event) {
		state_ptr->registered_events[code].push_back(on_event);
	}

	void event_unregister(event_system_code code, function_on_event on_event) {
		state_ptr->registered_events[code].erase(
			std::remove_if(
				begin(state_ptr->registered_events[code]), 
				end(state_ptr->registered_events[code]),
				[&on_event](function_on_event fn) {
					return fn == on_event;
				}
			),
			end(state_ptr->registered_events[code])
		);
	}

	bool event_fire(event_system_code code, std::any data) {
		int max_fnc = state_ptr->registered_events[code].size();
		
		for (int i = 0; i < max_fnc; ++i) {
			if (state_ptr->registered_events[code][i](code, data)) {
				// The event has been handled, do not send others.
				return true;
			}
		}

		// Not found events or handled every one
		return false;
	}
}