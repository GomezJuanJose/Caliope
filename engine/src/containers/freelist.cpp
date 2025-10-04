#include "freelist.h"

#include "core/cememory.h"
#include "core/logger.h"

namespace caliope {
	typedef struct freelist_node {
		uint64 offset;
		uint64 size;
		struct freelist_node* next;
	} freelist_node;

	typedef struct internal_state {
		uint64 total_size;
		uint64 max_entries;
		freelist_node* head;
		freelist_node* nodes;
	} internal_state;

	freelist_node* get_node(freelist& list);
	void return_node(freelist_node* node);

	void freelist_create(uint64 total_size, uint64& memory_requirement, void* memory, freelist& out_list) {
		uint64 max_entries = (total_size / (sizeof(void*) * sizeof(freelist_node)));
		memory_requirement = sizeof(internal_state) + (sizeof(freelist_node) * max_entries);
		if (!memory) {
			return;
		}

		uint64 mem_min = (sizeof(internal_state) + sizeof(freelist_node)) * 8;
		if (total_size < mem_min) {
			CE_LOG_WARNING("Freelist are very inefficient with amounts of memory less than %iB; it is recommended to not use this structure in this case.", mem_min);
		}

		out_list.memory = memory;

		// The block's layout in head* first, the array of available nodes.
		zero_memory(out_list.memory, memory_requirement);
		internal_state* state = static_cast<internal_state*>(out_list.memory);
		char* direction_number = ((char*)out_list.memory) + sizeof(internal_state);
		void* raw_direction = (void*)direction_number;
		state->nodes = static_cast<freelist_node*>(raw_direction);
		state->max_entries = max_entries;
		state->total_size = total_size;

		state->head = &state->nodes[0];
		state->head->offset = 0;
		state->head->size = total_size;
		state->head->next = 0;

		// Invalidate the offset and size for all but the first node.
		for (uint64 i = 1; i < state->max_entries; ++i) {
			state->nodes[i].offset = INVALID_ID;
			state->nodes[i].size = INVALID_ID;
		}
	}

	void freelist_destroy(freelist& list) {
		if (list.memory) {
			// Zero out the memory before giving it back.
			internal_state* state = static_cast<internal_state*>(list.memory);
			zero_memory(list.memory, sizeof(internal_state) + sizeof(freelist_node) * state->max_entries);
			list.memory = 0;
		}
	}

	bool freelist_allocate_block(freelist& list, uint64 size, uint64& out_offset) {
		if (!list.memory) {
			return false;
		}

		internal_state* state = static_cast<internal_state*>(list.memory);
		freelist_node* node = state->head;
		freelist_node* previous = 0;
		while (node) {
			if (node->size == size) {
				//Exact match. Just return the node.
				out_offset = node->offset;
				freelist_node* node_to_return = 0;
				if (previous) {
					previous->next = node->next;
					node_to_return = node;
				}
				else {
					// This node is the head of the list.
					node_to_return = state->head;
					state->head = node->next;
				}
				return_node(node_to_return);
				return true;
			} else if (node->size > size) {
				// Node is larger. Deduct the memory from it and move the offset by that amount.
				out_offset = node->offset;
				node->size -= size;
				node->offset += size;
				return true;
			}

			previous = node;
			node = node->next;
		}

		uint64 free_space = freelist_free_space(list);
		CE_LOG_WARNING("freelist_find_block, no block with enough free space found (requested: %uB, available: %lluB)", size, free_space);
		return false;
	}

	bool freelist_free_block(freelist& list, uint64 size, uint64 offset) {
		if (!list.memory || !size) {
			return false;
		}

		internal_state* state = static_cast<internal_state*>(list.memory);
		freelist_node* node = state->head;
		freelist_node* previous = 0;
		if (!node) {
			// Check for the case where the entire thing is allocated.
			freelist_node* new_node = get_node(list);
			new_node->offset = offset;
			new_node->size = size;
			new_node->next = 0;
			state->head = new_node;

			return true;
		}
		else {
			while (node) {
				if (node->offset == offset) {
					// Can just be appended to this node.
					node->size += size;

					// Check if can combine the next node, and the return the second node.
					if (node->next && node->next->offset == node->offset + node->size) {
						node->size += node->next->size;
						freelist_node* next = node->next;
						node->next = node->next->next;
						return_node(next);
					}
					return true;
				}
				else if (node->offset > offset) {
					// Iterated beyond the space to be freed. Need a new node
					freelist_node* new_node = get_node(list);
					new_node->offset = offset;
					new_node->size = size;

					// If there is a previous node, the new node should be inserted between this and it.
					if (previous) {
						previous->next = new_node;
						new_node->next = node;
					}
					else {
						new_node->next = node;
						state->head = new_node;
					}

					if (new_node->next && new_node->offset + new_node->size == new_node->next->offset) {
						new_node->size += new_node->next->size;
						freelist_node* rubbish = new_node->next;
						new_node->next = rubbish->next;
						return_node(rubbish);
					}

					if (previous && previous->offset + previous->size == new_node->offset) {
						previous->size += new_node->size;
						freelist_node* rubbish = new_node;
						previous->next = rubbish->next;
						return_node(rubbish);
					}

					return true;
				}

				previous = node;
				node = node->next;
			}
		}

		CE_LOG_WARNING("Unable to find block to be freed. Corruption possible?");
		return false;
	}

	bool freelist_resize(freelist& list, uint64& memory_requirement, void* new_memory, uint64 new_size, void*& out_old_memory) {
		if (!memory_requirement || ((internal_state*)list.memory)->total_size > new_size) {
			return false;
		}
		
		// Enought space to hold state, plus array for all nodes.
		uint64 max_entries = (new_size / (sizeof(void*) * sizeof(freelist_node)));
		memory_requirement = sizeof(internal_state) + (sizeof(freelist_node) * max_entries);
		if (!new_memory) {
			return true;
		}

		// Assign the old memory pointer so it can be freed.
		out_old_memory = list.memory;

		// Copy over the old state to the new.
		internal_state* old_state = (internal_state*)list.memory;
		uint64 size_diff = new_size - old_state->total_size;

		list.memory = new_memory;

		zero_memory(list.memory, memory_requirement);

		internal_state* state = (internal_state*)list.memory;
		state->nodes = static_cast<freelist_node*>((void*)(((char*)list.memory) + sizeof(internal_state)));
		state->max_entries = max_entries;
		state->total_size = new_size;

		for (uint64 i = 1; i < state->max_entries; ++i) {
			state->nodes[i].offset = INVALID_ID;
			state->nodes[i].size = INVALID_ID;
		}

		state->head = &state->nodes[0];

		freelist_node* new_list_node = state->head;
		freelist_node* old_node = old_state->head;
		if (!old_node) {
			state->head->offset = old_state->total_size;
			state->head->size = size_diff;
			state->head->next = 0;
		}
		else {
			while (old_node) {
				freelist_node* new_node = get_node(list);
				new_node->offset = old_node->offset;
				new_node->size = old_node->size;
				new_node->next = 0;
				new_list_node->next = new_node;
				// Move to the next entry.
				new_list_node = new_list_node->next;

				if (old_node->next) {
					old_node = old_node->next;
				}
				else {
					if (old_node->offset + old_node->size == old_state->total_size) {
						new_node->size += size_diff;
					}
					else {
						freelist_node* new_node_end = get_node(list);
						new_node_end->offset = old_state->total_size;
						new_node_end->size = size_diff;
						new_node_end->next = 0;
						new_node->next = new_node_end;
					}

					break;
				}
			}
		}

		return true;
	}

	void freelist_clear(freelist& list) {
		if (!list.memory) {
			return;
		}

		internal_state* state = static_cast<internal_state*>(list.memory);
		for (uint64 i = 1; i < state->max_entries; ++i) {
			state->nodes[i].offset = INVALID_ID;
			state->nodes[i].size = INVALID_ID;
		}

		state->head->offset = 0;
		state->head->size = state->total_size;
		state->head->next = 0;
	}

	uint64 freelist_free_space(freelist& list) {
		if (!list.memory) {
			return 0;
		}

		uint64 running_total = 0;
		internal_state* state = static_cast<internal_state*>(list.memory);
		freelist_node* node = state->head;
		while (node) {
			running_total += node->size;
			node = node->next;
		}

		return running_total;
	}

	freelist_node* get_node(freelist& list) {
		internal_state* state = static_cast<internal_state*>(list.memory);
		for (uint64 i = 1; i < state->max_entries; ++i) {
			if (state->nodes[i].offset == INVALID_ID) {
				return &state->nodes[i];
			}
		}

		return 0;
	}

	void return_node(freelist_node* node) {
		node->offset = INVALID_ID;
		node->size = INVALID_ID;
		node->next = 0;
	}
}