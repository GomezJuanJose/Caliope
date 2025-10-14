#include "audio_frontend.h"
#include "core/logger.h"
#include "core/cememory.h"
#include "core/asserts.h"

#include <miniaudio.h>
#include <thread>
#include <chrono>

#include <fstream>

namespace caliope {

#define MAX_SOUND_COUNT 256

	typedef struct miniaudio_state {

		ma_engine engine;

		std::array<ma_sound, MAX_SOUND_COUNT> sounds;
		std::vector<uint> reusable_ids;

		uint current_sounds_count;

	} miniaudio_state;

	std::unique_ptr<miniaudio_state> state_ptr;


	bool audio_frontend_create() {
		state_ptr = std::make_unique<miniaudio_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		state_ptr->current_sounds_count = 0;

		if (ma_engine_init(NULL, &state_ptr->engine) != MA_SUCCESS) {
			return false;
		}


		

		return true;
	}

	void audio_frontend_destroy() {

		ma_engine_uninit(&state_ptr->engine);

		for (uint i = 0; i < MAX_SOUND_COUNT; ++i) {
			if (state_ptr->sounds[i].pDataSource != nullptr) {
				ma_sound_uninit(&state_ptr->sounds[i]);
			}
		}

		state_ptr.reset();
		state_ptr = nullptr;
	}


	uint audio_frontend_create_emmiter(uint format,	int channels, uint sample_rate, uint total_samples_left , void* data, uint data_size) {
		
		if (state_ptr->current_sounds_count >= MAX_SOUND_COUNT) {
			CE_LOG_WARNING("create_emmiter reached maximum emmiters created, use destroy_emmiter to free space. Returning 0");
			return 0;
		}

		bool has_reusable_id = !state_ptr->reusable_ids.empty();

		uint source_id = -1;
		if (has_reusable_id) {
			source_id = state_ptr->reusable_ids.back();
			state_ptr->reusable_ids.pop_back();

		} else {

			source_id = state_ptr->current_sounds_count;
			state_ptr->current_sounds_count++;
		}



		ma_decoder decoder;
		ma_uint64 length_in_pcm_frames;
		ma_result result = ma_decoder_init_file((const char*)data, NULL, &decoder);
		ma_decoder_get_length_in_pcm_frames(&decoder, &length_in_pcm_frames);
		float length_in_seconds = length_in_pcm_frames / (float)decoder.outputSampleRate;
		ma_decoder_uninit(&decoder);

		ma_uint64 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;

		// If the audio is long then stream loading
		if (length_in_seconds > 10) {
			flags |= MA_SOUND_FLAG_STREAM;
		}

		ma_sound_init_from_file(&state_ptr->engine, (const char*)data, flags, nullptr, nullptr, &state_ptr->sounds[source_id]);

		return source_id;
	}

	void audio_frontend_destroy_emmiter(uint emmiter_id) {
		ma_sound_uninit(&state_ptr->sounds[emmiter_id]);
		state_ptr->reusable_ids.push_back(emmiter_id);
	}

	void play_emmiter(uint emmiter_id, uint delay_ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
		ma_sound_start(&state_ptr->sounds[emmiter_id]);
	}
	void audio_frontend_play_emmiter(uint emmiter_id, uint delay_ms) {
		std::thread thread(play_emmiter, emmiter_id, delay_ms);
		thread.detach();
	}

	void stop_emmiter(uint emmiter_id, uint delay_ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
		ma_sound_stop(&state_ptr->sounds[emmiter_id]);
		ma_sound_seek_to_pcm_frame(&state_ptr->sounds[emmiter_id], 0);
	}
	void audio_frontend_stop_emmiter(uint emmiter_id, uint delay_ms) {
		std::thread thread(stop_emmiter, emmiter_id, delay_ms);
		thread.detach();
	}

	void pause_emmiter(uint emmiter_id, uint delay_ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
		ma_sound_stop(&state_ptr->sounds[emmiter_id]);
	}
	void audio_frontend_pause_emmiter(uint emmiter_id, uint delay_ms) {
		std::thread thread(pause_emmiter, emmiter_id, delay_ms);
		thread.detach();
	}

	void audio_frontend_fade_emmiter(uint emmiter_id, float begin_volume, float end_volume, uint64 time_ms) {
		ma_sound_set_fade_in_milliseconds(&state_ptr->sounds[emmiter_id], begin_volume, end_volume, time_ms);
	}

	void audio_frontend_loop_emmiter(uint emmiter_id, bool loop) {
		ma_sound_set_looping(&state_ptr->sounds[emmiter_id], loop);
	}

	void audio_frontend_set_emmiter_gain(uint emmiter_id, float gain) {
		ma_sound_set_volume(&state_ptr->sounds[emmiter_id], gain);
	}

	void audio_frontend_positionate_emmiter(uint emmiter_id, glm::vec3 position) {
		ma_sound_set_position(&state_ptr->sounds[emmiter_id], position.x, position.y, position.z);
	}

	void audio_frontend_move_listener(glm::vec3 new_position) {
		ma_engine_listener_set_position(&state_ptr->engine, 0, new_position.x, new_position.y, new_position.z);
	}
}