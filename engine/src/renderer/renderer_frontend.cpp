#include "renderer_frontend.h"
#include "cepch.h"

#include "core/logger.h"

#include "renderer/renderer_types.inl"
#include "renderer/renderer_backend.h"

namespace caliope{

	typedef struct renderer_system_state {
		renderer_backend backend;
	} renderer_system_state;

	static std::unique_ptr<renderer_system_state> state_ptr;

	bool caliope::renderer_system_initialize(const std::string& application_name) {
		state_ptr = std::make_unique<renderer_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		renderer_backend_system_create(renderer_backend_type::BACKEND_TYPE_VULKAN, state_ptr->backend);

		if (!state_ptr->backend.initialize(application_name)) {
			CE_LOG_ERROR("Renderer backend failed to initialized. Shutting down");
			return false;
		}

		return true;
	}

	void caliope::renderer_system_shutdown() {
		if (state_ptr != nullptr) {
			state_ptr->backend.shutdown();
			renderer_backend_system_destroy(state_ptr->backend);
			state_ptr.reset();
		}
	}

	void caliope::renderer_on_resized(uint16 width, uint16 height) {
		if (state_ptr->backend.resize != nullptr) {
			state_ptr->backend.resize(width, height);
		}
		else {
			CE_LOG_WARNING("Renderer backend does not support a resize function");
		}
	}

	bool caliope::renderer_draw_frame(renderer_packet& packet) {
		if (state_ptr->backend.begin_frame(packet.delta_time)) {


			// TODO: Renderpasses


			if (!state_ptr->backend.end_frame(packet.delta_time)) {
				CE_LOG_ERROR("renderer_end_frame failed. Application shutting down");
				return false;
			}
		}

		return true;
	}
}