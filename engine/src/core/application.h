#pragma once
#include "../defines.h"

namespace caliope {
	struct program_config;

	CE_API bool application_create(program_config&);

	CE_API bool application_run();
}