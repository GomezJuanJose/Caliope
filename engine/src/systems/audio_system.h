#pragma once
#include "defines.h"
#include <glm/glm.hpp>


namespace caliope {
	bool audio_system_initialize();
	void audio_system_shutdown();

	// Load a audio file returns and id. The name must be only the asset name
	CE_API uint audio_system_create_emmiter(std::string& name);
	CE_API void audio_system_destroy_emmiter(uint emmiter_id);

	CE_API void audio_system_play_emmiter(uint emmiter_id, uint delay_ms);
	CE_API void audio_system_stop_emmiter(uint emmiter_id, uint delay_ms);
	CE_API void audio_system_pause_emmiter(uint emmiter_id, uint delay_ms);

	// In -1 is passed to begin_volume and end_volumen then it will use the current emmiter audio
	CE_API void audio_system_fade_emmiter(uint emmiter_id, float begin_volume, float end_volume, uint64 time_ms);
	CE_API void audio_system_loop_emmiter(uint emmiter_id, bool loop);
	CE_API void audio_system_set_emmiter_gain(uint emmiter_id, float gain);
	CE_API void audio_system_positionate_emmiter(uint emmiter_id, glm::vec3 position);

	CE_API void audio_system_move_listener(glm::vec3 new_position);
}