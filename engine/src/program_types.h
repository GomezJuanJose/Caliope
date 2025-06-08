#pragma once

#include <string>

namespace caliope {
	typedef struct program_config {

		std::string name;
		int width;
		int height;

		bool (*initialize) ();
		bool (*update) ();
		bool (*resize) ();
		void (*shutdown) ();


	} program_config;
}