#include "sprite_animation_system.h"
#include "cepch.h"

#include "core/logger.h"

namespace caliope {

	typedef struct sprite_animation_internal {
		std::string name;
		std::vector<sprite_frame> frames;
		bool is_looping;
		bool is_playing;
		float frames_per_second;
		float accumulated_delta;
		uint current_frame;
	} sprite_animation_internal;


	typedef struct sprite_animation_system_state {
		std::unordered_map<std::string, sprite_animation_internal> registered_animations;
	} sprite_animation_system_state;

	static std::unique_ptr<sprite_animation_system_state> state_ptr;

	bool sprite_animation_system_initialize() {
		state_ptr = std::make_unique<sprite_animation_system_state>();

		if (!state_ptr) {
			return false;
		}

		return true;
	}

	void sprite_animation_system_shutdown() {
		state_ptr->registered_animations.empty();
		state_ptr.reset();
	}

	bool sprite_animation_system_register(sprite_animation_config& animation_config) {
		if (state_ptr->registered_animations.find(animation_config.name) != state_ptr->registered_animations.end()) {
			CE_LOG_ERROR("Animation with this name already exists");
			return false;
		}

		sprite_animation_internal animation_internal;
		animation_internal.name = animation_config.name;
		animation_internal.frames = animation_config.frames;
		animation_internal.is_looping = animation_config.is_looping;
		animation_internal.is_playing = animation_config.is_playing;
		animation_internal.frames_per_second = std::abs(animation_config.frames_per_second);
		animation_internal.accumulated_delta = 0.0f;
		animation_internal.current_frame = 0;

		state_ptr->registered_animations.insert({ animation_internal.name , animation_internal });

		return true;
	}

	void sprite_animation_system_unregister(std::string animation_name) {
		if (state_ptr->registered_animations.find(animation_name) != state_ptr->registered_animations.end()) {
			state_ptr->registered_animations.erase(animation_name);
		}
	}

	sprite_frame& sprite_animation_system_acquire_frame(std::string& animation_name, float delta_time) {
		if (state_ptr->registered_animations.find(animation_name) == state_ptr->registered_animations.end()) {
			CE_LOG_ERROR("Animation with this name not exists");
			return sprite_frame();
		}

		sprite_animation_internal& animation = state_ptr->registered_animations[animation_name];

		if (animation.is_playing) {
			animation.accumulated_delta += delta_time;
			if (animation.accumulated_delta > 1 / animation.frames_per_second) {
				animation.accumulated_delta = 0.0f;
				animation.current_frame++;

				if (animation.is_looping) {
					animation.current_frame %= animation.frames.size();
				}
				else if(animation.current_frame >= animation.frames.size()){
					animation.current_frame = animation.frames.size() - 1;
				}
			}
		}

		return animation.frames[animation.current_frame];
	}
}
