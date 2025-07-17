#pragma once

#include <string>
#include <memory>

namespace caliope {
	struct camera;

	typedef struct game_state {
		camera* world_camera;
	} game_state;

	typedef struct program_config {

		std::string name;
		int width;
		int height;

		bool (*initialize) (game_state& game_state);
		bool (*update) (game_state& game_state, float delta_time);
		bool (*resize) ();
		void (*shutdown) ();

		game_state game_state;
	} program_config;
}