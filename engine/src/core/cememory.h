#pragma once

#include "defines.h"

namespace caliope {

	typedef enum memory_tag {
		MEMORY_TAG_UNKNOWN = 0,
		MEMORY_TAG_NEW_OPERATOR,
		MEMORY_TAG_ECS,
		MEMORY_TAG_LOADER,


		MAX_MEMORY_TAGS
	} memory_tag;


	typedef struct memory_system_configuration {
		uint64 total_alloc_size;
	}memory_system_configuration;

	bool memory_system_initialize(memory_system_configuration config);
	void memory_system_shutdown();

	void* allocate_memory(memory_tag tag, uint64 size);
	void free_memory(memory_tag tag, void* block, uint64 size);
	void* zero_memory(void* block, uint64 size);
	void* copy_memory(void* dest, const void* source, uint64 size);
	void* set_memory(void* dest, int value, uint64 size);

	std::string get_memory_stats();
	uint64 get_memory_usage();
	uint64 get_memory_alloc_count();

}