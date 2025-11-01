#pragma once
#include "defines.h"
#include <any>


namespace caliope {
	typedef enum event_system_code {
		EVENT_CODE_APPLICATION_QUIT = 0X01,
		EVENT_CODE_KEY_PRESSED = 0X02,
		EVENT_CODE_KEY_RELEASED = 0X03,
		EVENT_CODE_BUTTON_PRESSED = 0X04,
		EVENT_CODE_BUTTON_RELEASED = 0X05,
		EVENT_CODE_MOUSE_MOVED = 0X06,
		EVENT_CODE_MOUSE_WHEEL = 0X07,
		EVENT_CODE_MOUSE_RESIZED = 0X08,

		EVENT_CODE_ON_ENTITY_HOVER = 0X09,


		MAX_EVENT_CODE = 0XFF
	}event_system_code;
	
	typedef bool (*function_on_event)(event_system_code code, std::any data);

	bool event_system_initialize();
	void event_system_shutdown();

	CE_API void event_register(event_system_code code, function_on_event on_event);
	CE_API void event_unregister(event_system_code code, function_on_event on_event);

	bool event_fire(event_system_code code, std::any data);
}