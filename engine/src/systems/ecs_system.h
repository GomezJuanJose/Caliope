#pragma once
#include "defines.h"
#include "components/components.inl"

namespace caliope {

	typedef enum archetype {
		ARCHETYPE_SPRITE = 0,
		ARCHETYPE_SPRITE_ANIMATION,
		ARCHETYPE_POINT_LIGHT,

		ARCHETYPE_UI_IMAGE,
		ARCHETYPE_UI_BUTTON,
		ARCHETYPE_UI_TEXT_BOX,
		ARCHETYPE_UI_CONTAINER_BOX
	}archetype;

	typedef enum component_id {
		TRANSFORM_COMPONENT = 0,
		MATERIAL_COMPONENT,
		MATERIAL_ANIMATION_COMPONENT,
		SOUND_EMMITER_COMPONENT,
		POINT_LIGHT_COMPONENT,
		PARENT_COMPONENT,

		UI_TRANSFORM_COMPONENT,
		UI_MATERIAL_COMPONENT,
		UI_DYNAMIC_MATERIAL_COMPONENT,
		UI_MOUSE_EVENTS_COMPONENT,
		UI_TEXT_COMPONENT,
		UI_BEHAVIOUR_COMPONENT,
		UI_CONTAINER_COMPONENT,
	} component_id;

	typedef enum component_data_type {
		COMPONENT_DATA_TYPE_STRING = 0,
		COMPONENT_DATA_TYPE_VEC4,
		COMPONENT_DATA_TYPE_VEC3,
		COMPONENT_DATA_TYPE_VEC2,
		COMPONENT_DATA_TYPE_UINT,
		COMPONENT_DATA_TYPE_FLOAT
	} component_data_type;

	bool ecs_system_initialize();
	void ecs_system_shutdown();

	CE_API uint ecs_system_add_entity(archetype archetype);
	CE_API void ecs_system_change_entity(uint entity, archetype archetype);
	CE_API void ecs_system_insert_data(uint entity, component_id component, void* data);
	CE_API void ecs_system_delete_entity(uint entity);
	CE_API void ecs_system_enable_entity(uint entity, bool enabled);

	/*
	 *  @param components_data_types Defines the data types wich compouse the component, its needed to parse the components data to a file. NOTE: Must be in the same order as the structure!
	 */
	CE_API void ecs_system_build_archetype(archetype archetype, std::vector<component_id>& components_id, std::vector<uint>& components_sizes, std::vector<std::vector<component_data_type>>& components_data_types);

	CE_API std::vector<uint>& ecs_system_get_entities_by_archetype(archetype archetype);
	CE_API void* ecs_system_get_component_data(uint entity, component_id component, uint64& out_component_size);
	CE_API std::vector<component_id>& ecs_system_get_entity_components(uint entity);
	CE_API archetype ecs_system_get_entity_archetype(uint entity);
	CE_API std::vector<component_data_type>& ecs_system_get_component_data_types(component_id component);
}