#include "cestring.h"
#include <cstdarg>
#include <iostream>
#include <memory>

namespace caliope {
	bool strings_equal(std::string* str1, std::string* str2)
	{
		if (!str1 || !str2) {
			return false;
		}

		return strcmp(str1->c_str(), str2->c_str()) == 0;
	}

	bool strings_equali(std::string* str1, std::string* str2)
	{
		if (!str1 || !str2) {
			return false;
		}

#if defined __GNUC__	
		return strcasecmp(str1.c_str(), str2.c_str()) == 0;
#elif defined _MSC_VER
		return strcmpi(str1->c_str(), str2->c_str()) == 0;
#endif
	}
	
	void string_format(std::string* str_format, char (& out_buffer)[2048], ...) // NOTE: Increase the buffer size in case an exception of callstack is called
	{
		if (!str_format) {
			return;
		}

		std::va_list args;
		va_start(args, str_format);
		vsprintf_s(out_buffer, str_format->c_str(), args);
		va_end(args);
	}
	
	void string_trim_character(std::string* str, char character)
	{
		if (!str) {
			return;
		}

		str->erase(std::remove_if(str->begin(), str->end(), 
			[character](char c) {
				return c == character;
			}), 
			str->end());
	}
	
	std::string string_substring(std::string* str, uint start, int count)
	{
		if (!str) {
			return std::string();
		}

		if (count + start > str->size() - 1 || count == -1) {
			count = - 1;
		}

		return str->substr(start, count);
	}
	
	void string_split(std::string* str, std::string* left, std::string* right, char separator)
	{
		if (!str) {
			return ;
		}
		
		std::stringstream ss(str->c_str());
		std::getline(ss, *left, separator);
		std::getline(ss, *right, separator);
	}
	
	bool string_to_vec4(std::string* str, glm::vec4* out_vec)
	{
		if (!str || !out_vec) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%f %f %f %f", &out_vec->x, &out_vec->y, &out_vec->z, &out_vec->w);
		return result != -1;
	}
	
	bool string_to_vec3(std::string* str, glm::vec3* out_vec)
	{
		if (!str || !out_vec) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%f %f %f", &out_vec->x, &out_vec->y, &out_vec->z);
		return result != -1;
	}
	
	bool string_to_vec2(std::string* str, glm::vec2* out_vec)
	{
		if (!str || !out_vec) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%f %f", &out_vec->x, &out_vec->y);
		return result != -1;
	}
	
	bool string_to_char(std::string* str, char* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%hhi", out_value);
		return result != -1;
	}
	
	bool string_to_int16(std::string* str, int16* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%hi", out_value);
		return result != -1;
	}
	
	bool string_to_int(std::string* str, int* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%i", out_value);
		return result != -1;
	}
	
	bool string_to_int64(std::string* str, int64* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%lli", out_value);
		return result != -1;
	}
	
	bool string_to_uchar(std::string* str, uchar* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%hhu", out_value);
		return result != -1;
	}
	
	bool string_to_ushort(std::string* str, uint16* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%hu", out_value);
		return result != -1;
	}
	
	bool string_to_uint(std::string* str, uint* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%u", out_value);
		return result != -1;
	}
	
	bool string_to_uint64(std::string* str, uint64* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%llu", out_value);
		return result != -1;
	}
	
	bool string_to_float(std::string* str, float* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%f", out_value);
		return result != -1;
	}
	
	bool string_to_double(std::string* str, double* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		int result = std::sscanf(str->c_str(), "%lf", out_value);
		return result != -1;
	}
	
	bool string_to_bool(std::string* str, bool* out_value)
	{
		if (!str || !out_value) {
			return false;
		}

		*out_value = strings_equal(str, &std::string("1")) || strings_equali(str, &std::string("true"));

		return true;
	}

	void string_append_string(std::string* str, std::string* append)
	{
		if (!str || !append) {
			return;
		}

		str->append(append->c_str());
	}
}