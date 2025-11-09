#pragma once

#include <string>
#include <memory>

namespace caliope {
	struct camera;

	typedef struct game_state {
		camera* world_camera;
		camera* ui_camera;
	} game_state;

	typedef struct program_config {

		std::string name;
		int width;
		int height;
		unsigned int maximum_number_entities_per_frame; // Maximum number of entities that the renderer can render each frame
		unsigned int maximum_number_textures_per_frame; // Maximum number of textures that the renderer can use each frame

		bool (*initialize) (game_state& game_state);
		bool (*update) (game_state& game_state, float delta_time);
		bool (*resize) ();
		void (*shutdown) ();

		game_state game_state;
	} program_config;
}