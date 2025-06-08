#pragma once

#include "defines.h"

#include <string>

namespace caliope {

	typedef enum renderer_backend_type {
		BACKEND_TYPE_VULKAN,
		BACKEND_TYPE_DX
	} renderer_backend_type;

	typedef struct renderer_backend {

		bool (*initialize)(const std::string& application_name);
		void (*shutdown)();
		void (*resize)(uint16 width, uint16 height);

		bool (*begin_frame)(float delta_time);
		bool (*end_frame)(float delta_time);
	};

	typedef struct renderer_packet {
		float delta_time;
	} renderer_packet;
}