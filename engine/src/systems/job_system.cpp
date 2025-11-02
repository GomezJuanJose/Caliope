#include "job_system.h"

#include "core/cememory.h"
#include "core/logger.h"
#include "containers/ring_queue.h"

#include <thread>
#include <mutex>
#include <chrono>

namespace caliope {

	typedef struct job_thread {
		uchar index;
		std::thread thread;
		job_info info;
		// A mutex to guard access to this thread's info.
		std::mutex info_mutex;

		// The types of jobs this thread can handle
		uint type_mask;
	}job_thread;

	typedef struct job_result_entry {
		uint16 id;
		pfn_job_on_complete callback;
		uint param_size;
		void* params;
	}job_result_entry;

	// The max number of job results that can be stored at once.
	#define MAX_JOB_RESULTS 512

	typedef struct job_system_state {
		bool running;
		uchar thread_count;
		job_thread job_threads[32];

		ring_queue low_priority_queue;
		ring_queue normal_priority_queue;
		ring_queue high_priority_queue;

		// Mutex for each queue, since a job could kick off from another job (thread).
		std::mutex  low_pri_queue_mutex;
		std::mutex  normal_pri_queue_mutex;
		std::mutex  high_pri_queue_mutex;

		job_result_entry pending_results[MAX_JOB_RESULTS];
		std::mutex result_mutex;
	} job_system_state;

	std::unique_ptr<job_system_state> state_ptr;

	void store_result(pfn_job_on_complete callback, uint param_size, void* params) {
		// Create the new entry
		job_result_entry entry;
		entry.id = INVALID_ID_U16;
		entry.param_size = param_size;
		entry.callback = callback;
		if (entry.param_size > 0) {
			// Take a copy, as the job is destroyed after this.
			entry.params = allocate_memory(MEMORY_TAG_JOB, param_size);
			copy_memory(entry.params, params, param_size);
		}
		else {
			entry.params = 0;
		}

		// Lock, find a free space, store, unlock
		state_ptr->result_mutex.lock();
		
		for (uint16 i = 0; i < MAX_JOB_RESULTS; ++i) {
			if (state_ptr->pending_results[i].id == INVALID_ID_U16) {
				state_ptr->pending_results[i] = entry;
				state_ptr->pending_results[i].id = i;
				break;
			}
		}

		state_ptr->result_mutex.unlock();

	}

	uint job_thread_run(void* params) {
		uint index = *(uint*)params;
		job_thread* thread = &state_ptr->job_threads[index];
		std::thread::id thread_id = thread->thread.get_id();
		CE_LOG_INFO("Starting job thread %#i (id=%#i, type=%#i).", thread->index, thread_id, thread->type_mask);

		while (true)
		{
			if (!state_ptr || !state_ptr->running || !thread) {
				break;
			}

			thread->info_mutex.lock();
			job_info info = thread->info;
			thread->info_mutex.unlock();

			if (info.entry_point) {
				bool result = info.entry_point(info.param_data, info.result_data);

				// Store the result to bre executed on the main thread later.
				// Note that store_result takes a copy of the reuslt_data
				// so it does not have to be held onto by this thread any longer.
				if (result && info.on_success) {
					store_result(info.on_success, info.result_data_size, info.result_data);
				}
				else if (!result && info.on_fail) {
					store_result(info.on_fail, info.result_data_size, info.result_data);
				}
			
				// Lock and reset the thread's info object
				thread->info_mutex.lock();
				zero_memory(&thread->info, sizeof(job_info));
				thread->info_mutex.unlock();
			}

			if (state_ptr->running) {
				// TODO: Should probably find a better way to do this, such as sleeping until a request comes throught for a new job.
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}

		return 1;
	}

	bool job_system_initialize(uchar max_job_thread_count, uint type_masks[])
	{
		state_ptr = std::make_unique<job_system_state>();

		state_ptr->running = true;

		ring_queue_create(sizeof(job_info), 1024, 0, state_ptr->low_priority_queue);
		ring_queue_create(sizeof(job_info), 1024, 0, state_ptr->normal_priority_queue);
		ring_queue_create(sizeof(job_info), 1024, 0, state_ptr->high_priority_queue);
		state_ptr->thread_count = max_job_thread_count;

		// Invalidate all result slots
		for (uint16 i = 0; i < MAX_JOB_RESULTS; ++i) {
			state_ptr->pending_results[i].id = INVALID_ID_U16;
		}

		CE_LOG_INFO("Main thread id is: %#x", std::this_thread::get_id());
		CE_LOG_INFO("Spawning %i job threads.", state_ptr->thread_count);

		for (uchar i = 0; i < state_ptr->thread_count; ++i) {
			state_ptr->job_threads[i].index = i;
			state_ptr->job_threads[i].type_mask = type_masks[i];
			state_ptr->job_threads[i].thread = std::thread(job_thread_run, &state_ptr->job_threads[i].index);
			zero_memory(&state_ptr->job_threads[i].info, sizeof(job_info));
		}

		CE_LOG_INFO("Job system initialized.");

		return true;
	}

	void job_system_shutdown()
	{
		if (state_ptr) {
			state_ptr->running = false;

			uint64 thread_count = state_ptr->thread_count;

			// Check for a free thread first.
			for (uchar i = 0; i < thread_count; ++i) {
				state_ptr->job_threads[i].thread.join();
				state_ptr->job_threads[i].thread.~thread();
			}

			ring_queue_destroy(state_ptr->low_priority_queue);
			ring_queue_destroy(state_ptr->normal_priority_queue);
			ring_queue_destroy(state_ptr->high_priority_queue);
	
			state_ptr.reset();
			state_ptr = nullptr;
		}
	}

	void process_queue(ring_queue& queue, std::mutex& queue_mutex) {
		uint64 thread_count = state_ptr->thread_count;

		// Check for a free thread first
		while (queue.length > 0) {
			job_info info;
			if (!ring_queue_peek(queue, &info)) {
				break;
			}

			bool thread_found = false;
			for (uchar i = 0; i < thread_count; ++i) {
				job_thread& thread = state_ptr->job_threads[i];
				if ((thread.type_mask & info.type) == 0) {
					continue;
				}

				// Check that the job thread can handle the job type.
				thread.info_mutex.lock();

				if (!thread.info.entry_point) {
					// Make sure to remove the entry from the queue.
					queue_mutex.lock();
					ring_queue_dequeue(queue, &info);
					queue_mutex.unlock();

					thread.info = info;
					CE_LOG_INFO("Assigning job to thread: %u", thread.index);
					thread_found = true;
				}

				thread.info_mutex.unlock();

				// Break after unlocking if an available thread was found.
				if (thread_found) {
					break;
				}
			}

			// This means all of the threads are currently handling a job, so wait until the next update and try again.
			if (!thread_found) {
				break;
			}
		}
	}

	void job_system_update()
	{
		if (state_ptr == nullptr || !state_ptr->running) {
			return;
		}

		process_queue(state_ptr->high_priority_queue, state_ptr->high_pri_queue_mutex);
		process_queue(state_ptr->normal_priority_queue, state_ptr->normal_pri_queue_mutex);
		process_queue(state_ptr->low_priority_queue, state_ptr->low_pri_queue_mutex);

		// Process pending results.
		for (uint16 i = 0; i < MAX_JOB_RESULTS; ++i) {
			// Lock and take a copy, unlock
			state_ptr->result_mutex.lock();
			job_result_entry entry = state_ptr->pending_results[i];
			state_ptr->result_mutex.unlock();

			if (entry.id != INVALID_ID_U16) {
				// Execute the callbacks.
				entry.callback(entry.params);

				if (entry.params) {
					free_memory(MEMORY_TAG_JOB, entry.params, entry.param_size);
				}

				state_ptr->result_mutex.lock();
				zero_memory(&state_ptr->pending_results[i], sizeof(job_result_entry));
				state_ptr->pending_results[i].id = INVALID_ID_U16;
				state_ptr->result_mutex.unlock();
			}
		}
	}

	void job_system_submit(job_info info)
	{
		uint64 thread_count = state_ptr->thread_count;
		ring_queue* queue = &state_ptr->normal_priority_queue;
		std::mutex* queue_mutex = &state_ptr->normal_pri_queue_mutex;

		// If the job is high priority, try to kick if off immediately.
		if (info.priority == JOB_PRIORITY_HIGH) {
			queue = &state_ptr->high_priority_queue;
			queue_mutex = &state_ptr->high_pri_queue_mutex;

			// Check for a free thread that supports the job type first.
			for (uchar i = 0; i < thread_count; ++i) {
				job_thread* thread = &state_ptr->job_threads[i];

				if (state_ptr->job_threads[i].type_mask & info.type) {
					bool found = false;

					thread->info_mutex.lock();
					if (!state_ptr->job_threads[i].info.entry_point) {
						CE_LOG_INFO("Job immediately submitted on thread %i", state_ptr->job_threads[i].index);
						state_ptr->job_threads[i].info = info;
						found = true;
					}
					thread->info_mutex.unlock();
					if (found) {
						return;
					}
				}
			}
		}

		// If this point is reached, all threads are busy (if high) or it can wait a frame. Add to the queue and try again next cycle.
		if (info.priority == JOB_PRIORITY_LOW) {
			queue = &state_ptr->low_priority_queue;
			queue_mutex = &state_ptr->low_pri_queue_mutex;
		}

		// NOTE: Locking here in case the job is submitted from another job/thread
		queue_mutex->lock();
		ring_queue_enqueue(*queue, &info);
		queue_mutex->unlock();

		CE_LOG_INFO("Job queued");
	}

	job_info job_create(pfn_job_start entry_point, pfn_job_on_complete on_success, pfn_job_on_complete on_fail, void* param_data, uint param_data_size, uint result_data_size)
	{
		return job_create_priority(entry_point, on_success, on_fail, param_data, param_data_size, result_data_size, JOB_TYPE_GENERAL, JOB_PRIORITY_NORMAL);
	}

	job_info job_create_type(pfn_job_start entry_point, pfn_job_on_complete on_success, pfn_job_on_complete on_fail, void* param_data, uint param_data_size, uint result_data_size, job_type type)
	{
		return job_create_priority(entry_point, on_success, on_fail, param_data, param_data_size, result_data_size, type, JOB_PRIORITY_NORMAL);;
	}

	job_info job_create_priority(pfn_job_start entry_point, pfn_job_on_complete on_success, pfn_job_on_complete on_fail, void* param_data, uint param_data_size, uint result_data_size, job_type type, job_priority priority)
	{
		job_info job;
		job.entry_point = entry_point;
		job.on_success = on_success;
		job.on_fail = on_fail;
		job.type = type;
		job.priority = priority;

		job.param_data_size = param_data_size;
		if (param_data_size) {
			job.param_data = allocate_memory(MEMORY_TAG_JOB, param_data_size);
			copy_memory(job.param_data, param_data, param_data_size);
		}
		else {
			job.param_data = 0;
		}

		job.result_data_size = result_data_size;
		if (result_data_size) {
			job.result_data = allocate_memory(MEMORY_TAG_JOB, result_data_size);
		}
		else
		{
			job.result_data = 0;
		}

		return job;
	}
}