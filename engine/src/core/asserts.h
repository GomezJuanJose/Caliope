#pragma once

#include "defines.h"

#define CE_ASSERTION_ENABLED

#ifdef CE_ASSERTION_ENABLED
	#if _MSC_VER
		#include <intrin.h>
		#define debugbreak() __debugbreak()
	#else
		#define debugbreak() __builtin_trap()
	#endif

#define CE_ASSERT(expr)			\
		{						\
			if (expr) {			\
			}					\
			else {				\
				debugbreak();	\
			}					\
		}

#else
	#define CE_ASSERT(expr)
#endif // CE_ASSERTION_ENABLED
