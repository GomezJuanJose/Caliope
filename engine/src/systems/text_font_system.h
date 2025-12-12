#pragma once
#include "defines.h"

namespace caliope {

	struct text_font;
	struct text_font_glyph;

	bool text_font_system_initialize();
	void text_font_system_shutdown();

	CE_API text_font* text_font_system_adquire_font(std::string& name, uint font_size);
	CE_API text_font_glyph* text_font_system_get_glyph(text_font* font, uint codepoint);

	CE_API void text_font_system_release(std::string& name);

}