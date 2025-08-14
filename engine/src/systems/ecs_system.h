#pragma once
#include "defines.h"
#include "components/components.inl"

namespace caliope {

	typedef enum archetype {
		ARCHETYPE_SPRITE = 0,
		ARCHETYPE_SPRITE_ANIMATION
	}archetype;

	typedef enum component_id {
		TRANSFORM_COMPONENT = 0,
		MATERIAL_COMPONENT,
		MATERIAL_ANIMATION_COMPONENT
	} component_id;

	bool ecs_system_initialize();
	void ecs_system_shutdown();

	CE_API uint ecs_system_add_entity(archetype archetype);
	CE_API void ecs_system_change_entity(uint entity, archetype archetype);
	CE_API void ecs_system_insert_data(uint entity, component_id component, void* data, uint size_data);
	CE_API void ecs_system_delete_entity(uint entity);

	CE_API void ecs_system_build_archetype(archetype archetype, std::vector<component_id>& components_id, std::vector<uint>& components_sizes);
	CE_API std::vector<std::vector<void*>>& ecs_system_get_archetype_data(archetype archetype);

}