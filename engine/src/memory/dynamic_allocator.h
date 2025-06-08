#pragma once

#include "defines.h"

namespace caliope {
	typedef struct dynamic_allocator {
		void* memory;
	} dynamic_allocator;

	bool dynamic_allocator_create(uint64 total_size, uint64& memory_requirement, void* memory, dynamic_allocator& out_allocator);
	bool dynamic_allocator_destroy(dynamic_allocator& allocator);

	void* dynamic_allocator_allocate(dynamic_allocator& allocator, uint64 size);
	bool dynamic_allocator_free(dynamic_allocator& allocator, void* block, uint64 size);
	uint64 dynamic_allocator_free_space(dynamic_allocator& allocator);
}