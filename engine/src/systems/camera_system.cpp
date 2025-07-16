#include "camera_system.h"

#include "cepch.h"
#include "core/logger.h"

#include "renderer/camera.h"

namespace caliope {
	typedef struct camera_system_state {
		std::unordered_map<std::string, camera> registered_cameras;

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

	
	std::shared_ptr<camera> camera_system_acquire(const std::string& name) {
		if (state_ptr->registered_cameras.find(name) == state_ptr->registered_cameras.end()) {

			state_ptr->registered_cameras.insert({ name, camera_create() });
		}

		std::shared_ptr<camera> cam = std::make_shared<camera>(state_ptr->registered_cameras[name]);
		return cam;
	}
	
	void camera_system_release(const std::string& name) {
		if (state_ptr->registered_cameras.find(name) != state_ptr->registered_cameras.end()) {
			state_ptr->registered_cameras.erase(name);
		}
	}
	
	std::shared_ptr<camera> camera_system_get_default() {
		std::shared_ptr<camera> cam = std::make_shared<camera>(state_ptr->default_camera);
		return cam;
	}
}