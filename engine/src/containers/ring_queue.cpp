#include "ring_queue.h"

#include "core/cememory.h"
#include "core/logger.h"

namespace caliope {
	bool ring_queue_create(uint stride, uint capacity, void* memory, ring_queue& out_queue)
	{
		out_queue.length = 0;
		out_queue.capacity = capacity;
		out_queue.stride = stride;
		out_queue.head = 0;
		out_queue.tail = -1;
		if (memory) {
			out_queue.owns_memory = false;
			out_queue.block = memory;
		}
		else {
			out_queue.owns_memory = true;
			out_queue.block = allocate_memory(MEMORY_TAG_RING_QUEUE, capacity * stride);
		}

		return true;
	}

	void ring_queue_destroy(ring_queue& queue)
	{
		if (queue.owns_memory) {
			free_memory(MEMORY_TAG_RING_QUEUE, queue.block, queue.capacity * queue.stride);
		}

		zero_memory(&queue, sizeof(ring_queue));
	}

	bool ring_queue_enqueue(ring_queue& queue, void* value)
	{
		if (value != nullptr) {
			if (queue.length == queue.capacity) {
				CE_LOG_ERROR("ring_queue_enqueue cannot enqueue the value, ring_queue full");
				return false;
			}

			queue.tail = (queue.tail + 1) % queue.capacity;

			copy_memory((char*)queue.block + (queue.tail * queue.stride), value, queue.stride);
			queue.length++;
			return true;
		}

		CE_LOG_ERROR("ring_queue_enqueue requires a valid value");
		return false;
	}

	bool ring_queue_dequeue(ring_queue& queue, void* out_value)
	{
		if (out_value != nullptr) {
			if (queue.length == 0) {
				CE_LOG_ERROR("ring_queue_dequeue the queue is empty");
			}

			copy_memory(out_value, (char*)queue.block + (queue.head * queue.stride), queue.stride);
			queue.head = (queue.head + 1) % queue.capacity;
			queue.length--;
			return true;
		}
		
		CE_LOG_ERROR("ring_queue_dequeue requires a valid out_value");
		return false;
	}

	bool ring_queue_peek(const ring_queue& queue, void* out_value)
	{
		if (out_value != nullptr) {
			if (queue.length == 0) {
				CE_LOG_ERROR("ring_queue_dequeue the queue is empty");
			}

			copy_memory(out_value, (char*)queue.block + (queue.head * queue.stride), queue.stride);
			return true;
		}

		CE_LOG_ERROR("ring_queue_dequeue requires a valid out_value");
		return false;
	}
}