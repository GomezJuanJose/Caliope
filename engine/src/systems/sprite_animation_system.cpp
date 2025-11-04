#include "sprite_animation_system.h"
#include "cepch.h"

#include "core/logger.h"
#include "resources/resources_types.inl"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"

namespace caliope {

	typedef struct sprite_animation_system_state {
		std::unordered_map<std::string, sprite_animation> registered_animations;
	} sprite_animation_system_state;

	static std::unique_ptr<sprite_animation_system_state> state_ptr;

	bool load_sprite_animation(sprite_animation_resource_data& animation_config);

	bool sprite_animation_system_initialize() {
		state_ptr = std::make_unique<sprite_animation_system_state>();

		if (!state_ptr) {
			return false;
		}

		return true;
	}

	void sprite_animation_system_shutdown() {
		state_ptr->registered_animations.clear();
		state_ptr.reset();
	}

	bool sprite_animation_system_register(std::string& name)
	{
		if (state_ptr->registered_animations.find(name) != state_ptr->registered_animations.end()) {
			CE_LOG_ERROR("Animation with this name already exists");
			return false;
		}

		resource r;
		if (!resource_system_load(name, RESOURCE_TYPE_SPRITE_ANIMATION, r)) {
			CE_LOG_ERROR("material_system_adquire couldnt load file material");
			return false;
		}
		sprite_animation_resource_data sprite_anim_data = std::any_cast<sprite_animation_resource_data>(r.data);
		resource_system_unload(r);


		load_sprite_animation(sprite_anim_data);

		return true;
	}



	void sprite_animation_system_unregister(std::string name) {
		if (state_ptr->registered_animations.find(name) != state_ptr->registered_animations.end()) {
			state_ptr->registered_animations.erase(name);
		}
	}

	sprite_frame* sprite_animation_system_acquire_frame(std::string& name, float delta_time) {
		
		if (state_ptr->registered_animations.find(name) == state_ptr->registered_animations.end()) {
			CE_LOG_WARNING("Animation with this name not exists, trying to adquire it : %s", name.c_str());
			sprite_animation_system_register(name);
		}

		sprite_animation& animation = state_ptr->registered_animations[name];

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

		return &animation.frames[animation.current_frame];
	}

	bool load_sprite_animation(sprite_animation_resource_data& animation_config) {
		sprite_animation animation_internal;

		animation_internal.name = std::string(animation_config.name.data());
		for (uint i = animation_config.starting_row; i < animation_config.number_of_rows; ++i) {
			for (uint j = animation_config.starting_column; j < animation_config.number_of_columns; ++j) {
				caliope::sprite_frame frame;
				frame.material_name = animation_config.frames_data[i].material_name;
				frame.texture_region = texture_system_calculate_grid_region_coordinates(*material_system_adquire(frame.material_name)->diffuse_texture, animation_config.frames_data[i].grid_size, i, j);
				animation_internal.frames.push_back(frame);
			}
		}
		animation_internal.is_looping = animation_config.is_looping;
		animation_internal.is_playing = animation_config.is_playing;
		animation_internal.frames_per_second = std::abs(animation_config.frames_per_second);
		animation_internal.accumulated_delta = 0.0f;
		animation_internal.current_frame = 0;


		state_ptr->registered_animations.insert({ animation_internal.name , animation_internal });

		return true;
	}
}
