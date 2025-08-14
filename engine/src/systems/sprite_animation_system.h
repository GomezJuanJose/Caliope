
#include "defines.h"
#include <glm/glm.hpp>

namespace caliope {

	typedef struct sprite_frame {
		std::string material_name;
		std::array<glm::vec2, 4> texture_region;
	};

	typedef struct sprite_animation_config {
		std::string name;
		std::vector<sprite_frame> frames;
		bool is_looping;
		bool is_playing;
		float frames_per_second;
	} sprite_animation_config;

	bool sprite_animation_system_initialize();
	void sprite_animation_system_shutdown();

	CE_API bool sprite_animation_system_register(sprite_animation_config& animation_config);
	CE_API void sprite_animation_system_unregister(std::string animation_name);

	sprite_frame& sprite_animation_system_acquire_frame(std::string& animation_name, float delta_time);
}