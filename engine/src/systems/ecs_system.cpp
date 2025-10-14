#include "ecs_system.h"
#include "core/cememory.h"
#include "core/logger.h"

#include "cepch.h"

namespace caliope {


	typedef struct ecs_entity_entry {
		archetype archetype;
		uint component_index; // Index corresponding to their data on component_pool
		bool is_enabled;
	} ecs_entity_entry;

	typedef struct archetype_data {
		std::vector<std::vector<void*>> component_pool;
		std::vector<uint> component_sizes;
		std::vector<component_id> components_tracker; // Has the information about where is stored each component
		uint entity_count;
		//std::unordered_map<component_id, uint> components_tracker; // Has the information about where is stored each component
	} archetype_data;

	typedef struct ecs_system_state {
		std::vector<archetype_data> archetypes;
		std::unordered_map<uint, ecs_entity_entry> entities_tracker; // Has the information about the archetype of the component and where is stored
		std::unordered_map<archetype, std::vector<uint>> entites_grouped_by_archetypes;			// TODO: Rework
		std::unordered_map<archetype, std::vector<uint>> enabled_entites_grouped_by_archetypes;
		std::unordered_map<archetype, std::vector<uint>> disabled_entites_grouped_by_archetypes;

		std::stack<uint> reusable_entities_pool;
		uint system_entities_count;

	} ecs_system_state;

	static std::unique_ptr<ecs_system_state> state_ptr;

	//TODO: DRY the operation of delete from an vector, try to place the last element into the deleted space or make the vectors to static arrays and make the data without holes between data and iterate until a invalid data found
	// or better, a vector but only increase its size when adding entities or deleting. The rest of operations would be threated as a static array ( and compact the data in the array)

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

		new_archetype_size = { sizeof(transform_component), sizeof(point_light_component) };
		new_archetype_id = { TRANSFORM_COMPONENT, POINT_LIGHT_COMPONENT };
		ecs_system_build_archetype(ARCHETYPE_POINT_LIGHT, new_archetype_id, new_archetype_size);

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

		state_ptr->archetypes.clear();
		state_ptr->entites_grouped_by_archetypes.clear();
		state_ptr->enabled_entites_grouped_by_archetypes.clear();
		state_ptr->disabled_entites_grouped_by_archetypes.clear();
		state_ptr->reusable_entities_pool = std::stack<uint>();
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
		entity_entry.component_index = state_ptr->archetypes[archetype].entity_count;
		entity_entry.is_enabled = true;
		state_ptr->archetypes[archetype].entity_count++;
		state_ptr->entites_grouped_by_archetypes[archetype].push_back(id_entity);
		state_ptr->enabled_entites_grouped_by_archetypes[archetype].push_back(id_entity);

		state_ptr->entities_tracker.insert({ id_entity, entity_entry });
		
		return id_entity;
	}

	void ecs_system_change_entity(uint entity, archetype archetype) {
		ecs_system_delete_entity(entity);
		ecs_system_add_entity(archetype);
	}

	void ecs_system_insert_data(uint entity, component_id component, void* data) {
		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);

		/*/if (state_ptr->archetypes[entity_entry.archetype].components_tracker.find(component) == state_ptr->archetypes[entity_entry.archetype].components_tracker.end()) {
			CE_LOG_WARNING("ecs_system_insert_data component not found in the entity %s", entity);
			return;
		}*/
		uint component_index = -1;
		std::vector<component_id> components = state_ptr->archetypes[entity_entry.archetype].components_tracker;
		for (uint i = 0; i < components.size(); ++i) {
			if (components[i] == component) {
				component_index = i;
			}
		}

		if (component_index == -1) {
			return;
		}

		//uint component_index = state_ptr->archetypes[entity_entry.archetype].components_tracker.at(component);

		uint size_data = state_ptr->archetypes[entity_entry.archetype].component_sizes[component_index];
		copy_memory(state_ptr->archetypes[entity_entry.archetype].component_pool[component_index][entity_entry.component_index], data, size_data);
	}

	void ecs_system_delete_entity(uint entity) {
		// TODO: Make the erase in O(1)
		// TODO: Refactor this to avoid to use this if and still if the user enters a invalid id do not crash


		if (state_ptr->entities_tracker.find(entity) == state_ptr->entities_tracker.end()) {
			return;
		}

		state_ptr->reusable_entities_pool.push(entity);

		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);
		archetype archtype = entity_entry.archetype;
		uint entity_index = entity_entry.component_index;

		std::unordered_map<archetype, std::vector<uint>>& entites_grouped_by_archetypes = entity_entry.is_enabled ? state_ptr->enabled_entites_grouped_by_archetypes : state_ptr->disabled_entites_grouped_by_archetypes;;

		// Get last entity of the archetype to replace the erased entity
		uint last_entity = state_ptr->entites_grouped_by_archetypes.at(entity_entry.archetype).back();
		ecs_entity_entry& last_entity_entry = state_ptr->entities_tracker.at(last_entity);

		for (uint i = 0; i < state_ptr->archetypes[archtype].component_pool.size(); i++) {

			uint component_size = state_ptr->archetypes[archtype].component_sizes[i];
			void* block = state_ptr->archetypes[archtype].component_pool[i][entity_index];
			free_memory(MEMORY_TAG_ECS, block, component_size);

			// Replace the erased entity pointer with the pointer to the last entity
			// There is a secondary effect wich is the order of the entities may vary wich at the time of render for example affects the order (thats why exists z-order)
			state_ptr->archetypes[archtype].component_pool[i][entity_index] = state_ptr->archetypes[archtype].component_pool[i][last_entity_entry.component_index];
			state_ptr->archetypes[archtype].component_pool[i].pop_back();

		}
		last_entity_entry.component_index = entity_index;

		state_ptr->archetypes[archtype].entity_count--;

		// TODO: Rework
		for (uint i = 0; i < entites_grouped_by_archetypes.at(entity_entry.archetype).size(); ++i) {
			if (entites_grouped_by_archetypes.at(entity_entry.archetype)[i] == entity) {
				entites_grouped_by_archetypes[archtype].erase(entites_grouped_by_archetypes[archtype].begin() + i);
				break;
			}
		}

		for (uint i = 0; i < state_ptr->entites_grouped_by_archetypes.at(entity_entry.archetype).size(); ++i) {
			if (state_ptr->entites_grouped_by_archetypes.at(entity_entry.archetype)[i] == entity) {
				state_ptr->entites_grouped_by_archetypes[archtype].erase(state_ptr->entites_grouped_by_archetypes[archtype].begin() + i);
				break;
			}
		}
		// -----------

		state_ptr->entities_tracker.erase(entity);
		state_ptr->system_entities_count--;
	}

	// TODO: Rework(O(1) After deciding if use vectors or arrays)
	void ecs_system_enable_entity(uint entity, bool enabled) {
		if (state_ptr->entities_tracker.find(entity) == state_ptr->entities_tracker.end()) {
			return;
		}

		ecs_entity_entry& entity_entry = state_ptr->entities_tracker.at(entity);
		std::unordered_map<archetype, std::vector<uint>>& entites_grouped_by_archetypes_add = enabled ? state_ptr->enabled_entites_grouped_by_archetypes : state_ptr->disabled_entites_grouped_by_archetypes;
		std::unordered_map<archetype, std::vector<uint>>& entites_grouped_by_archetypes_remove = !enabled ? state_ptr->enabled_entites_grouped_by_archetypes : state_ptr->disabled_entites_grouped_by_archetypes;

		for (uint i = 0; i < entites_grouped_by_archetypes_remove[entity_entry.archetype].size(); ++i) {
			if (entites_grouped_by_archetypes_remove.at(entity_entry.archetype)[i] == entity) {
				entites_grouped_by_archetypes_remove.at(entity_entry.archetype).erase(entites_grouped_by_archetypes_remove.at(entity_entry.archetype).begin() + i);
				
				entites_grouped_by_archetypes_add[entity_entry.archetype].insert(entites_grouped_by_archetypes_add.at(entity_entry.archetype).begin(), entity); // Important to insert at the begining because when deleted takes the last element of the array and if its the same the component_id its not updated for the real last element that will ocuppies the position of the deleted entity
			}
		}
		

		entity_entry.is_enabled = enabled;
	}

	void ecs_system_build_archetype(archetype archetype, std::vector<component_id>& components_id, std::vector<uint>& components_sizes) {
		archetype_data arch_data;
		for (uint i = 0; i < components_sizes.size(); ++i) {
			arch_data.component_sizes.push_back(components_sizes[i]);
			//arch_data.components_tracker.insert({ components_id[i], i });
			arch_data.components_tracker.push_back(components_id[i]);
			
			std::vector<void*> component_pool;
			arch_data.component_pool.push_back(component_pool);

		}
		arch_data.entity_count = 0;
		state_ptr->archetypes.insert(state_ptr->archetypes.begin() + archetype, arch_data);
	}

	std::vector<uint>& ecs_system_get_entities_by_archetype(archetype archetype){
		return state_ptr->enabled_entites_grouped_by_archetypes[archetype];
	}

	void* ecs_system_get_component_data(uint entity, component_id component, uint64& out_component_size){
		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);
		//uint component_index = state_ptr->archetypes[entity_entry.archetype].components_tracker.at(component);
		uint component_index = -1;
		std::vector<component_id> components = state_ptr->archetypes[entity_entry.archetype].components_tracker;
		for (uint i = 0; i < components.size(); ++i) {
			if (components[i] == component) {
				component_index = i;
			}
		}
		if (component_index == -1) {
			return 0;
		}
		
		out_component_size = state_ptr->archetypes[entity_entry.archetype].component_sizes[component_index];

		return state_ptr->archetypes[entity_entry.archetype].component_pool[component_index][entity_entry.component_index];
	}

	std::vector<component_id>& ecs_system_get_entity_components(uint entity) {
		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);
		return state_ptr->archetypes[entity_entry.archetype].components_tracker;
	}
	 archetype ecs_system_get_entity_archetype(uint entity) {
		ecs_entity_entry entity_entry = state_ptr->entities_tracker.at(entity);
		return entity_entry.archetype;
	}
}