
#include "defines.h"
#include <glm/glm.hpp>

namespace caliope {


	struct sprite_frame;

	bool sprite_animation_system_initialize();
	void sprite_animation_system_shutdown();

	CE_API bool sprite_animation_system_register(std::string& name);
	CE_API void sprite_animation_system_unregister(std::string name);

	sprite_frame* sprite_animation_system_acquire_frame(std::string& name, float delta_time);
}