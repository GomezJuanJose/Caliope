#pragma once
#include "defines.h"
#include <glm/glm.hpp>

namespace caliope {
	bool audio_frontend_create();
	void audio_frontend_destroy();

	uint audio_frontend_create_emmiter(uint format, int channels, uint sample_rate, uint total_samples_left, void* data, uint data_size);
	void audio_frontend_destroy_emmiter(uint emmiter_id);
	void audio_frontend_play_emmiter(uint emmiter_id, uint delay_ms);
	void audio_frontend_stop_emmiter(uint emmiter_id, uint delay_ms);
	void audio_frontend_pause_emmiter(uint emmiter_id, uint delay_ms);

	// In -1 is passed to begin_volume and end_volumen then it will use the current emmiter audio
	void audio_frontend_fade_emmiter(uint emmiter_id, float begin_volume, float end_volume, uint64 time_ms);
	void audio_frontend_loop_emmiter(uint emmiter_id, bool loop);
	void audio_frontend_set_emmiter_gain(uint emmiter_id, float gain);
	void audio_frontend_positionate_emmiter(uint emmiter_id, glm::vec3 position);

	void audio_frontend_move_listener(glm::vec3 new_position);
}