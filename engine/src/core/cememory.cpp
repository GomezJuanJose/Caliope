#include "cememory.h"

#include "core/logger.h"
#include "platform/platform.h"

#include "memory/dynamic_allocator.h"

namespace caliope {
	typedef struct memory_system_state {
		memory_system_configuration config;
		uint64 alloc_count;
		uint64 allocator_memory_requirement;
		dynamic_allocator allocator;
		void* allocator_block;

		int memory_stats[MAX_MEMORY_TAGS];

		uint64 total_usage;
	} memory_system_state;

	static memory_system_state* state_ptr;

	bool memory_system_initialize(memory_system_configuration config) {
		uint64 state_memory_requirement = sizeof(memory_system_state);

		uint64 alloc_requirement = 0;
		dynamic_allocator_create(config.total_alloc_size, alloc_requirement, 0, state_ptr->allocator);

		void* block = platform_system_allocate_memory(state_memory_requirement + alloc_requirement);
		if (!block) {
			CE_LOG_FATAL("Memory system allocation failed and the system cannot continue.");
		}

		state_ptr = (memory_system_state*)block;
		state_ptr->config = config;
		state_ptr->total_usage = 0;
		state_ptr->alloc_count = 0;
		state_ptr->allocator_memory_requirement = alloc_requirement;

		state_ptr->allocator_block = (((char*)(void*)block) + state_memory_requirement);
		zero_memory(state_ptr->memory_stats, sizeof(int) * MAX_MEMORY_TAGS);

		if (!dynamic_allocator_create(
			config.total_alloc_size,
			state_ptr->allocator_memory_requirement,
			state_ptr->allocator_block,
			state_ptr->allocator
		)) {
			CE_LOG_FATAL("Memory system is unable to setup internal allocator. Application cannot continue.");
			return false;
		}

		CE_LOG_INFO("Memory system successfully allocated %llu bytes.", config.total_alloc_size);
		return true;
	}

	void memory_system_shutdown() {
		CE_LOG_INFO(get_memory_stats().c_str());
		CE_LOG_INFO("Total usage of memory: %.2fMb/%.2fMb", get_memory_usage() / 1024.0 / 1024.0, state_ptr->config.total_alloc_size / 1024.0 / 1024.0);

		if (state_ptr) {
			dynamic_allocator_destroy(state_ptr->allocator);
			platform_system_free_memory(state_ptr);
		}
		state_ptr = nullptr;
	}

	void* allocate_memory(memory_tag tag, uint64 size) {
		void* block = 0;
		if (state_ptr != nullptr) {
			state_ptr->total_usage += size;
			state_ptr->memory_stats[tag] += size;
			block = dynamic_allocator_allocate(state_ptr->allocator, size);
		}
		else {
			CE_LOG_WARNING("allocate_memory called when the memory system is uninitialize, calling SO to allocate.");
			block = platform_system_allocate_memory(size);
		}

		if (block) {
			platform_system_zero_memory(block, size);
			return block;
		}

		CE_LOG_FATAL("allocate_memory failed to allocate.");
		return nullptr;
	}

	void free_memory(memory_tag tag, void* block, uint64 size) {
		if (state_ptr != nullptr) {
			state_ptr->total_usage -= size;
			state_ptr->memory_stats[tag] -= size;
			bool result = dynamic_allocator_free(state_ptr->allocator, block, size);
			if (!result) {
				platform_system_free_memory(block);
			}
		}
		else {
			CE_LOG_WARNING("free_memory called when the memory system is uninitialize, calling SO to free.");
			platform_system_free_memory(block);
		}
	}

	void* zero_memory(void* block, uint64 size) {
		return platform_system_zero_memory(block, size);
	}

	void* copy_memory(void* dest, const void* source, uint64 size) {
		return platform_system_copy_memory(dest, source, size);
	}

	void* set_memory(void* dest, int value, uint64 size) {
		return platform_system_set_memory(dest, value, size);
	}

	std::string get_memory_stats() {
		std::string stats = "\n";

		std::string string_tags[] = {
			"MEMORY_TAG_UNKNOWN         ",
			"MEMORY_TAG_NEW_OPERATOR    ",
			"MEMORY_TAG_ECS             ",
			"MEMORY_TAG_LOADERS         ",
			"MEMORY_TAG_RING_QUEUE      ",
			"MEMORY_TAG_JOB             "
		};

		for (int i = 0; i < MAX_MEMORY_TAGS; ++i) {
			stats += (string_tags[i] + std::to_string(state_ptr->memory_stats[i] / 1024.0 / 1024.0) + "Mb \n");
		}

		return stats;
	}

	uint64 get_memory_usage() {
		return state_ptr->total_usage;
	}

	uint64 get_memory_alloc_count() {
		if (state_ptr) {
			return state_ptr->alloc_count;
		}
		
		return 0;
	}
}

void* operator new (size_t size) {
	return caliope::allocate_memory(caliope::MEMORY_TAG_NEW_OPERATOR, size);
}

void operator delete(void* memory, size_t size) {
	caliope::free_memory(caliope::MEMORY_TAG_NEW_OPERATOR, memory, size);
}