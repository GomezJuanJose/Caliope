#include "audio_system.h"

#include "audio/audio_frontend.h"


#include "core/logger.h"
#include "loaders/resources_types.inl"
#include "systems/resource_system.h"

namespace caliope {
	bool audio_system_initialize() {

		if (!audio_frontend_create()) {
			return false;
		}

		CE_LOG_INFO("Audio system initialized.");
		return true;
	}
	
	void audio_system_shutdown() {
		audio_frontend_destroy();
	}


	uint audio_system_create_emmiter(std::string& name) {
		resource r;
		if (!resource_system_load(name, RESOURCE_TYPE_AUDIO, r)) {
			CE_LOG_ERROR("audio_system_create_emmiter couldnt load file audio clip");
			return 0; // TODO: Devolver un audio por defecto? osea un audio que siempre esta cargado en el 0
		}
		audio_clip_resource_data clip_data = std::any_cast<audio_clip_resource_data>(r.data);
		
		uint emmiter_id = audio_frontend_create_emmiter(clip_data.format, clip_data.channels, clip_data.sample_rate, clip_data.total_samples_left, clip_data.buffer, clip_data.buffer_size);
	
		resource_system_unload(r);

		return emmiter_id;
	}

	void audio_system_destroy_emmiter(uint emmiter_id) {
		audio_frontend_destroy_emmiter(emmiter_id);
	}

	void audio_system_play_emmiter(uint emmiter_id, uint delay_ms) {
		audio_frontend_play_emmiter(emmiter_id, delay_ms);
	}

	void audio_system_fade_emmiter(uint emmiter_id, float begin_volume, float end_volume, uint64 time_ms) {
		audio_frontend_fade_emmiter(emmiter_id, begin_volume, end_volume, time_ms);
	}

	void audio_system_loop_emmiter(uint emmiter_id, bool loop) {
		audio_frontend_loop_emmiter(emmiter_id, loop);
	}

	void audio_system_set_emmiter_gain(uint emmiter_id, float gain) {
		audio_frontend_set_emmiter_gain(emmiter_id, gain);
	}

	void audio_system_positionate_emmiter(uint emmiter_id, glm::vec3 position) {
		audio_frontend_positionate_emmiter(emmiter_id, position);
	}

	void audio_system_move_listener(glm::vec3 new_position) {
		audio_frontend_move_listener(new_position);
	}

	void audio_system_stop_emmiter(uint emmiter_id, uint delay_ms) {
		audio_frontend_stop_emmiter(emmiter_id, delay_ms);
	}

	void audio_system_pause_emmiter(uint emmiter_id, uint delay_ms) {
		audio_frontend_pause_emmiter(emmiter_id, delay_ms);
	}


}