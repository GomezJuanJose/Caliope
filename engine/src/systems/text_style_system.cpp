#include "text_style_system.h"
#include "resources/resources_types.inl"
#include "core/logger.h"
#include "core/cestring.h"
#include "core/cememory.h"

#include "systems/resource_system.h"
#include "systems/material_system.h"
#include "systems/text_font_system.h"

namespace caliope {

	typedef struct text_style_reference {
		text_style_table text_style;
		uint reference_count;
	} text_style_reference;


	typedef struct text_style_system_state {
		std::unordered_map<std::string, text_style_reference> registered_style_tables;

	} text_style_system_state;

	static std::unique_ptr<text_style_system_state> state_ptr;

	bool load_text_style(text_style_resource_data& text_style_config);
	void destroy_text_style(text_style_table& ts);

	bool text_style_system_initialize()
	{
		state_ptr = std::make_unique<text_style_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		CE_LOG_INFO("Text style system initialized.");
		return true;
	}

	void text_style_system_shutdown()
	{
		for (auto [key, value] : state_ptr->registered_style_tables) {
			destroy_text_style(value.text_style);
			value.reference_count = 0;
		}

		state_ptr->registered_style_tables.clear();
		state_ptr.reset();
		state_ptr = nullptr;
	}

	text_style_table* text_style_system_adquire_text_style_table(std::string& name)
	{
		if (state_ptr->registered_style_tables.find(name) == state_ptr->registered_style_tables.end()) {

			resource r;
			if (!resource_system_load(name, RESOURCE_TYPE_TEXT_STYLE, r)) {
				CE_LOG_ERROR("text_style_system_adquire couldnt load file text style");
				return nullptr;
			}
			text_style_resource_data text_style_config = std::any_cast<text_style_resource_data>(r.data);
			resource_system_unload(r);


			if (!load_text_style(text_style_config)) {
				CE_LOG_ERROR("text_style_system_adquire couldnt adquire text style");
				return nullptr;
			}

		}

		state_ptr->registered_style_tables[name].reference_count++;
		return &state_ptr->registered_style_tables[name].text_style;
	}

	text_style text_style_system_adquire_text_style(text_style_table* style_table, std::string& text_tag)
	{
		text_style style;

		if (style_table == nullptr) {
			CE_LOG_WARNING("text_style_system_adquire_text_style the style is null.");
			return text_style();
		}

		if (state_ptr->registered_style_tables.at(style_table->name).text_style.tag_style_indexes.find(text_tag) == state_ptr->registered_style_tables.at(style_table->name).text_style.tag_style_indexes.end()) {
			CE_LOG_WARNING("text_style_system_adquire_text_style the tag %s not found.", text_tag.c_str());
			return text_style(); // TODO: Return a default
		}

		uint index_style = state_ptr->registered_style_tables.at(style_table->name).text_style.tag_style_indexes.at(text_tag);

		style.tag_name_length = text_tag.size();
		style.font = state_ptr->registered_style_tables.at(style_table->name).text_style.fonts[index_style];
		style.font_size = state_ptr->registered_style_tables.at(style_table->name).text_style.text_sizes[index_style];
		style.additional_interlinial_space = state_ptr->registered_style_tables.at(style_table->name).text_style.additional_interline_spaces[index_style];
		style.text_color = state_ptr->registered_style_tables.at(style_table->name).text_style.text_colors[index_style];

		return style;
	}

	text_image_style text_style_system_adquire_text_image_style(text_style_table* style_table, std::string& image_tag)
	{
		text_image_style style;

		if (style_table == nullptr) {
			CE_LOG_WARNING("text_style_system_adquire_text_image_style the image style is null.");
			return text_image_style();
		}

		if (state_ptr->registered_style_tables.at(style_table->name).text_style.tag_image_indexes.find(image_tag) != state_ptr->registered_style_tables.at(style_table->name).text_style.tag_style_indexes.end()) {
			CE_LOG_WARNING("text_style_system_adquire_text_image_style the tag %s not found.", image_tag.c_str());
			return text_image_style();
		}

		uint index_style = state_ptr->registered_style_tables.at(style_table->name).text_style.tag_image_indexes.at(image_tag);

		style.material = state_ptr->registered_style_tables.at(style_table->name).text_style.materials[index_style];
		style.image_size = state_ptr->registered_style_tables.at(style_table->name).text_style.image_sizes[index_style];

		return style;
	}

	void text_style_system_get_metrics(text_style_table* style_table, std::string& text, std::vector<text_style>& styles, std::vector<uint>& style_starting_indices, std::vector<uint>& style_ending_indices)
	{

		// TODO: Redo to make read this simpler format {tag_name|Text wants to be formatted}

		bool start_tag_retrieving = false;
		bool tags_matches = false;
		std::string found_tag_name = "";
		uint found_tag_starting_index = 0;
		uint found_tag_ending_index = 0;

		for (uint char_index = 0; char_index < text.size(); ++char_index) {
			
			if (start_tag_retrieving == true) {
				start_tag_retrieving = false;

				if (tags_matches == false) {
					// Iterates the style table to get each tag and first check if it has a > at the end, 
					// second substring the tag found in the text and check if the name exists in the table
					for (auto [tag, id] : style_table->tag_style_indexes) {
						bool has_close_symbol = text.at(char_index + tag.size()) == '|';

						found_tag_name = string_substring(&text, char_index, tag.size());
						tags_matches = style_table->tag_style_indexes.find(found_tag_name) != style_table->tag_style_indexes.end();

						if (has_close_symbol && tags_matches) {
							found_tag_starting_index = char_index - 1; // Minus 1 due to  previous iteration has the special character '{'
							char_index += tag.size();
							break;
						}
					}
				}
			}

			// If is the end format tag (}), reset
			if (tags_matches && text[char_index] == '}') {
				found_tag_ending_index = char_index;

				styles.push_back(text_style_system_adquire_text_style(style_table, found_tag_name));
				style_starting_indices.push_back(found_tag_starting_index);
				style_ending_indices.push_back(found_tag_ending_index);

				found_tag_name = "";
				tags_matches = false;
				found_tag_starting_index = 0;
				found_tag_ending_index = 0;
				continue;
			}
			
			// If is start format, starts to the search for the tag name
			if (text.at(char_index) == '{') {
				start_tag_retrieving = true;
			}
		}
	}

	void text_style_system_release(std::string& name)
	{
		if (state_ptr->registered_style_tables.find(name) != state_ptr->registered_style_tables.end()) {

			state_ptr->registered_style_tables[name].reference_count++;

			if (state_ptr->registered_style_tables[name].reference_count <= 0) {
				destroy_text_style(state_ptr->registered_style_tables[name].text_style);
				state_ptr->registered_style_tables.erase(name);
			}
		}
	}


	bool load_text_style(text_style_resource_data& text_style_config) {
		text_style_reference tsr;
		
		tsr.text_style.name = std::string(&text_style_config.name[0]);

		for (uint i = 0; i < text_style_config.style_tag_names.size(); ++i) {
			tsr.text_style.tag_style_indexes.insert({std::string(&text_style_config.style_tag_names[i][0]), i});
			tsr.text_style.fonts.push_back(text_font_system_adquire_font(std::string(&text_style_config.text_fonts[i][0]), text_style_config.font_sizes[i]));
			tsr.text_style.text_sizes.push_back(text_style_config.font_sizes[i]);
			tsr.text_style.additional_interline_spaces.push_back(text_style_config.additional_interline_spaces[i]);
			tsr.text_style.text_colors.push_back(text_style_config.text_colors[i]);
		}

		for (uint i = 0; i < text_style_config.image_tag_names.size(); ++i) {
			tsr.text_style.tag_image_indexes.insert({ std::string(&text_style_config.image_tag_names[i][0]), i });
			tsr.text_style.materials.push_back(material_system_adquire(std::string(&text_style_config.image_materials[i][0])));
			tsr.text_style.image_sizes.push_back(text_style_config.image_sizes[i]);
		}

		state_ptr->registered_style_tables.insert({ tsr.text_style.name, tsr });

		return true;
	}

	void destroy_text_style(text_style_table& ts) {
		state_ptr->registered_style_tables.erase(ts.name);
		ts.name = "";
		
		for (uint i = 0; i < ts.materials.size(); ++i) {
			text_font_system_release(ts.fonts[i]->name);
			material_system_release(ts.materials[i]->name);
		}

		ts.tag_style_indexes.clear();
		ts.fonts.clear();
		ts.text_sizes.clear();
		ts.text_colors.clear();
		ts.tag_image_indexes.clear();
		ts.materials.clear();
		ts.image_sizes.clear();
	}
}