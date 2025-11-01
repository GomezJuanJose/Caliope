#pragma once
#include "defines.h"

namespace caliope {
	typedef struct ring_queue {
		uint length;
		uint stride;
		uint capacity;
		void* block;
		bool owns_memory;
		int head;
		int tail;

	}ring_queue;

	/**
	 * @note capacity is the total number of elements to be available
	 */
	bool ring_queue_create(uint stride, uint capacity, void* memory, ring_queue& out_queue);

	void ring_queue_destroy(ring_queue& queue);

	bool ring_queue_enqueue(ring_queue& queue, void* value);

	bool ring_queue_dequeue(ring_queue& queue, void* out_value);

	bool ring_queue_peek(const ring_queue& queue, void* out_value);
}
