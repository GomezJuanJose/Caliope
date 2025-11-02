#pragma once
#include "defines.h"

namespace caliope {
	typedef bool (*pfn_job_start)(void*, void*);

	typedef void (*pfn_job_on_complete)(void*);

	typedef enum job_type {
		JOB_TYPE_GENERAL = 0x02,
		JOB_TYPE_RESOURCE_LOAD = 0x04,
		JOB_TYPE_GPU_RESOURCE = 0x08
	} job_type;

	typedef enum job_priority {
		JOB_PRIORITY_LOW,
		JOB_PRIORITY_NORMAL,
		JOB_PRIORITY_HIGH
	} job_priority;

	typedef struct job_info {
		job_type type;
		job_priority priority;
		
		pfn_job_start entry_point;
		pfn_job_on_complete on_success;
		pfn_job_on_complete on_fail;

		void* param_data;
		uint param_data_size;
		
		void* result_data;
		uint result_data_size;

	} job_info;

	/**
	 * @note max_job_thread_count is the maximum number of job threads to be spun up. Should be no more than the number of cores on the CPU, minus one to account the main thread.
	 */
	bool job_system_initialize(uchar max_job_thread_count, uint type_masks[]);

	void job_system_shutdown();

	void job_system_update();

	CE_API void job_system_submit(job_info info);

	CE_API job_info job_create(pfn_job_start entry_point, pfn_job_on_complete on_success, pfn_job_on_complete on_fail, void* param_data, uint param_data_size, uint result_data_size);
	CE_API job_info job_create_type(pfn_job_start entry_point, pfn_job_on_complete on_success, pfn_job_on_complete on_fail, void* param_data, uint param_data_size, uint result_data_size, job_type type);
	CE_API job_info job_create_priority(pfn_job_start entry_point, pfn_job_on_complete on_success, pfn_job_on_complete on_fail, void* param_data, uint param_data_size, uint result_data_size, job_type type, job_priority priority);
}