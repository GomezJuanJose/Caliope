#include "platform.h"

#if CE_PLATFORM_WINDOWS
#include "core/logger.h"

#include <string>


namespace caliope {
	bool platform_system_startup(const std::string& window_name, int width, int height) {


		CE_LOG_INFO("Windows platform initialized");
		return true;
	}

	void platform_system_shutdown() {

	}
	void* platform_system_allocate_memory(size_t size) {
		return malloc(size);
	}
}
#endif // CE_PLATFORM_WINDOWS
