#include "ecs_system.h"
#include "core/cememory.h"
#include "core/logger.h"

#include "cepch.h"

namespace caliope {


	typedef struct ecs_entity_entry {
		archetype archetype;
		uint index;
	} ecs_entity_entry;

	typedef struct archetype_data {
		std::vector<std::vector<void*>> component_pool;
		std::vector<uint> component_sizes;
		std::unordered_map<component_id, uint> components_tracker; // Has the information about where is stored each component
		uint entity_count;
	} archetype_data;

	typedef struct ecs_system_state {
		std::vector<archetype_data> archetypes;
		std::unordered_map<uint, ecs_entity_entry> entities_tracker; // Has the information about the archetype of the component and where is stored
		std::stack<uint> reusable_entities_pool;
		uint system_entities_count;

	} ecs_system_state;

	static std::unique_ptr<ecs_system_state> state_ptr;

	bool ecs_system_initialize() {
		state_ptr = std::make_unique<ecs_system_state>();
		
		if (state_ptr == nullptr) {
			return false;
		}
		
		// Builtin archetypes
		std::vector<uint> new_archetype_size = { sizeof(transform_component), sizeof(material_component) };
		std::vector<component_id> new_archetype_id = {TRANSFORM_COMPONENT, MATERIAL_COMPONENT};
		ecs_system_build_archetype(ARCHETYPE_SPRITE, new_archetype_id, new_archetype_size);

		new_archetype_size = { sizeof(transform_component), sizeof(material_animation_component) };
		new_archetype_id = { TRANSFORM_COMPONENT, MATERIAL_ANIMATION_COMPONENT };
		ecs_system_build_archetype(ARCHETYPE_SPRITE_ANIMATION, new_archetype_id, new_archetype_size);

		CE_LOG_INFO("ECS system initialized.");

		return true;
	}

	void ecs_system_shutdown() {
		for (uint i = 0; i < state_ptr->archetypes.size(); ++i) {

			if (state_ptr->archetypes[i].entity_count == 0) {
				continue;
			}

			for (uint j = 0; j < state_ptr->archetypes[i].component_sizes.size(); ++j) {
				uint block_size = state_ptr->archetypes[i].entity_count * state_ptr->archetypes[i].component_sizes[j];	
				void* block = state_ptr->archetypes[i].component_pool[j].data();
				free_memory(MEMORY_TAG_ECS, block, block_size);
			}
		}

		state_ptr->archetypes.empty();
		state_ptr.reset();
	}
	
	uint ecs_system_add_entity(archetype archetype) {
		for (uint i = 0; i < state_ptr->archetypes[archetype].component_pool.size(); i++) {
			uint component_size = state_ptr->archetypes[archetype].component_sizes[i];
			void* component_blocK = allocate_memory(MEMORY_TAG_ECS, component_size);
			zero_memory(component_blocK, component_size);

			state_ptr->archetypes[archetype].component_pool[i].push_back(component_blocK);
		}

		ecs_entity_entry entity_entry;
		uint id_entity = 0;
		
		// TODO: Try to avoid conditions
		if (state_ptr->reusable_entities_pool.empty()) {
			id_entity = state_ptr->system_entities_count;
			state_ptr->system_entities_count++;
		}
		else {
			id_entity = state_ptr->reusable_entities_pool.top();
			state_ptr->reusable_entities_pool.pop();
		}

		entity_entry.archetype = archetype;
		entity_entry.index = state_ptr->archetypes[archetype].entity_count;
		state_ptr->archetypes[archetype].entity_count++;

		state_ptr->entities_tracker.insert({ id_entity, entity_entry });
		
		return id_entity;
	}

	void ecs_system_change_entity(uint entity, archetype archetype) {
		ecs_system_delete_entity(entity);
		ecs_system_add_entity(archetype);
	}

	void ecs_system_insert_data(uint entity, component_id component, void* data, uint size_data) {
		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);

		if (state_ptr->archetypes[entity_entry.archetype].components_tracker.find(component) == state_ptr->archetypes[entity_entry.archetype].components_tracker.end()) {
			CE_LOG_WARNING("ecs_system_insert_data component not found in the entity %s", entity);
			return;
		}

		uint component_index = state_ptr->archetypes[entity_entry.archetype].components_tracker.at(component);

		copy_memory(state_ptr->archetypes[entity_entry.archetype].component_pool[component_index][entity_entry.index], data, size_data);
	}

	void ecs_system_delete_entity(uint entity) {
		// TODO: Make the erase in O(1)
		// TODO: Refactor this to avoid to use this if and still if the user enters a invalid id do not crash
		if (state_ptr->entities_tracker.find(entity) == state_ptr->entities_tracker.end()) {
			return;
		}

		state_ptr->reusable_entities_pool.push(entity);

		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);
		archetype archetype = entity_entry.archetype;
		uint entity_index = entity_entry.index;

		for (uint i = 0; i < state_ptr->archetypes[archetype].component_pool.size(); i++) {

			uint component_size = state_ptr->archetypes[archetype].component_sizes[i];
			void* block = state_ptr->archetypes[archetype].component_pool[i][entity_index];
			free_memory(MEMORY_TAG_ECS, block, component_size);

			state_ptr->archetypes[archetype].component_pool[i].erase(state_ptr->archetypes[archetype].component_pool[i].begin() + entity_index);
		}
		state_ptr->archetypes[archetype].entity_count--;

		state_ptr->entities_tracker.erase(entity);
		state_ptr->system_entities_count--;
	}

	void ecs_system_build_archetype(archetype archetype, std::vector<component_id>& components_id, std::vector<uint>& components_sizes) {
		archetype_data arch_data;
		for (uint i = 0; i < components_sizes.size(); ++i) {
			arch_data.component_sizes.push_back(components_sizes[i]);
			arch_data.components_tracker.insert({ components_id[i], i });
			
			std::vector<void*> component_pool;
			arch_data.component_pool.push_back(component_pool);

		}
		arch_data.entity_count = 0;
		state_ptr->archetypes.insert(state_ptr->archetypes.begin() + archetype, arch_data);
	}

	std::vector<std::vector<void*>>& ecs_system_get_archetype_data(archetype archetype) {
		return state_ptr->archetypes[archetype].component_pool;
	}
}