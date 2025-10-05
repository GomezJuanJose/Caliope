#pragma once

#include "defines.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>


namespace caliope {
	CE_API bool strings_equal(std::string* str1, std::string* str2);
	CE_API bool strings_equali(std::string* str1, std::string* str2);

	CE_API void string_format(std::string* str_format, char(&out_buffer)[2048], ...);
	CE_API void string_trim(std::string* str);
	// If end is -1 means to the end of the string
	CE_API std::string string_substring(std::string* str, uint start, int end);
	CE_API void string_split(std::string* str, std::string* left, std::string* right, char separator);

	CE_API bool string_to_vec4(std::string* str, glm::vec4* out_vec);
	CE_API bool string_to_vec3(std::string* str, glm::vec3* out_vec);
	CE_API bool string_to_vec2(std::string* str, glm::vec2* out_vec);

	CE_API bool string_to_char(std::string* str, char* out_value);
	CE_API bool string_to_int16(std::string* str, int16* out_value);
	CE_API bool string_to_int(std::string* str, int* out_value);
	CE_API bool string_to_int64(std::string* str, int64* out_value);

	CE_API bool string_to_uchar(std::string* str, uchar* out_value);
	CE_API bool string_to_ushort(std::string* str, uint16* out_value);
	CE_API bool string_to_uint(std::string* str, uint* out_value);
	CE_API bool string_to_uint64(std::string* str, uint64* out_value);

	CE_API bool string_to_float(std::string* str, float* out_value);
	CE_API bool string_to_double(std::string* str, double* out_value);

	// It detects 0 as false other value as true, also detects the true/false words
	CE_API bool string_to_bool(std::string* str, bool* out_value);

	CE_API void string_append_string(std::string* str, std::string* append);
}