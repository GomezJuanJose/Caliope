#include "dynamic_allocator.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "containers/freelist.h"


namespace caliope {
	typedef struct dynamic_allocator_state {
		uint64 total_size;
		freelist list;
		void* freelist_block;
		void* memory_block;
	} dynamic_allocator_state;


	bool dynamic_allocator_create(uint64 total_size, uint64& memory_requirement, void* memory, dynamic_allocator& out_allocator) {
		if (total_size < 1) {
			CE_LOG_ERROR("dynamic_allocator_create cannot have a total_size of 0. Create failed.");
			return false;
		}

		uint64 freelist_requirement = 0;
		dynamic_allocator_state* state = nullptr;
		freelist_create(total_size, freelist_requirement, 0, state->list);

		memory_requirement = freelist_requirement + sizeof(dynamic_allocator_state) + total_size;
		if (!memory) {
			return true;
		}

		out_allocator.memory = memory;
		state = static_cast<dynamic_allocator_state*>(out_allocator.memory);
		state->total_size = total_size;
		state->freelist_block = (void*)(((char*)out_allocator.memory) + sizeof(dynamic_allocator_state));
		state->memory_block = (void*)(((char*)state->freelist_block) + freelist_requirement);

		freelist_create(total_size, freelist_requirement, state->freelist_block, state->list);

		zero_memory(state->memory_block, total_size);
		return true;
	}

	bool dynamic_allocator_destroy(dynamic_allocator& allocator) {
		dynamic_allocator_state* state = static_cast<dynamic_allocator_state*>(allocator.memory);
		freelist_destroy(state->list);
		zero_memory(state->memory_block, state->total_size);
		state->total_size = 0;
		allocator.memory = 0;
		return true;
	}

	void* dynamic_allocator_allocate(dynamic_allocator& allocator, uint64 size) {
		if (size) {
			dynamic_allocator_state* state = static_cast<dynamic_allocator_state*>(allocator.memory);
			uint64 offset = 0;
			if (freelist_allocate_block(state->list, size, offset)) {
				void* block = (void*)(((char*)state->memory_block) + offset);
				return block;
			}
			else {
				CE_LOG_ERROR("dynamic_allocator_allocate no blocks of memory large enough to allocate from.");
				uint64 available = freelist_free_space(state->list);
				CE_LOG_ERROR("Requestd size: %llu, total space available: %llu", size, available);
				return 0;
			}
		}
		
		CE_LOG_ERROR("dynamic_allocator_allocate requires a valid allocator and size.");
		return nullptr;
	}

	bool dynamic_allocator_free(dynamic_allocator& allocator, void* block, uint64 size) {
		if (!block || !size) {
			CE_LOG_ERROR("dynamic_allocator_free requires both a valid allocator (0x%p) and a block (0x%p) to be freed.", allocator, block);
			return false;
		}

		dynamic_allocator_state* state = static_cast<dynamic_allocator_state*>(allocator.memory);
		if (block < state->memory_block || block > ((char*)state->memory_block) + state->total_size) {
			void* end_of_block = (void*)(((char*)state->memory_block) + state->total_size);
			CE_LOG_ERROR("dynamic_allocator_free trying to release block (0x%p) outside of allocator range (0x%p)-(0x%p)", block, state->memory_block, end_of_block);
			return false;
		}

		uint64 offset = (((char*)block) - state->memory_block);
		if (!freelist_free_block(state->list, size, offset)) {
			CE_LOG_ERROR("dynamic_allocator_free failed.");
			return false;
		}

		return true;
	}

	uint64 dynamic_allocator_free_space(dynamic_allocator& allocator) {
		dynamic_allocator_state* state = static_cast<dynamic_allocator_state*>(allocator.memory);
		return freelist_free_space(state->list);
	}
}