#pragma once

#include "defines.h"

namespace caliope {

	typedef struct freelist {
		void* memory;
	} freelist;

	void freelist_create(uint64 total_size, uint64& memory_requirement, void* memory, freelist& out_list);
	void freelist_destroy(freelist& list);

	bool freelist_allocate_block(freelist& list, uint64 size, uint64& out_offset);
	bool freelist_free_block(freelist& list, uint64 size, uint64 offset);

	bool freelist_resize(freelist& list, uint64& memory_requirement, void* new_memory, uint64 new_size, void*& out_old_memory);

	void freelist_clear(freelist& list);

	uint64 freelist_free_space(freelist& list);
}