#include "program.h"

#include <core/logger.h>
#include <core/input.h>

bool initialize_testbed() {
	CE_LOG_INFO("Initialize testbed");

	return true;
}

bool update_testbed(float delta_time) {

	if (caliope::is_key_pressed(caliope::KEY_A)) {
		CE_LOG_INFO("key A pressed!");
	}

	if (caliope::is_key_released(caliope::KEY_A)) {
		CE_LOG_INFO("key A released!");
	}

	return true;
}

bool resize_testbed() {
	return true;
}

void shutdown_testbed() {
}
