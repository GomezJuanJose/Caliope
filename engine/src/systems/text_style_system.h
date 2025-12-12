#pragma once
#include "defines.h"
#include <glm/glm.hpp>

namespace caliope {

	struct text_style_table;
	struct text_font;
	struct material;

	typedef struct text_style {
		uint tag_name_length; // Is used to skip the characters of the tag to not being renderer;
		text_font* font;
		uint font_size;
		uint additional_interlinial_space; //NOTE: Is on pixels
		glm::vec4 text_color;
	} text_style;

	typedef struct text_image_style {
		material* material;
		float image_size;
	} text_image_style;

	bool text_style_system_initialize();
	void text_style_system_shutdown();

	CE_API text_style_table* text_style_system_adquire_text_style_table(std::string& name);
	CE_API text_style text_style_system_adquire_text_style(text_style_table* style_table, std::string& text_tag);
	CE_API text_image_style text_style_system_adquire_text_image_style(text_style_table* style_table, std::string& image_tag);
	CE_API void text_style_system_get_metrics(text_style_table* style_table, std::string& text, std::vector<text_style>& styles, std::vector<uint>& style_starting_indices, std::vector<uint>& style_ending_indices);

	CE_API void text_style_system_release(std::string& name);
	

}