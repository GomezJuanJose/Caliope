#include "platform.h"

#if CE_PLATFORM_WINDOWS
#include "core/logger.h"
#include "core/input.h"
#include "core/event.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "cepch.h"


namespace caliope {

	typedef struct platform_windows_state {
		GLFWwindow* window;
	} platform_windows_state;

	static std::unique_ptr<platform_windows_state> state_ptr;

	bool platform_system_initialize(const std::string& window_name, int width, int height) {

		state_ptr = std::make_unique<platform_windows_state>();

		if (!state_ptr || !glfwInit()) {
			return false;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		state_ptr->window = glfwCreateWindow(width, height, window_name.c_str(), NULL, NULL);

		// Set callbacks for the windows event
		glfwSetWindowSizeCallback(state_ptr->window, [](GLFWwindow* window, int width, int height) {
				int size[] = { width, height };
				event_fire(EVENT_CODE_MOUSE_RESIZED, size);
			}
		);

		glfwSetKeyCallback(state_ptr->window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
				switch (action) {
					case GLFW_PRESS:
					case GLFW_REPEAT:
						input_system_process_key(key, true);
						break;

					case GLFW_RELEASE:
						input_system_process_key(key, false);
						break;


				}
			}
		);

		glfwSetMouseButtonCallback(state_ptr->window, [](GLFWwindow* window, int button, int action, int mods) {
				switch (action) {
					case GLFW_PRESS:
						input_system_process_button(button, true);
						break;

					case GLFW_RELEASE:
						input_system_process_button(button, false);
						break;
				}
			}
		);

		glfwSetCursorPosCallback(state_ptr->window, [](GLFWwindow* window, double xPos, double yPos) {
				input_system_process_mouse_move(xPos, yPos);
			}
		);

		glfwSetScrollCallback(state_ptr->window, [](GLFWwindow* window, double xOffset, double yOffset) {
				input_system_process_scroll(xOffset, yOffset);
			}
		);



		CE_LOG_INFO("Windows platform initialized");
		return true;
	}

	void platform_system_shutdown() {
		glfwDestroyWindow(state_ptr->window);
		glfwTerminate();
	}

	bool platform_system_pump_event() {
		glfwPollEvents();
		return !glfwWindowShouldClose(state_ptr->window);
	}

	std::any platform_system_get_window() {
		return state_ptr->window;
	}

	void* platform_system_allocate_memory(size_t size) {
		return malloc(size);
	}

	void platform_system_free_memory(void* block) {
		free(block);
	}

	void* platform_system_zero_memory(void* block, uint64 size) {
		return memset(block, 0, size);
	}

	void* platform_system_copy_memory(void* dest, const void* source, uint64 size) {
		return memcpy(dest, source, size);
	}

	void* platform_system_set_memory(void* dest, int value, uint64 size) {
		return memset(dest, value, size);
	}
}
#endif // CE_PLATFORM_WINDOWS
