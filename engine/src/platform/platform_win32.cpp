#include "platform.h"

#if CE_PLATFORM_WINDOWS
#include "cepch.h"

#include "core/logger.h"
#include "core/input.h"
#include "core/event.h"
#include "core/cememory.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <windows.h>




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
		state_ptr.reset();
		state_ptr = nullptr;
	}

	bool platform_system_pump_event() {
		glfwPollEvents();
		return !glfwWindowShouldClose(state_ptr->window);
	}

	std::any platform_system_get_window() {
		return state_ptr->window;
	}

	float platform_system_get_time() {
		return (float)glfwGetTime();
	}

	void platform_system_console_write(const char* message, uchar log_level) {
		HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		// FATAL, ERROR, WARN, INFO
		static uchar levels[4] = { 64, 4, 6, 8};
		SetConsoleTextAttribute(console_handle, levels[log_level]);
		OutputDebugStringA(message);
		uint64 length = strlen(message);
		LPDWORD number_written = 0;
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
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


	bool platform_system_open_file(const char* path, std::any& handle, int mode) {

		HANDLE file = CreateFile(
			path,
			mode == 0X1 ? GENERIC_READ : GENERIC_WRITE,
			mode == 0X1 ? FILE_SHARE_READ : 0,
			NULL,
			mode == 0X1 ? OPEN_EXISTING : CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (file == INVALID_HANDLE_VALUE) {
			const DWORD error = GetLastError();
			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
			std::string message(messageBuffer, size);

			CE_LOG_ERROR(message.c_str());
			return false;
		}

		handle = file;

		return true;
	}

	void platform_system_close_file(std::any& handle) {
		HANDLE h = std::any_cast<HANDLE>(handle);
		CloseHandle(h);
	}

	uint64 platform_system_file_size(std::any& handle) {
		LARGE_INTEGER file_size;
		HANDLE h = std::any_cast<HANDLE>(handle);
		if (GetFileSizeEx(h, &file_size) == 0) {
			return 0;
		}	

		return file_size.QuadPart;
	}

	bool platform_system_file_read_text(std::any& handle, uint64 max_length, char* line_buf) {
		HANDLE h = std::any_cast<HANDLE>(handle);

		OVERLAPPED ol = { 0 };
		return ReadFileEx(h, line_buf, max_length - 1, &ol, NULL) != FALSE;

	}

	bool platform_system_file_write_text(std::any& handle, const char* text) {
		HANDLE h = std::any_cast<HANDLE>(handle);
		DWORD dw_bytes_to_write = (DWORD)strlen(text);
		DWORD dw_bytes_written = 0;

		return WriteFile(h, text, dw_bytes_to_write, &dw_bytes_written, NULL) != FALSE;
	}

	uint64 platform_system_file_read_bytes(std::any& handle, uint64 size, uchar* data) {
		HANDLE h = std::any_cast<HANDLE>(handle);
		DWORD bytes_read;
		
		bool result = ReadFile(h, data, size, &bytes_read, nullptr) != FALSE;
	
		return bytes_read;
	}

	bool platform_system_file_write_bytes(std::any& handle, uint64 size, void* data) {
		HANDLE h = std::any_cast<HANDLE>(handle);
		DWORD bytes_written = 0;
		
		return WriteFile(h, data, size, &bytes_written, NULL) != FALSE;
	}
}
#endif // CE_PLATFORM_WINDOWS
