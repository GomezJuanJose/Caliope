#pragma once

#include <program_types.h>

bool initialize_testbed(caliope::game_state& game_state);
bool update_testbed(caliope::game_state& game_state, float delta_time);
bool resize_testbed();
void shutdown_testbed();