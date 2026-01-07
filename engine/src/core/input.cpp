#include "input.h"
#include "event.h"

namespace caliope {

	typedef enum input_status {
		INPUT_PRESSED,
		INPUT_RELEASED,
		INPUT_NEUTRAL // This means that is released but the release status was the frame before
	} input_status;

	typedef struct input_system_state {

		std::array<input_status, MAX_KEYS> keyboard_status;
		int current_frame_key;

		std::array <input_status, MAX_BUTTONS> mouse_status;
		int current_frame_button;
		short mouse_x;
		short mouse_y;

	}input_system_state;

	static std::unique_ptr<input_system_state> state_ptr;

	bool input_system_initialize() {

		state_ptr = std::make_unique<input_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->keyboard_status.fill(input_status::INPUT_NEUTRAL);
		state_ptr->mouse_status.fill(input_status::INPUT_NEUTRAL);

		return true;
	}

	void input_system_shutdown() {
		state_ptr.reset();
	}

	void input_system_update_inputs(float delta_time) {
		state_ptr->keyboard_status[state_ptr->current_frame_key] = input_status::INPUT_NEUTRAL;
		state_ptr->mouse_status[state_ptr->current_frame_button] = input_status::INPUT_NEUTRAL;
	}

	bool is_key_pressed(key keycode) {
		return state_ptr->keyboard_status[keycode] == INPUT_PRESSED;
	}

	bool is_key_released(key keycode) {
		return state_ptr->keyboard_status[keycode] == INPUT_RELEASED;
	}

	bool is_button_pressed(button_input buttoncode) {
		return state_ptr->mouse_status[buttoncode] == INPUT_PRESSED;
	}

	bool is_button_released(button_input buttoncode) {
		return state_ptr->mouse_status[buttoncode] == INPUT_RELEASED;
	}

	std::array<int, 2> get_mouse_position() {
		return { state_ptr->mouse_x, state_ptr->mouse_y };
	}

	void input_system_process_key(int keycode, bool pressed) {
		if (keycode < MAX_KEYS && state_ptr->keyboard_status[keycode] != pressed) {
			state_ptr->keyboard_status[keycode] = pressed ? INPUT_PRESSED : INPUT_RELEASED;
			state_ptr->current_frame_key = keycode;

			event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, (key)keycode);
		}
	}

	void input_system_process_button(int keycode, bool pressed) {
		if (keycode < MAX_BUTTONS && state_ptr->mouse_status[keycode] != pressed) {
			state_ptr->mouse_status[keycode] = pressed ? INPUT_PRESSED : INPUT_RELEASED;
			state_ptr->current_frame_button = keycode;

			event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, (button_input)keycode);
		}
	}

	void input_system_process_mouse_move(double x, double y) {
		int position[2] = { (int)x, (int)y };
		state_ptr->mouse_x = position[0];
		state_ptr->mouse_y = position[1];
		event_fire(EVENT_CODE_MOUSE_MOVED, position);
	}

	void input_system_process_scroll(double xoffset, double yoffset) {
		double scroll[2] = { xoffset, yoffset };
		event_fire(EVENT_CODE_MOUSE_WHEEL, scroll);
	}
}