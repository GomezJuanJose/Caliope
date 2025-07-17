#include "camera_system.h"

#include "cepch.h"
#include "core/logger.h"

#include "renderer/camera.h"

namespace caliope {
	typedef struct camera_reference {
		camera camera;
		uint reference_count;
	} camera_reference;

	typedef struct camera_system_state {
		std::unordered_map<std::string, camera_reference> registered_cameras;

		camera default_camera;

	}camera_system_state;

	static std::unique_ptr<camera_system_state> state_ptr;

	bool camera_system_initialize() {

		state_ptr = std::make_unique<camera_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->default_camera = camera_create();

		CE_LOG_INFO("Camera system initialized.");
		return true;
	}

	void camera_system_shutdown() {
		state_ptr.reset();
		state_ptr = nullptr;
	}

	
	camera* camera_system_acquire(const std::string& name) {
		if (state_ptr->registered_cameras.find(name) == state_ptr->registered_cameras.end()) {
			camera_reference cr;
			cr.camera = camera_create();
			state_ptr->registered_cameras.insert({ name, cr });
		}

		state_ptr->registered_cameras[name].reference_count++;
		return &state_ptr->registered_cameras[name].camera;
	}
	
	void camera_system_release(const std::string& name) {
		if (state_ptr->registered_cameras.find(name) != state_ptr->registered_cameras.end()) {
			state_ptr->registered_cameras[name].reference_count--;

			if (state_ptr->registered_cameras[name].reference_count <= 0) {
				state_ptr->registered_cameras.erase(name);
			}
		}
	}
	
	camera* camera_system_get_default() {
		return &state_ptr->default_camera;
	}
}